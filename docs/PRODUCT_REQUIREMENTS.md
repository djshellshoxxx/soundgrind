# SoundGrind — Initial Product Requirements Baseline

Status: DRAFT v0.1 / 2026-10-19. These are starting requirements for the master design process, not completed implementation contracts.
Authority: prompts/MASTER_DESIGN_PROMPT.md.
Goal: robust quality assurance for developer-owned audio plugins and standalone audio software.

## Problem and differentiation

Developers typically use multiple tools to test plugin format validity, audio output, user interface operation, performance and reliability. No single generic interface guarantees access to arbitrary custom-drawn GUI controls, standalones, audio-thread data, and every host format. SoundGrind will unify **supported** testing workflows and make findings reproducible rather than claim invisible test coverage.

SoundGrind differentiators:
- full-settings randomized experiments repeated at a configurable post-apply dwell time;
- parameter-domain-aware sampling plus text input fuzz where accessible;
- native audio and MIDI signal generators and diagnostic oracles;
- GUI test discovery with explicit mapping coverage;
- supervised worker processes and persistence of the last attempted action before a crash;
- one-click replay, crash minimization, triage and export;
- adapters for plugin formats, standalone apps, validators, DAWs and test instrumentation;
- local desktop app and identical headless CLI/CI engine.

## Phased platform target

MVP: Windows x64 with VST3 and CLAP plugins, one reproducible headless case loop, fixtures, crash logs, JSON/HTML reports, CLI and basic desktop UI. Host API use must conform to each format and honor threading. Integration with external validators may come before native validation logic.

Phase 2: more GUI mappings, text fuzzing, preset/state and audio/MIDI test matrix, longer soak, CI, screenshot baselines. Standalone app testing is first meaningful on Windows via supervised process and UI Automation but requires control mapping; GUI-only targets will offer more limited assertions.

Phase 3: macOS arm64 and x86_64, Audio Units, Linux x86_64 and LV2 when tested platform workers exist; real DAW integration and advanced profiling. Cross-platform UX and storage code should be designed now, tested separately.

Explicit non-goals for MVP: asserting exhaustive input coverage; deep black-box code coverage; arbitrary third-party applications with no compatible adapters; remote execution; in-process hostless tests of every VST3/CLAP lifecycle scenario; automatic distribution of copyrighted third-party plugins; unattended invasive changes to OS or production user workspaces.

## Functional requirements (priority codes: P0 launch; P1 early; P2 future)

