# SoundGrind M1 Implementation Prompt: Reusable Tiered CLAP Test Modules

Use this prompt to carry SoundGrind from its current design workspace to its first working build.

---

You are implementing **M1: SoundGrind’s first reusable CLAP testing vertical slice** in [djshellshoxxx/soundgrind](https://github.com/djshellshoxxx/soundgrind).

## Goal

Build a working Windows x64 command-line application and reusable testing library that loads a synthetic CLAP plugin in a supervised worker process, runs selectable, composable test modules at clearly defined test depths, detects planted failures, preserves evidence across worker crashes and hangs, replays saved cases, and produces machine-readable and human-readable reports.

SoundGrind’s central value is **pre-written, reusable tests**. An LLM helping develop an audio plugin should be able to select a SoundGrind test module and configure it for the target project, instead of generating a new test harness for every testing phase. Make test modules small, documented, independently testable, versioned, and callable through a stable API and CLI. Include one integration example that shows how an LLM or developer selects a profile, configures it, and invokes the existing test code. Do not require the LLM to author test implementation code.

This task authorizes implementing M1 only. It does not authorize the full SoundGrind roadmap.

## 1. Inspect the repository

Before changing files:

1. Inspect the current branch, working tree, repository structure, existing CI, and latest commit.
2. Read root `AGENTS.md` and `CLAUDE.md` if present.
3. Read the relevant project documents:
   - `README.md`
   - `prompts/MASTER_DESIGN_PROMPT.md`
   - `docs/PRODUCT_REQUIREMENTS.md`
   - `docs/RESEARCH_NOTES.md`
   - `specs/INDEX.md`
   - `specs/ULTIMATE_FUZZ_STRESS.md`
4. Verify the actual implementation and CI state; do not treat design notes as completed features.
5. Create a focused implementation branch. Preserve existing documents and unrelated work.

The repository is currently a design workspace. Existing requirements and fuzz notes are design inputs. Update them only where needed to keep the implemented M1 contracts accurate. Do not repeat broad architecture research unless a specific unresolved question requires it.

## 2. M1 implementation scope

Build a native **C++20 Windows x64 CLI and reusable test engine** with these commands:

- `inspect`: load the target and report identity, supported parameters, ports, and adapter capabilities.
- `test`: run one named profile or an explicitly composed set of test modules.
- `replay`: replay a saved case using the recorded snapshot and processing setup.
- `report`: produce a self-contained HTML report from campaign results; also retain JSON results.

Use offline rendering. Do not open a physical audio device or produce audible output. Default dwell is **1,000 ms of processed audio**, with documented sample-rate, block-size, and sample-count rules. Users can set shorter dwell for stress loops and longer dwell for stateful checks.

Separate modules for configuration and validation, test registry, campaign planning, target inspection, CLAP hosting, stimulus/rendering, state generation, worker supervision, evidence storage, replay, and reporting. Keep CLAP API calls inside the CLAP adapter. A profile is configuration over reusable test modules, not a separate hard-coded test implementation.

## 3. Reusable test model and profiles

Expose a stable API conceptually similar to:

```cpp
CampaignResult run_campaign(
    Target target,
    TestProfile profile,
    CampaignConfig config);
```

Also provide CLI commands that map to the same engine. Define versioned public contracts rather than requiring LLMs to guess function signatures. Document the actual signatures after implementation.

Use the following profile model. Names are user-facing presets; their selected checks must be composed from independently callable modules. Each module declares its required capabilities, setup, assertions/oracles, evidence, and unsupported conditions.

### Level 1 — Basic smoke

Quickly check the most fundamental observable behavior:

- Target can be loaded, initialized, activated, processed, deactivated, and destroyed in the expected lifecycle.
- Offline processing returns without a crash or hang and produces finite output.
- A known deterministic fixture stimulus follows the fixture’s documented expected behavior.
- Repeated start/process/stop cycles work for a small configured count.
- Basic parameter read/write works for representative supported parameters.

Do not assume every audio effect should produce non-silent output for every input, or every instrument should produce audio without MIDI. Define profile-appropriate stimuli and explicit fixture oracles. If a target has no applicable input/output path, report that check as `UNSUPPORTED`, not as passed.

### Level 2 — Feature and boundary checks

Run the applicable registered module set across every discovered capability SoundGrind can safely exercise:

- Parameter defaults, minimum/maximum, stepped and enum values, normalized conversion, and valid random samples.
- Parameter application and readback where the CLAP contract supports it.
- Repeated activation/deactivation, reset, and supported state save/restore round trips.
- Audio port and channel layouts, supported sample rates, and varied legal process block sizes.
- MIDI/note and transport event tests when the adapter and target expose those capabilities.
- Latency, tail, bypass, and extension checks only when the target advertises the relevant capability and the test has a meaningful oracle.
- Repeated complete snapshots and deterministic output checks for the synthetic fixture.

Report coverage per module and control. A discovered parameter is not “tested” merely because it was enumerated; show which values and operations were actually exercised. Do not claim to have tested custom GUI controls, text inputs, presets, or application-specific features unless M1 has an adapter and an observable oracle for them.

### Level 3 — Configurable state/round stress

Run a bounded campaign over selected features or parameter groups. Define these dimensions separately:

- **Feature selection:** which registered test modules or feature groups to run.
- **Unique states/iterations:** how many distinct valid parameter snapshots to generate for each selected group.
- **Rounds per state:** how many repeated render runs to perform using the same snapshot.
- **Dwell per run:** how many milliseconds of audio to process for each round.
- **Campaign budget:** optional maximum cases, wall-clock time, or cancellation condition.

For example, 30 unique states × 5 rounds per state × 2,000 ms means 30 distinct complete snapshots, each processed in five separate rounds of 2,000 ms. Record every dimension and actual processed sample count. Do not ambiguously use “iteration” for both a setting combination and a repeated run.

Changes between feature tests must use the CLAP protocol’s valid event/thread model. Do not directly mutate plugin parameters from an arbitrary thread.

### Fuzz Mode 1 — User-directed randomized testing

The caller explicitly selects the feature groups or registered tests, the number of unique settings/iterations, the rounds per state, dwell per round, seed, and optional parameter subset. Sample valid values from each parameter’s declared domain, including stepped and enum values. Every generated case must store a complete snapshot of all enabled, supported, writable parameters; record excluded controls and why they were excluded.

For example, selecting five feature groups and six unique states per group with 1 ms requested dwell means each applicable group receives six separately identified states and each run processes the minimum legal number of samples for the adapter. State the number of rounds independently. Do not imply that “five features × six states” necessarily results in only six total cases; report the actual planned case count before execution.

### Fuzz Mode 2 — Bounded adaptive/random selection

Use a recorded seed to select applicable test modules or feature groups and valid settings at random. Let the caller configure bounds for module count, states, rounds, and dwell. Randomized bounds are allowed only within those explicit limits. Persist the selected module, state, round, and random choices before applying each case so a crash remains reproducible.

### Grind Mode — Full-snapshot randomized stress

For every case, randomize every enabled, supported, writable parameter into a complete valid snapshot, then process it for a randomized dwell sampled from a caller-provided interval (default interval may be 1 ms to 5,000 ms). Continue until the caller’s case-count or wall-clock budget is reached, or the caller cancels. Require a recorded seed and bounded maximum run budget; never default to an unbounded campaign. Randomization must respect parameter domains and stepped values unless a separate invalid-input test explicitly requests malformed values and the adapter can safely deliver them.

All profile and fuzz modes must be reproducible from stored seeds, generator version, config, module versions, and case snapshots. Random testing is sampling, not exhaustive enumeration.

## 4. Timing, audio processing, and useful oracles

A millisecond of requested dwell can be shorter than a legal host callback block. Define dwell in **processed audio samples**, with the requested milliseconds converted using sample rate and rounded according to a documented policy. Process whole legal blocks as required. Record requested dwell, effective dwell, total samples, sample rate, block sizes, and rounding. A request such as 1 ms must never result in zero processing. It is acceptable that the minimum actual duration exceeds the requested duration when a whole block is required.

Avoid sleeping for the requested dwell as a substitute for processing audio. Ensure the worker does not enter a real-time audio device path. Include deterministic input fixtures for effects and MIDI/note fixtures for instruments where applicable.

M1 should report strong, objective failures with a clear classification, including worker crash, timeout/hang, non-finite output, rejected/invalid parameter operation, protocol/lifecycle failure, and deterministic fixture-oracle mismatch. Add other assertions only when evidence supports them. Do not label unusual but finite audio as incorrect without a documented oracle. Keep “no failure observed” distinct from “correctness proven.”

## 5. Failure records and campaign evidence

Before launching each case, write and flush a complete case record containing:

- Target path, format, plugin identity, and SHA-256.
- Campaign and case identifiers; profile, module, feature group, iteration/state index, and round index.
- Campaign seed, case seed, generator version, engine/schema version, and module version.
- Full parameter snapshot, plus excluded, unsupported, and unchanged controls.
- Audio/MIDI fixture identity, sample rate, requested dwell, block policy, and planned sample count.
- Worker request, start timestamp, and relevant adapter setup.

On completion, crash, or timeout, preserve the case record, worker exit status, stderr, last received worker event, observed output checks, timestamps, and available diagnostic artifact paths. The test harness must not overwrite original evidence during replay or retry. Stop retrying a failing case indefinitely.

A reportable failure should be structured conceptually like:

```json
{
  "type": "failure",
  "target": "Synthetic CLAP Fixture",
  "campaign_id": "…",
  "case_id": "…",
  "module": "audio.finite_output",
  "feature_group": "audio_processing",
  "state_index": 3,
  "round_index": 2,
  "parameter_snapshot": {"gain": 0.75, "mode": 1},
  "seed": 12345,
  "requested_dwell_ms": 2,
  "processed_samples": 128,
  "sample_rate": 48000,
  "elapsed_ms": 2.67,
  "classification": "non_finite_output",
  "details": "Output channel 0 contained NaN at sample 84",
  "worker_exit_code": 0,
  "artifacts": []
}
```

The actual schema may differ, but it must preserve equivalent actionable data and stable identifiers. By default, store failures and essential campaign metadata; allow users to opt into per-case success events. Always provide a summary of planned and completed cases, skipped/unsupported checks, and coverage, even when individual successes are omitted.

## 6. Synthetic CLAP fixture

Create an in-repository synthetic CLAP plugin with writable parameters covering supported parameter types, deterministic finite normal behavior, and documented expected output for known stimuli. Include fault modes that can be triggered deterministically by tests and are disabled by default:

- Produce non-finite output.
- Crash the worker.
- Hang long enough to trigger supervisor timeout.
- Optionally reject or mishandle a known parameter/state operation if that can be represented reliably.

Fault modes must not depend on random chance selecting a rare value. Tests must activate them through explicit fixture configuration.

## 7. Process supervision, replay, and reports

Run the plugin in a supervised subprocess so a target crash or hang does not terminate the coordinator. Use a narrow, versioned worker protocol and enforce a configurable timeout. Process isolation is crash containment, **not** a security sandbox. Do not claim malicious-binary isolation or minidump capture unless implemented and verified.

Replay must verify the target hash and stop on mismatch unless an explicit override is supplied and recorded. Replay the saved module, parameter snapshot, stimulus, sample rate, block setup, and dwell. Store the replay result separately from its source failure.

JSON and HTML results must include target provenance, engine/schema and module versions, profile and configuration, seeds, all module outcomes, requested and actual processed dwell, parameter and module coverage, unsupported capabilities, classifications, failure details, and artifact paths. The report should tell a developer which module failed, where it failed, how to reproduce it, and what evidence is available. Do not claim exhaustive test coverage or native code coverage.

## 8. Prior art, research, and code reuse

Before final dependency and reuse decisions, verify current upstream docs, exact revisions, licenses, and build status. Add concise findings and the implementation decision to `docs/RESEARCH_NOTES.md` or a focused M1 research addendum. At minimum assess:

- [Tracktion/pluginval](https://github.com/Tracktion/pluginval): test-level/strictness selection, modular test registration, parameter fuzz tests, headless CI, and subprocess isolation. Its repository identifies GPLv3; use as a behavioral/design reference or optional external executable unless license review approves another route.
- [free-audio/clap-validator](https://github.com/free-audio/clap-validator): CLAP compliance checks, deterministic/multiprocess fuzzing and replayable seeds. Its upstream license and dependency versions must be verified at the chosen revision. Prefer invoking it as an optional external validator or reimplementing a narrow clean-room contract over copying substantial source.
- [free-audio/clap](https://github.com/free-audio/clap): authoritative extension and host/plugin protocol details; use it to keep parameter and event handling legal.
- [dfl/clap-trap](https://github.com/dfl/clap-trap): investigate headless render, state, validation, and benchmark capabilities; verify maintenance and license before integration or reuse.
- [iPlug3/audio-plugin-dev-skills](https://github.com/iPlug3/audio-plugin-dev-skills): investigate the reusable validator-command/LLM-skill pattern as an example of making existing validators easy for coding agents to select and run.
- Established fuzz-testing methods: deterministic seeds, saved failing inputs, corpus reuse, boundary-biased valid sampling, stateful operation sequences, coverage-guided fuzzing for source-instrumented components, and failure minimization. Keep source-instrumented fuzzing distinct from the black-box CLAP campaign.
- Audio/DSP methods: deterministic fixtures, sample-rate and block-size variation, finite-value checks, channel-isolation checks, repeatability comparisons, callback deadline/performance checks where a meaningful measurement exists, and explicit tolerances/oracles to avoid false positives.

Identify which ideas are reused conceptually, which tools are invoked, and which code (if any) is copied. Do not copy an entire external codebase merely because it is useful. Verify compatibility and licenses before adding source or linked dependencies. The MIT-licensed CLAP headers and JSON dependency remain subject to exact-version verification and hash recording.

## 9. Dependencies and CI

Use the smallest practical dependency set:

- C++20, CMake, and Visual Studio 2022 x64 in Windows CI.
- Official CLAP headers, pinned to an exact upstream tag or commit after verifying it.
- `nlohmann/json` 3.12.0, or a newer version only if a fresh version check justifies updating the pin.
- CTest for tests.
- Do not add JUCE to M1.
- Treat `clap-validator` 0.4.1 as an optional external validation tool, not a linked dependency, unless fresh research justifies a documented change.
- Treat pluginval as an optional external tool. Do not link or copy its GPLv3 code into SoundGrind without an explicit licensing decision.

Record resolved dependency versions, licenses, and hashes. Recheck upstream sources before pinning:

- [CLAP](https://github.com/free-audio/clap)
- [clap-validator](https://github.com/free-audio/clap-validator)
- [nlohmann/json](https://github.com/nlohmann/json)
- [Tracktion pluginval](https://github.com/Tracktion/pluginval)

Add a Windows x64 GitHub Actions workflow that builds the CLI and fixture, runs tests, and uploads logs and reports. Crash and hang tests must run only in the worker process and clean up reliably.

## 10. Test requirements

Use the synthetic fixture for integration tests. Add tests for:

- Configuration bounds, invalid profiles, empty module selections, and cancellation/budget handling.
- Deterministic seed derivation, random selection, and valid domain/stepped-value sampling.
- Unique-state versus rounds-per-state accounting and exact planned case counts.
- Serialization/deserialization and schema versioning.
- CLAP capability discovery and protocol-correct parameter/event application.
- Complete snapshots in every generated case and explicit exclusion reporting.
- Profile composition and registry behavior, including `UNSUPPORTED` results.
- Offline processing, whole-block dwell rounding, and requested/actual sample accounting.
- Non-finite output and deterministic fixture-oracle mismatch.
- Worker crash and hang containment.
- Preservation of flushed pre-apply evidence and bounded retry behavior.
- Replay with target hash validation and separate result storage.
- JSON/HTML report contents, module coverage, and reproducibility instructions.

Do not require private or commercial plugins.

## 11. Documentation and module reuse

Add or update:

- `specs/M1_REUSABLE_CLAP_TESTING.md`
- `docs/DECISIONS.md` with concise architecture and CLAP-first/test-module ADRs
- `docs/DATA_CONTRACTS.md` with M1 schemas and representative cases/failures
- `docs/M1_VERIFICATION.md` with exact commands, dependency/tool versions, test results, CI run, and limitations
- `specs/INDEX.md` with accurate M1 status
- `README.md` with build, inspect, test-profile, replay, and report instructions
- An LLM-facing usage example that selects existing modules and fills configuration. The example must call SoundGrind code or CLI; it must not regenerate the test harness.

Document module identifiers, required capabilities, configuration fields and defaults, evidence/oracle semantics, deterministic replay requirements, and `UNSUPPORTED`/inconclusive outcomes. Make it easy to add a test module without modifying the CLAP host, campaign scheduler, or report renderer.

Mark M1 verified only after its Windows x64 build and acceptance tests pass.

## 12. M1 completion criteria

M1 is complete when a clean Windows x64 checkout can build the CLI and fixture, and tests demonstrate that:

- Existing test modules can be selected and configured without writing new test code.
- Level 1 and Level 2 run their applicable registered checks; Level 3 and both fuzz modes and Grind use the documented independent state/round/dwell dimensions.
- Every generated fuzz/grind case contains a complete valid snapshot for all enabled, supported, writable parameters.
- The campaign preview reports an accurate planned case count and bounded run budget.
- Requested dwell converts to a nonzero, documented sample count and reports actual whole-block processing.
- Non-finite output and fixture-oracle mismatch are detected.
- Worker crash and hang do not terminate the coordinator.
- Pre-apply evidence survives failures.
- Replay uses the recorded configuration and checks target hash.
- JSON and HTML reports contain actionable failure context, provenance, coverage, and capability limits.
- Windows CI passes and its results are recorded in `docs/M1_VERIFICATION.md`.

If a criterion cannot be met, continue troubleshooting. If a concrete dependency or protocol limitation blocks progress, record evidence and the smallest decision needed. Do not mark M1 complete based on planned work, unsupported claims, or test counts alone. Do not begin M2 or claim VST3, GUI, text-input, or standalone-application support.

## Handoff

Summarize changed files, reusable modules and how to invoke them, exact build/test commands, actual results, CI status, known limitations, research/reuse decisions, and whether M1 is verified.
