# SoundGrind

SoundGrind is a C++20 command-line test engine for exercising CLAP audio plugins during development. Its first implementation milestone (M1) provides reusable test modules, offline rendering, supervised workers, reproducible parameter snapshots, replay, JSON evidence and a self-contained HTML report.

**Status:** M1 implementation is in progress. Linux build and fixture tests pass. Windows x64 CI is the required verification gate; check the workflow badge/run before treating M1 as verified.

M1 supports CLAP plugins only. It does not open audio devices, run GUI or text-input tests, host VST3, or sandbox untrusted binaries. A worker process contains ordinary target crashes and hangs; it is not a security boundary.

## Build

Requirements: CMake 3.24+, C++20 compiler, Git and Python 3. CMake fetches the pinned official CLAP headers and `nlohmann/json` sources during configure.

```sh
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

Visual Studio x64 build:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

## CLI examples

```sh
# Inspect plugin identity, CLAP capabilities, parameters and audio ports
build/soundgrind inspect --target ./Plugin.clap

# Quick smoke profile (three independent cases; 1,000 ms dwell by default)
build/soundgrind test --target ./Plugin.clap --profile level1 --out results

# Run the feature and boundary profile with a small bounded campaign
build/soundgrind test --target ./Plugin.clap --profile level2 \
  --states 3 --rounds 2 --dwell-ms 1000 --seed 4242 --max-cases 6 --out results

# Select existing modules directly. The modules are already implemented in SoundGrind.
build/soundgrind test --target ./Plugin.clap --modules fixture.oracle,params.readback \
  --profile level1 --seed 42 --out selected-results

# Preview planned case count and capabilities without launching test cases
build/soundgrind test --target ./Plugin.clap --profile level3 \
  --states 30 --rounds 5 --max-cases 150 --preview --out preview

# Run seeded valid-state fuzzing; each state has a complete writable-parameter snapshot
build/soundgrind test --target ./Plugin.clap --profile fuzz1 \
  --states 20 --rounds 2 --dwell-ms 1000 --seed 42 --max-cases 40 --out fuzz-results

# Grind uses randomized complete snapshots and per-case dwell from the bounded interval
build/soundgrind test --target ./Plugin.clap --profile grind \
  --states 100 --rounds 1 --dwell-min-ms 1 --dwell-max-ms 5000 \
  --seed 42 --max-cases 100 --out grind-results

# Replay a saved case into a separate results directory
build/soundgrind replay --target ./Plugin.clap --case results/cases/CASE_ID.json \
  --out replay-results

# Render HTML from saved campaign JSON
build/soundgrind report --input results/campaign.json --out results/report.html
```

For Windows, use `soundgrind.exe`; it locates `soundgrind_worker.exe` beside itself unless `--worker` is supplied. Paths may contain spaces.

## Profiles and reusable modules

`level1`, `level2`, and `level3` select named sets of registered modules. `fuzz1` uses caller-selected modules and randomized complete valid states. `fuzz2` deterministically selects a subset of the built-in test modules from the supplied seed. `grind` randomizes every enabled writable parameter for each unique state and chooses dwell independently from `--dwell-min-ms`/`--dwell-max-ms` (default 1–5,000 ms).

The dimensions are independent: planned cases equal `unique states × rounds per state`; selected modules all execute within each case. `--preview` writes the exact case count and target inventory. Dwell means processed audio samples, not sleeping: `ceil(dwell_ms × sample_rate / 1000)` is rounded up again to a whole configured block. The fixture's 1 ms at 48 kHz with a 64-frame block processes 64 samples (about 1.333 ms).

See [the M1 contract](specs/M1_REUSABLE_CLAP_TESTING.md), [module integration example](examples/llm-module-selection.md), [data contracts](docs/DATA_CONTRACTS.md), [decisions](docs/DECISIONS.md), [dependency pins and research](docs/DEPENDENCIES.md), and [verification record](docs/M1_VERIFICATION.md).