| ID | P | Requirement | Verifiable acceptance |
| --- | --- | --- | --- |
| SG-001 | P0 | Inspect local VST3/CLAP binaries in isolated process | Valid synthetic targets produce descriptors; malformed target is reported and does not crash coordinator |
| SG-002 | P0 | Enumerate writable exposed parameters and boundaries | Stable IDs, types, legal values, defaults and missing metadata recorded |
| SG-003 | P0 | Run every-enabled-parameter random full-state cases | Each case records exactly one attempted value per included writable parameter, plus skipped reason for omissions |
| SG-004 | P0 | Default evaluation dwell of 1000 ms | Fresh campaign defaults to 1000 ms; user override appears in case manifest and execution |
| SG-005 | P0 | Repeat until stopped or bounded | Count limit, time budget, manual stop and resume/checkpoint function |
| SG-006 | P0 | Deterministic RNG/replay | Fixed target+schema+fixture+runner versions and case seed give same intended actions/samples; nondeterministic observed output is labeled |
| SG-007 | P0 | Contain child crash and detect hang | Crash/hang fixture cannot kill main UI/coordinator; case journal retained |
| SG-008 | P0 | Monitor obvious audio invalidity | NaN/Inf and non-finite output, obvious zero/silent anomaly when oracle warrants, invalid channel count and process errors recorded |
| SG-009 | P0 | Generate useful failure artifact | User can export exact seed, state snapshot, binary hash, fixture, environment, logs and replay recipe |
| SG-010 | P0 | CLI and basic desktop setup | Local user can select target, profile, start/stop and review failure without building scripts |
| SG-011 | P0 | Make unsupported test coverage visible | GUI text unsupported/no map appears as NOT TESTED, never PASS |
| SG-012 | P0 | Integration test synthetic failure fixtures | Known crash, hang, NaN and zero-output fixture detected correctly with no false claim |
| SG-013 | P1 | Fuzz accessible text fields | Configurable length/encoding profiles, complete journal of generated input and field ID |
| SG-014 | P1 | GUI control discovery and execution | UIA/custom bridge works for instrumented synthetic application and reports unmapped fields |
| SG-015 | P1 | Boundary, pairwise, mutation strategies | Every strategy is selectable, seeded, replayable and has strategy-specific coverage |
| SG-016 | P1 | Audio and MIDI suites | Fixture generator and saved fixture hashes support stress and regression |
| SG-017 | P1 | Additional validator orchestration | Imported validator results cite exact tool/version/executable and are distinguished from native SoundGrind results |
| SG-018 | P1 | Failure shrinking | A seeded, repeatable failing case can be reduced while preserving untouched original |
| SG-019 | P1 | Settings serialization/restore tests | Save/load/preset roundtrip changes and checksum evidence retained |
| SG-020 | P1 | Build comparisons and issue dedup | Same issue across binary revisions shows status and unique signature |
| SG-021 | P1 | CI headless export | JSON/JUnit XML/HTML; configurable thresholds cause nonzero exit |
| SG-022 | P2 | macOS and Linux native worker parity | Per-platform format tests gate their own release, unsupported capabilities documented |
| SG-023 | P2 | Source-level coverage fuzzing | Explicit instrumentation profile and real code coverage vs black-box parameter coverage |
| SG-024 | P2 | Real DAW integration and automation | Capability-checked vendor-specific adapter, no generic DAW assumption |

## Flagship exact test procedure

Preflight: identify target hash, compatible architecture/format, supported capabilities, input fixture, output monitor, parameter schema, control method and safety profile. The user selects complete-state fuzzing, default 1000-ms dwell, chosen seed, optional audio sample rate and block size, text profiles and stop budget.

For each case:
1. Allocate monotonically increasing case sequence number. Derive per-case and per-domain seeds from a versioned master seed generator.
2. Generate one complete candidate state for every enabled supported writable parameter, respecting type-specific legal domains and known dependencies. If one control is excluded or unavailable, include a recorded reason.
3. Generate optional independent text inputs for each supported visible text field; bound default lengths to known field limits and enable oversized cases only in an opt-in adversarial profile.
4. Save a pre-apply write-ahead case manifest sufficient to reconstruct all intended actions if the worker crashes immediately.
5. Apply parameter changes legally for the format/thread; optionally settle/ramp; verify effective values by readback where supported. Distinguish attempted from confirmed effective values.
6. Run chosen deterministic audio/MIDI/transport fixture for 1000 ms processed-audio time in offline plugin mode (or 1000 ms real wall time in live app mode), with actual elapsed/wall sample count recorded. Bound startup, load and shutdown separately.
7. Inspect health, finite samples, anomalies, application/UI state and performance. Put raw measurements next to any tolerance-based verdict.
8. Commit outcome and per-case artifacts, checkpoint state and deduplicate failures. On crashed worker, supervisor recovers the journal and adds OS diagnostic results before restarting clean worker.
9. Optionally reinitialize target before next iteration; otherwise persist state according to the chosen mode. Repeat.

A **million randomized trials** remains a sample, not a proof of complete parameter-domain coverage.

## Text fuzz profile examples

Standard: random length 0–256 or discovery-derived maximum, printable ASCII/Unicode, whitespace and localized strings.

Boundary: 0,1,2,maximum-1,maximum,maximum+1 where input API can accept it; near reasonable format limits; long repeated strings.

