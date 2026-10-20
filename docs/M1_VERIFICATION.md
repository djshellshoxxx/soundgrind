# M1 verification record

**Status:** Not verified for release. Local Linux build and integration checks passed on 2026-10-20. The acceptance target is a clean Windows x64 Visual Studio build in `.github/workflows/windows-m1.yml`; record its run URL and result here after CI completes.

## Local environment

- GCC 13.3.0, CMake 4.4.4, Ninja 1.13.2, Python 3.12.14
- CLAP 1.2.9 commit `cd94482ba5941ae410809b6fbaed3bc851044270`
- nlohmann/json 3.12.0 commit `65ee68451d8eb2b5f3a30b410476ab83deb3289b`

## Commands and results

```sh
cmake -S . -B build -G Ninja
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
```

Result: **7/7 tests passed**. Tests cover engine config/profile contracts, fixture smoke processing, inspect and CLI integration, campaign previews and exact case counts, complete snapshots and round accounting, requested/actual sample rounding, replay snapshot fidelity and target-hash rejection, HTML report generation, and explicit NaN, crash, hang and oracle-mismatch faults. Fault evidence tests confirm pre-worker case records persist with the expected classification.

An additional manual run exercised 3 states × 2 rounds, 1 ms dwell at 48 kHz / 64-frame blocks, selected module composition, replay from a non-default random snapshot, and SHA-256 agreement with `sha256sum`.

## Pending Windows gate and limitations

- Windows x64 build and CTest workflow have not yet completed in this record.
- External commercial plugins were not used; only the synthetic fixture was tested.
- CLAP process isolation is not a security sandbox. No untrusted-binary containment claim is made.
- M1 has no GUI, text-input, VST3, physical audio, DAW, minidump or source-code-coverage adapter.
- The normalized/plain mapping check validates SoundGrind's linear mapping; CLAP base parameter metadata does not expose a generic logarithmic scale descriptor.
- `fuzz2` chooses a deterministic random subset of built-in modules. Cancellation, wall-clock campaign budgets, state minimization and adaptive failure-guided mutation are not implemented in M1.
