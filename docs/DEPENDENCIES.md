# M1 dependency and reuse record

Verified 2026-10-20 against upstream repository metadata, tags, source licenses and current default-branch commits.

## Build dependencies

| Project | Pin used / inspected | License | M1 decision |
| --- | --- | --- | --- |
| [free-audio/clap](https://github.com/free-audio/clap) | 1.2.9, commit `cd94482ba5941ae410809b6fbaed3bc851044270` | MIT | Fetch exact commit in CMake; official API headers only |
| [nlohmann/json](https://github.com/nlohmann/json) | 3.12.0, commit `65ee68451d8eb2b5f3a30b410476ab83deb3289b` | MIT | Fetch exact commit in CMake |
| [free-audio/clap-validator](https://github.com/free-audio/clap-validator) | 0.4.1, commit `152b9823e992d782c5c1fd33bca0295478b919aa` | MIT | Optional external tool; not linked or required in M1 |
| [Tracktion/pluginval](https://github.com/Tracktion/pluginval) | Current repository inspected 2026-10-20 | GPL-3.0 | Design reference or user-installed external tool only; no source copied or linked |

The CMake pins are Git commit hashes. Upstream repository metadata reports the CLAP and JSON repositories as MIT and pluginval as GPL-3.0. `LICENSE` files were checked at the pinned CLAP and JSON commits. No third-party implementation source is copied into SoundGrind.

## Prior art and implementation decisions

- **pluginval:** study test-depth selection, strictness and headless plugin validation. The GPL-3.0 license makes an optional separately installed executable a safer integration boundary; M1 does not invoke it.
- **clap-validator:** study deterministic validation and replay. It is MIT-licensed and current 0.4.1 was inspected, but it remains optional and unlinked so SoundGrind's worker/evidence contracts stay independently testable.
- **CLAP:** authoritative lifecycle, parameter events, audio ports and state contract. M1 sends parameter changes as timestamped CLAP events during process and uses `params.flush` only while inactive.
- **clap-trap:** [MIT-licensed headless CLAP integration host](https://github.com/dfl/clap-trap), inspected at current `main` commit `c75d353dc57140cfceebf96dcbea9c491bef4f10` (last pushed 2026-01-24 per upstream metadata). Studied as a precedent; no source copied or runtime dependency introduced.
- **iPlug3/audio-plugin-dev-skills:** [MIT-licensed LLM audio-plugin skill collection](https://github.com/iPlug3/audio-plugin-dev-skills), inspected at current `main` commit `e593a808a6fc97d0f13a6fd2e91e61fcf2e5a314`. Reused conceptually: an LLM selects existing documented capabilities instead of generating test code. No source copied.
- **Fuzz/audio practice:** use recorded seeds, complete valid snapshots, explicit stepped domains, bounded cases, deterministic fixture oracles, finite-output scanning, whole-block sample accounting and replayable case files. M1 is black-box parameter-state testing; it does not report native code coverage or use coverage-guided fuzzing.

## Upstream references

- https://github.com/free-audio/clap/releases/tag/1.2.9
- https://github.com/nlohmann/json/releases/tag/v3.12.0
- https://github.com/free-audio/clap-validator/releases/tag/0.4.1
- https://github.com/Tracktion/pluginval
- https://github.com/dfl/clap-trap
- https://github.com/iPlug3/audio-plugin-dev-skills