Unicode: combining sequences, non-BMP symbols, surrogate handling appropriate to API, bidirectional text, normalization, multiline and extreme grapheme complexity.

File/preset text: paths with separators, spaces, Unicode, reserved filenames, different extensions but only within disposable working folders.

Malformed: null bytes and control characters only if input transport allows, never pretend application UI can type embedded zero characters through an ordinary text control.

Each text test preserves exact byte sequence or code point sequence, chosen encoding, target field locator, input injection method and any auto-truncation/readback.

## Error taxonomy

Outcome values: PASS, FAIL, WARNING, SKIPPED, UNSUPPORTED, INCONCLUSIVE, INFRASTRUCTURE_ERROR, CANCELLED.

Failure kinds: crash, startup_failure, hang, timeout, invalid_audio, numeric_instability, underrun, resource_growth, state_corruption, gui_assertion, rendering_diff, protocol_violation, exception, sanitizer_report, harness_failure.

Separate: observed facts, heuristic suspicions, and proven root cause. Severity and confidence are independent. A missing UI screenshot must not conceal a known crash.

## Suggested data records

- TargetManifest: path, SHA-256, host type, format, arch, version, compatibility, discovery results.
- ParameterDescriptor: stable ID/name, kind, legal min/max/step, default, unit, logarithmic flag, readable/writable flag, dependency, adapter origin and mapping confidence.
- TextFieldDescriptor: stable locator, kind, max length if known, access method, encoding, writable/readable and constraints.
- TestCase: campaign, monotonically increasing index, seed(s), generator version, target hash, scope, full snapshot, timeline, dwell and fixture hashes.
- TestOutcome: lifecycle timestamps, actual audio samples and wall duration, health/metrics, verdict, error kind, failure fingerprint and artifact links.
- ReproductionRecipe: supported environment and versions, exact settings/action order, command, verification status and known nondeterministic dependencies.

## Performance, resources, and operational UX

Metrics must distinguish target CPU from runner CPU, real-time processing from offline render, rolling averages from max render deadline, and device xruns from simulated buffer misses. Start with lightweight short cases; avoid expensive screenshots or large dumps every iteration. Keep the most recent failure candidate and journal flush durable, but batch routine metrics. Allow bounded local file retention.

Desktop UX: target selector, suite selector, profile explanation, obvious 1.0-second default field, included vs excluded parameter count, run/pause/stop, live case snapshot, anomalies and reports. Headless CLI uses the same test-plan schema and reports the same verdicts. App must offer keyboard operation and readable progress.

## Design and release gates

Gate D1 — researched capability matrix and license decisions.
Gate D2 — versioned target, case and result schemas.
Gate D3 — deterministic seeded core and synthetic mocks demonstrate correct case construction.
Gate D4 — isolated worker can be killed and coordinator survives with intact pre-apply case manifest.
Gate D5 — correct host-specific plugin lifecycle for initial supported format.
Gate D6 — fixed fixture + default 1s and override work, with meaningful audio and health oracles.
Gate D7 — reports and replay validated against planted defects.
Gate D8 — functional Windows CLI and basic desktop binary with smoke/regression passing.
Gate D9 — user-guide, no false coverage claims, CI tests and license notices complete.

## Open design questions

- Final primary language/hosting architecture and GUI toolkit.
- How much plugin/host metadata is exposed across each target format.
- How to handle partial/invalid parameter application and dependency interactions deterministically.
- Whether default continuous campaign runs for N cases or until user stops (UI should supply an explicit safe default budget).
- Which synthetic fixtures best characterize processor classes without false positives.
- Which text fields require explicit hooks versus UIA and how much control mapping can be automated.
- What exact workstation resource protection profile is default.
- How to compare benchmark results across different host adapters and render modes fairly.

All questions above require ADRs, not arbitrary undocumented assumptions.
