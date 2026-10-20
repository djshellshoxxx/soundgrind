# M1 verification record

**Status:** Verified for the documented M1 scope on 2026-10-20. A clean Windows x64 Visual Studio build and all CI tests passed. [Windows x64 workflow run](https://github.com/djshellshoxxx/soundgrind/actions/runs/39391823940).

## Local environment

- GCC 13.3.0, CMake 4.4.4, Ninja 1.13.2, Python 3.12.14
- CLAP 1.2.9 commit `cd94482ba5941ae410809b6fbaed3bc851044270`
- nlohmann/json 3.12.0 commit `65ee68451d8eb2b5f3a30b410476ab83deb3289b`
- Windows runner: `windows-latest`, Visual Studio x64, Python 3.12

## Commands and results

```sh
cmake -S . -B build -G Ninja
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
```

Result: **7/7 tests passed**. Tests cover engine config/profile contracts, fixture smoke processing, inspect and CLI integration, campaign previews and exact case counts, complete snapshots and round accounting, requested/actual sample rounding, replay snapshot fidelity and target-hash rejection, HTML report generation, and explicit NaN, crash, hang and oracle-mismatch faults. Fault evidence tests confirm pre-worker case records persist with the expected classification.

Windows x64 GitHub Actions run `39391823940` completed successfully on commit `f0a5b22f6b6feb92da6a50f428487755d0ee46d0`: configure, Visual Studio x64 build, all seven CTest cases, and evidence upload passed.

An additional manual run exercised 3 states × 2 rounds, 1 ms dwell at 48 kHz / 64-frame blocks, selected module composition, replay from a non-default random snapshot, and SHA-256 agreement with `sha256sum`.

## Scope limits

- External commercial plugins were not used; only the synthetic fixture was tested.
- CLAP process isolation is not a security sandbox. No untrusted-binary containment claim is made.
- M1 has no GUI, text-input, VST3, physical audio, DAW, minidump or source-code-coverage adapter.
- The normalized/plain mapping check validates SoundGrind's linear mapping; CLAP base parameter metadata does not expose a generic logarithmic scale descriptor.
- `fuzz2` chooses a deterministic random subset of built-in modules. Cancellation, wall-clock campaign budgets, state minimization and adaptive failure-guided mutation are not implemented in M1.
