# SoundGrind subsystem specification register

Updated 2026-10-20. **Draft** means initial prose exists, not an approved implementation contract. M1 is verified for the documented CLAP CLI scope; future subsystem specs remain drafts or planned.

| Order | Spec file | Scope | Status | Dependency |
| --- | --- | --- | --- | --- |
| 00 | M1_REUSABLE_CLAP_TESTING.md | First reusable CLAP CLI, offline worker, tiered modules, replay and reports | Verified on Windows x64 and Linux | C++20, CLAP, worker protocol |
| 01 | ULTIMATE_FUZZ_STRESS.md | Flagship complete-state randomization, 1-second dwell, text mutation, loops, replay | Draft written | Shared schemas, scheduler, worker and host contract |
| 02 | TARGET_DISCOVERY.md | Plugin/app scan, identities, parameter inventory, capability and input maps | Planned | None |
| 03 | HOST_ADAPTERS.md | Format-aware VST3 and CLAP host workers, lifecycle and callback/thread legality | Planned | Discovery, process protocol |
| 04 | STANDALONE_APP_ADAPTER.md | Launch, supervision, visible UI and optional app-specific test hooks | Planned | Worker, control map, UI |
| 05 | AUDIO_DSP_TEST_ENGINE.md | Stimuli, render, audio metrics, expected-oracle fixtures, thresholds | Planned | Host adapters, schema |
| 06 | MIDI_TRANSPORT_AUTOMATION.md | Note/events, transport changes, parameter ramps and timing | Planned | Host, audio |
| 07 | GUI_AUTOMATION.md | UIA/custom controls, mappings, screenshots, accessibility and interactions | Planned | Standalone adapter, discovery |
| 08 | TEXT_FUZZING.md | Text discovery, length/charset profiles, transport, replay and constraints | Planned | GUI/bridge, fuzz |
| 09 | CORE_SCHEDULER_AND_PROTOCOL.md | Campaign plan, versioned IPC, jobs, pause/resume, thread/process isolation | Planned | Data contracts |
| 10 | MONITORING_AND_WATCHDOG.md | Hangs, crashes, minidumps, memory/CPU, heartbeat and failure attribution | Planned | Core scheduler |
| 11 | RESULTS_AND_REPRODUCTION.md | Durability, artifacts, storage/retention, dedup, replay/shrinking | Planned | Protocol, fuzz |
| 12 | VALIDATOR_INTEGRATIONS.md | pluginval, clap-validator, SDK and auval external tool adapters | Planned | Discovery, results |
| 13 | CI_AND_HEADLESS.md | CLI, GitHub Actions, test policies, artifacts and fail gates | Planned | Worker, results |
| 14 | DESKTOP_UX.md | Navigation, targets, suite presets, campaign controls and report UX | Planned | Protocol, results |
| 15 | CROSS_PLATFORM_SUPPORT.md | macOS/Linux adapters, packaging, platform CI and support matrix | Planned | Windows MVP architecture |
| 16 | SOURCE_INSTRUMENTED_FUZZING.md | Sanitizer/AFL++/coverage harnesses for format, parser and state code | Planned | Replay/artifacts |
| 17 | PERFORMANCE_BENCHMARKING.md | RT scheduling, callback deadlines, isolation overhead, memory/leak testing | Planned | Host and audio |
| 18 | SECURITY_AND_ISOLATION.md | Threat model, permissions, filesystem sandbox, network/device safety | Planned | Core architecture |
| 19 | SYNTHETIC_FIXTURES.md | Intentionally faulty plugin/app fixtures and expected diagnostics | Planned | Adapter contract |

Each planned spec is to be written as a **separate document** when its implementation is approached, not as a single huge file. The master prompt in prompts/MASTER_DESIGN_PROMPT.md requires a complete list and necessary research before implementation.

## Mandatory spec template

Each new spec MUST state:
1. ID, current revision, author/source and status: proposed, reviewed, approved, implementing, implemented, verified.
2. Problem, persona, use cases, scope and non-goals.
3. Capability/platform/format matrix, prior art and licenses.
4. Modular design, process boundaries and typed public interface contract.
5. Data, events, state machines, concurrency and time semantics.
6. Error states, recovery, observability and artifacts.
7. User-facing interactions and accessibility if applicable.
8. Unit, fixture, integration and end-to-end tests; exact expected outputs.
9. Acceptance gates, regressions, performance criteria, explicit UNSUPPORTED/INCONCLUSIVE conditions.
10. Dependencies, ADRs, rollout/migration, known unknowns and next tasks.

## Proposed implementation order

Design/research and contracts -> mock target and deterministic sampler -> isolated worker and write-ahead results -> first host adapter and minimal audio signal analysis -> CLI/replay -> desktop UI -> second plugin adapter -> text/GUI adapters -> extended DSP/CI/multi-OS.

Start with a small working test path, retain success after each milestone, and add features as separate isolated modules. Do not allow speculative features to destabilize functioning build/release pipelines.

## Handoff

Design baseline: main, commit `e661ef0` (2026-10-19). M1 implementation branch: `feat/m1-clap-test-engine`.
Root docs:
- README.md — project status and entry points
- prompts/MASTER_DESIGN_PROMPT.md — authoritative reusable research/design metaprompt
- docs/PRODUCT_REQUIREMENTS.md — initial requirements and measurable gates
- docs/RESEARCH_NOTES.md — sources, differentiators, license caveats
- specs/ULTIMATE_FUZZ_STRESS.md — flagship initial technical draft

Current status: M1 Windows x64 CI and fixture acceptance tests passed. The next subsystem remains outside this M1 implementation scope.
