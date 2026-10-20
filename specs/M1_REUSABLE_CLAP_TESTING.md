# M1: Reusable tiered CLAP testing

**Status:** Implementing. **Revision:** 1.0. **Scope:** Windows x64 acceptance target; Linux is used for local development checks. The current implementation supports native CLAP binaries and the in-repository synthetic fixture.

## User contract

SoundGrind supplies the test implementation. A developer or coding model selects a registered module/profile and supplies values through the CLI or `CampaignConfig`; it does not write a new host or test harness. The stable C++ entry point is:

```cpp
CampaignResult run_campaign(const CampaignRequest& request);
```

Types and defaults are in `include/soundgrind/engine.hpp`. CLI and API share the same campaign engine. JSON case files use `soundgrind.case.v1`.

## Modules

| Module ID | Test and evidence | Unsupported / limits |
| --- | --- | --- |
| `clap.lifecycle` | Entry, factory, plugin init, activate, process, stop, deactivate and destroy; process status | Fails on lifecycle/process error |
| `audio.finite_output` | Scans output samples in dwell processing and block probes; reports channel and first bad sample | `UNSUPPORTED` when there is no output port |
| `fixture.oracle` | Checks the known synthetic fixture against input × gain and mode with absolute tolerance 1e-6 | Other plugins have no generic correctness oracle and report `UNSUPPORTED` |
| `lifecycle.repeat` | Re-activates and processes again for legal block probes; records cycle count | No audio port means no repeat render |
| `params.readback` | Applies complete case values as CLAP parameter events and reads effective values after processing | `UNSUPPORTED` without `clap.params` |
| `params.boundaries` | Applies default, min/max, every stepped/enum value up to 256 values per parameter; checks readback and enum labels | `INCONCLUSIVE` when a stepped domain exceeds the cap; `UNSUPPORTED` without params |
| `params.normalized_mapping` | Tests SoundGrind's documented linear normalized/plain mapping at 0, .5 and 1; stepped values are quantized | This is the host mapping, not an advertised target-specific logarithmic scale |
| `processing.block_sizes` | Probes 1, 16, 32, 64, 128, 256, 512, 1024 and configured frame sizes within the activation maximum | `UNSUPPORTED` without an output port |
| `state.roundtrip` | Saves state, mutates parameter values while inactive, loads state and verifies restored readback | `UNSUPPORTED` without `clap.state` |

Each module is listed in `registered_modules()` with a stable identifier, version, description and required capability names. Unknown module IDs and empty selections are rejected. A profile is a list of module IDs, not a separate hard-coded runner.

## Profiles and case accounting

`level1` runs lifecycle, finite-output, fixture-oracle, repeated lifecycle and parameter-readback checks. It defaults to one unique state and three rounds. `level2` adds boundary, normalized mapping, block-size and state round-trip modules. `level3` runs a caller-bounded number of randomized complete states and rounds. `fuzz1` does valid-domain randomization for explicitly selected modules. `fuzz2` uses the seed to choose a subset of the registered profile modules. `grind` samples a complete valid parameter snapshot for each state and selects per-case dwell from a caller-bounded interval.

`planned_cases = unique_states × rounds_per_state`. A round repeats the same state; a new state gets a new seed-derived snapshot. Modules are run within each case, so module count does not multiply the number of cases. `--max-cases` is mandatory as an effective upper bound; exceeding it prevents execution. `grind` defaults to 1–5,000 ms per case and is bounded by states, rounds and max cases.

Writable parameters are recorded in full for each generated state. Discovered but unwritable parameters are retained in `excluded_parameters` with a reason. The fixture's `fixture.fault` parameter is always held at its safe zero value unless a fault mode is explicitly requested.

## Audio timing

The harness never opens an audio device. Requested dwell is converted to samples using ceiling, then rounded to whole configured blocks:

`requested_samples = ceil(dwell_ms × sample_rate / 1000)`

`processed_samples = ceil(requested_samples / block_size) × block_size`

Zero-length renders are not permitted. Reports distinguish requested dwell, sample rate, block size, processed samples and effective dwell. Wall time spent in plugin initialization and teardown is not counted as audio dwell.

## Failure handling and replay

Before each worker starts, SoundGrind writes and flushes a complete case record to `cases/<case-id>.json`. It includes target and SHA-256, identity, module versions, seed and snapshot, fixture, processing setup, and planned sample count. The worker receives a versioned JSON request and writes a JSON response. A supervisor deadline kills hung workers. Crash, timeout, and result failures preserve the case record and worker stderr separately.

Replay verifies target SHA-256 before launching the worker. A mismatch stops replay unless `--allow-target-mismatch` is explicit; an override is recorded. Replay output goes to a separate output directory and leaves the source case untouched.

Process isolation only contains ordinary target crashes and hangs. M1 does not claim a security sandbox, malicious-code isolation, native code coverage, crash minidumps, GUI coverage, text-input coverage or exhaustive parameter coverage.

## Synthetic fixture

`org.soundgrind.synthetic` exposes mono input and output, continuous gain, stepped mode and hidden fault selector. Normal output is `input × gain`; modes invert or mute. Explicit test-only fault values produce NaN, terminate the worker, hang the worker, or offset output to fail the oracle. The fault selector is disabled by default and is excluded from random selection unless `--fixture-fault` is specified.

## Verification status

See `docs/M1_VERIFICATION.md`. Do not mark M1 verified until the Windows x64 CI workflow and acceptance suite pass.
