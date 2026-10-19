# SoundGrind — Master Research, Architecture, and Specification Prompt

Version: 0.1-design
Date: 2026-10-19
Destination repository: https://github.com/djshellshoxxx/soundgrind
Status: authoritative **design instruction**, not permission to implement a product.

## Role and mission

Act as a principal audio-software engineer, plugin-host engineer, real-time DSP test engineer, software quality architect, fuzzing specialist, desktop GUI automation engineer, and technical product designer. Design SoundGrind as a practical, extensible, cross-platform platform for testing **audio plugins AND standalone audio applications that are under active development**. Produce a **developer-ready product and technical specification**, a prioritized delivery plan, and a set of separately maintained implementation specifications.

The signature workflow is **Ultimate Fuzz Stress**: discover every safely controllable parameter exposed by a target, construct a new independently randomized or correlated complete settings snapshot, apply the snapshot through a format-aware host, app-specific control bridge, or GUI automation adapter, run the target with configured stimulus for a user-selectable dwell time (default **1 second**), monitor and persist all evidence and failures, and repeat. Randomly generated valid combinations are useful, but **do not claim to enumerate every possible combination**, especially with floating-point domains and text strings. Support indefinitely repeating campaigns plus user-defined run counts/time budgets. Any discovered editable text input must support random-length random-string generation with selectable text profiles.

This master prompt asks for a DESIGN FIRST. Do not silently begin writing the SoundGrind application. Additional specifications are required for each independently implementable subsystem before development begins. Preserve existing files/commit history. Read existing AGENTS.md, CLAUDE.md, README.md, specification index, and design decisions if present before editing. Never overwrite work from another branch without comparison.

## Non-negotiable product goals

1. Make it simple for developers to pick a VST3/CLAP plugin or standalone application and start meaningful automated tests with minimal setup.
2. Prioritize **Windows x64** as the first fully usable platform; plan macOS arm64/x86_64 and Linux x86_64 using platform-specific adapters without pretending portability is free.
3. Support VST3 and CLAP as first-class plugin formats initially. Describe AU/AUv3 (macOS), LV2 (Linux), standalone executables, and additional formats as phased support. Do not promise that one generic host can load all formats.
4. Support GUI-only black-box testing, plugin-native parameter tests, and instrumented white-box testing, each with explicitly different observability and guarantees.
5. Always distinguish test actions the adapter supports from ones SoundGrind merely hopes to support. For example, a plugin's host-visible parameters typically do not expose every text field or custom-drawn control.
6. Never allow a crashing plugin or application to take down the SoundGrind coordinator. Run subjects in supervised worker processes, and preserve test intent and evidence across a hard crash.
7. Make tests deterministic where technically possible, capture all random seeds and events, allow one-click replay, and automatically simplify a failing case into a minimal reproduction.
8. Make validation conservative: zero crashes does not establish correctness, stochastic test counts do not establish exhaustive coverage, and visual test failures must be distinguished from genuine functional failures.
9. Every module must have a stable contract, independent tests, documentation, and a clear owner. Favor low-risk incremental, reusable designs, with explicit portability and licensing decisions.

## Research responsibilities

Before locking architecture, perform targeted primary-source research and maintain a dated, linked research ledger. Compare at least:
- Tracktion pluginval: plugin discovery, validation, subprocess execution, GUI/headless tests, validation strictness, existing compatibility; investigate its license before code reuse.
- free-audio clap-validator: CLAP validation, fuzzer design, reproducible seeds, isolation, timing/parameter/threading tests.
- Steinberg VST3 SDK validator/test host: compliance, lifecycle, state, event and bus handling.
- Apple's AU validation tooling for macOS if applicable.
- JUCE plugin hosting and GUI lifetime rules, CLAP host/library implementations, and other well-maintained host backends; document format and license limitations.
- AFL++, LLVM's libFuzzer, AddressSanitizer/UndefinedBehaviorSanitizer, coverage-guided and stateful fuzzing, and how instrumentation differs from black-box campaigns.
- Official Windows UI Automation documentation; desktop UI testing options for Qt/JUCE custom rendering and macOS/Linux accessibility APIs.
- Established numerical DSP metrics, signal-processing test methods, performance profiling and real-time deadline analysis.
- Test-result conventions such as JUnit XML, JSONL event streams, machine-readable JSON, and artifact provenance.
- Relevant licensing for SDKs and third-party test tools; linking, redistribution, process-level invocation and attribution all require analysis.

For each source record URL, upstream revision/version/date, supported capabilities, license if confirmed, whether reuse is legally and technically appropriate, gaps relative to SoundGrind, risks, and a build-vs-integrate recommendation. Avoid copying source code into this planning repository. Mark unsupported claims as assumptions.

## Core use cases

A. Developer chooses a locally built plugin, SoundGrind scans metadata, shows editable parameters, runs baseline validation plus stress suites, and generates an HTML report and machine-readable artifacts.

B. Developer chooses a standalone Windows audio app, provides an executable and safe working directory, and uses accessibility inspection or a manually authored control map to identify UI controls and program settings. Tests run in an isolated desktop session and report observable behavior; do not imply internal audio or parameter introspection when none exists.

C. Developer launches **Ultimate Fuzz Stress** using the default 1-second dwell, an explicit seed, and the default valid-domain mode; every iteration applies a complete supported parameter snapshot, plays audio/MIDI stimuli, checks host and child process, records anomalies, and continues until stopped or budget exhausted. Inapplicable/unmapped controls are reported as uncovered, not counted as fuzzed.

D. Developer opts into text fuzzing: parameter text fields, preset names, filenames, search boxes, text edits, and other discoverable fields. The suite varies length and character class and captures unexpected exceptions, hangs, corruption or UI inconsistencies.

E. Developer selects a failed run and replays the exact parameter snapshot, sequence of changes, seed, audio/MIDI input, plugin binary hash and host configuration. SoundGrind compares repeated runs and reports reproducibility or nondeterminism.

F. Developer runs a GitHub Actions or local CLI job that enforces chosen test gates and uploads XML/JSON/HTML and reproducible crash artifacts without requiring a graphical desktop for headless suites.

## Required testing taxonomy

Specify a test matrix with each row showing objective, stimulus, oracle, data source, prerequisites, supported target/format/OS, execution cost, expected artifacts and false-positive modes.

1. **Discovery and compliance:** scanning, metadata, IDs, parameter bounds/defaults, buses, channels, plugin lifecycle, initialization/shutdown, load/unload/reload, state serialization, presets and restoration, duplicate/invalid parameter IDs, host-thread obligations; delegate to authoritative external validators where sensible.
2. **DSP correctness and behavior:** silence in/out, impulse response, step/DC, sine sweeps, multitone, pink/white noise, transient signals, clipped inputs, denormals, zero/near-zero amplitudes, sample-rate and block-size matrices, MIDI note sequences, pitch/control bend, stuck-note detection, modulation, gain/phase, peak/true-peak, RMS/LUFS where appropriate, NaN/Inf output, out-of-range sample values, channel isolation, latency compensation, tail length, determinism, discontinuity/click detection, excessive aliasing, THD/THD+N or spectral leakage when a valid measurement fixture exists. Use **type-specific oracles**: instrument/synth, effect, analyzer and dynamics processor cannot share arbitrary universal threshold assumptions.
3. **Parameter and state regression:** min/max/default, just-inside and just-outside invalid domains via supported API paths, boundaries, logarithmic scaling, enum steps, numeric precision, discrete rounding, linked controls, order of operations, automation ramps, high-frequency changes, save/reload/preset roundtrip, mute/bypass transitions, stereo and sidechain changes, format-specific thread and timestamp semantics.
4. **GUI functional, rendering, and accessibility:** opening/closing editor, resizing, DPI scaling, high contrast, light/dark UI when supported, keyboard, focus, tab order, hovering, knob drag/wheel, sliders, dropdowns, modal dialogs, tooltips, text typing and paste, scrolling, drag/drop, screenshot comparisons with user-approved baselines and tolerance masks, UI-to-DSP sync, DSP-to-UI sync, custom-drawn JUCE controls, multiple editor windows, repeated create/destroy, invalid coordinates. Prefer semantic accessibility locators, then app-provided automation bridge, then calibrated image/coordinate fallback marked fragile. No arbitrary clicking outside target window.
5. **Fuzzing:** valid domain full-snapshot randomization; boundary-biased sampling; pairwise/t-wise combination coverage; mutation of interesting snapshots; time-varying and asynchronous parameter changes; MIDI/event/audio permutations; file, preset and state-blob fuzzing in source-enabled harnesses; text-field fuzzing; lifecycle and GUI event sequencing; negative tests limited to well-defined supported APIs, not undefined host behavior by default.
6. **Performance/resource reliability:** steady-state CPU, max/worst-case render time relative to deadlines, buffer underrun/xrun, allocation on audio thread when instrumented, memory growth, handle/thread growth, open/close cycles, load time, plugin scan time, render throughput, background CPU, energy if supported, cold vs warm starts, long-run soak, multi-instance load.
7. **Compatibility:** relevant combinations of OS/architecture, plugin format, in-tree test hosts, actual DAWs when installed and licensed, block sizes 1/16/64/128/256/512/1024/2048/variable, sample rates 44.1/48/88.2/96/192 kHz subject to target support, float32/float64 paths where available, mono/stereo/multichannel, off-line faster-than-real-time versus live callback, transport changes, host suspend/resume, automation threading and latency reporting.
8. **Failure and recovery:** process crash, forced termination, hung UI/audio thread, failed startup, corrupt preset, invalid stored state, unavailable audio devices, device hot-plug (advanced), missing resource, cancelled run, interrupted run, resume/checkpoint, crash-loop prevention, disk/log quota exhaustion, loss of automation mapping.

Distinguish simulated audio/render processing from a real device test. No audible output by default; if monitoring real devices is enabled, use a protected virtual output/muted path and peak limits to avoid unexpected high-level sound.

## Ultimate Fuzz Stress: detailed required specification

Treat this as SoundGrind's flagship, with a **separate normative subsystem specification** after the product-level design.

**Default behavior:** one iteration = (1) determine iteration seed, (2) get complete target state and schema, (3) choose random values for **every writable, mapped, supported parameter** (unless explicitly excluded for safety or dependencies), (4) apply the complete state using a protocol-correct schedule with controlled ramp/flush/settling behavior, (5) process configured audio/MIDI or run app for **1000 ms** by default, (6) inspect health, audio and GUI oracles, (7) durably commit snapshot plus outcome, (8) optionally checkpoint and reset/reinitialize, (9) next iteration. The dwell value must be a user-editable duration in milliseconds/seconds; distinguish wall-clock duration for apps from audio-processed duration for offline plugins. Record actual elapsed time, sample count and underruns. Dwell applies to post-application evaluation, not necessarily to expensive setup. Offer explicit apply/settle/dwell/teardown phase budgets.

**Sampling strategies:** uniform legal numeric ranges; logarithmic/perceptual distribution where warranted; enumerated/discrete valid values; boundaries and near-boundaries; canonical named presets; biased corner cases; pairwise/t-wise covering combinations; mutation from crash-adjacent or coverage-novel states; optional domain-aware correlations. Define probabilities/config, ensure every included parameter is assigned and record exceptions. Do not accidentally write NaN/Inf as valid parameter values; treat malformed values as a different isolated adversarial API test, and never send values through APIs that expressly forbid them.

**Sequencing modes:** default full-state snapshots after optional reset; persistent state carry-over; incremental mutation; mid-buffer/sample-accurate automation where format supports it; fast-ramp 'parameter storm'; toggling power/bypass; reset and preset reload; cross-interaction with MIDI and transport. The complete snapshot mode must not mask history-dependent bugs; alternate with sequence and reset variants.

**Text mutation engine:** discover fields from supported UI accessibility tree, test-hook instrumentation, control maps or known parameter/string interfaces. Distinguish plain text from filenames, paths, numeric-only boxes, preset labels, passwords, URLs and limited-length protocol strings. Randomize lengths including empty, 1, near max, max, and deliberately oversized when input method permits; include ASCII, Unicode, combining marks, emoji, RTL, whitespace, newlines, NUL only if transport permits, control characters where permissible, markup-looking text, path separators, long repeated substrings and unusual normalization. Generate and log UTF-8 bytes, Unicode normalization, length and encoding. Avoid real secrets, destructive target paths and uncontrolled network traffic; synthetic sandbox only. Include a text boundary regression suite.

**Execution:** isolated child worker and plugin host, process-level watchdog, heartbeat, explicit timeout behavior, memory/CPU quotas where practical, core/minidump capture where supported, synchronous last-known intent journal before applying hazardous inputs, asynchronous telemetry flush, cancellation and safe shutdown. Handle host crash vs plugin crash as 'suspected origin' until stack trace confirms. No automatic infinite respawn on the same failing case. Separate plugin adapter exceptions from subject failures.

**Artifacts and triage:** manifest identifies target path/hash/version, platform/arch/OS, adapter/host versions, session/case seed, generator version, full parameter schema and snapshot (value + human-readable unit when available), ordered actions with timestamps, audio and MIDI fixture hashes, input text, plugin/host stderr, OS crash report, minidump path, native stack and symbol resolution when available, screenshot/video optional, observed audio metrics, resource samples, timeout thresholds, git revision, test runner version, reproduction command and environment. Store a concise machine-readable JSONL event log incrementally and produce immutable per-failure bundles. Deduplicate by normalized stack/failure signature while keeping distinct cases. Minimize failures via delta debugging/field reduction and sequence shrinking, but retain an untouched original artifact.

**Coverage:** show iteration count, valid snapshots applied, failure count, unique failures, parameter marginal coverage, boundary/discrete states exercised, pairwise coverage where computable, sequences and event categories exercised, native code coverage only in instrumented targets; provide confidence qualifiers and unseen/unmapped controls. Full exhaustive coverage is impossible for most parameter domains and should never be advertised.

**Stopping:** manual stop; total elapsed time; case budget; first unique failure; failure-rate threshold; minimum coverage goal where measurable; optional plateau detection. Support pause/resume and checkpoint after crash. Show estimated throughput as observed, not fixed guesses.

**Acceptance fixture:** sample fake plugin with numeric, log-scaled, bool, enum and text controls; intentionally buggy controls that trigger NaN, crash, hang and memory leak; verify complete randomization each case, 1000ms default, overrideable dwell, deterministic seed replay, crash survival and exact output artifact match. Create integration tests using intentionally failing synthetic plugins, never depend on private third-party binaries.

## Functional architecture to produce

Specify component interfaces and process boundaries, not merely a folder tree:

- Desktop controller UI (Windows-first), CLI and optional read-only local report server.
- Campaign planner and test scheduler: jobs, quotas, matrix expansion, priorities, seeds, checkpoints, transitions.
- Target discovery and capability registry: plugin format, binary/host architecture, parameters and editable text, provenance/permissions and supported test methods.
- Host workers per plugin format: VST3, CLAP first, other format adapters later; correct threading, process callback, parameter and MIDI/event protocols, editor lifetime, bus lifecycle.
- Standalone application adapter: launch/kill, window/process monitor, optional documented SDK/test hooks, filesystem sandbox and accessibility controls; explicit black-box limitations.
- GUI automation abstraction: UIA Windows; macOS AX and Linux AT-SPI as researched; calibrated image-based fallback; screenshot comparator with masks.
- DSP stimulus/render engine: deterministic fixture generation or imported fixtures, null virtual output, multichannel, transport/MIDI, capture and metrics.
- Fuzz/mutation strategies using a versioned test-case protocol and seeded RNG with separately derived streams for control/audio/text/timing.
- Watchdog and observability agents: process tree, crashes, stderr, logs, memory/CPU, audio overrun, heartbeat, optional sanitizers.
- Reproducer, minimizer, corpus manager, failure signatures and false-positive triage.
- Durable results store with SQLite job metadata, JSONL event streams and content-addressed artifacts, retention/GC, HTML/JSON/JUnit exports.
- External tool integration boundary for pluginval, clap-validator, VST3 SDK tools and native validators; avoid reimplementing compliance suites when robust integration exists.
- CI adapters to GitHub Actions and other pipelines, with fast PR smoke, nightly matrix and opt-in longer fuzz/soak campaigns.
- Extension/plugin SDK with versioned schema, capability handshake, documented permissions, tests and failure isolation.

Evaluate **C++20/JUCE** versus **Rust core plus C++/JUCE format-host shims** versus **Python/PySide6 orchestration with native workers**. Make a reasoned and benchmark-driven recommendation addressing hard real-time constraints, GUI iteration speed, cross-platform distribution, licensing, binary dependencies, debugging, testability and maintainability. Prefer out-of-process workers with strict protocol boundaries; do not require the Python interpreter on an audio callback thread.

Design a target-neutral protocol with at least TargetManifest, ParameterDescriptor, TextFieldDescriptor, CapabilityMatrix, TestProfile, TestCase, StepAction, TestOutcome, FailureArtifact, CrashFingerprint and ReproductionRecipe. Provide JSON Schema/protobuf/IDL proposals and example serialized instances. Define how unknown values, units, host transport events, nondeterministic environment, and format-native parameter IDs are represented without loss.

## Desktop UX

Design a usable native desktop app with:
- Left target browser: drag/drop plugin/app, recent builds, version/hash, metadata, formats, scanner errors and adapter selection.
- Central test dashboard: test suite selection (Quick Smoke, Validation, DSP, GUI, Ultimate Fuzz Stress, Soak, CI Profile), durations, per-suite options, tests supported/not supported, launch/pause/stop, progress and findings.
- Ultimate Fuzz page: prominent per-case dwell control **1.0 s by default**, included/excluded parameter counts, live parameter snapshot table, seeds, sampling mode, text fuzz controls, audio/MIDI source, isolation choice, reset mode, stop conditions and observed throughput.
- Failures view: deduplicated issue list, severity and confidence, replay/shrink buttons, event timeline, charts and screenshots, copy reproduction command, export bug package.
- Report view: coverage qualification, pass/fail/skip/unknown, actual evidence, trend-by-build, before/after regression.
- Advanced configurable policies without overloading Quick Smoke.

Describe all screens, states (including empty and crash states), keyboard access, navigation, responsive behavior, accessibility, and future UX mockup/spec handoffs. Use stable GUI controls instead of custom-drawn widgets where feasible; no requirements for flashy animations.

## Privacy, isolation and testing safeguards

SoundGrind will be used against developer-owned/test-authorized binaries. It should run untrusted or unstable binaries in locally isolated child processes, with ephemeral workspaces and optional VM/container isolation. Default offline/no network, and do not transmit source code, binaries, crash dumps or logs. Users must explicitly opt into network tests, real hardware output, installation/system changes or uploading reports. Treat generated filenames as sandbox paths; protect real projects, data and audio devices. Build a threat and failure model distinguishing QA fuzzing from containment of truly malicious code; process isolation alone is not a security sandbox. Avoid falsely promising safe execution of arbitrary unknown code.

## Required engineering practices

- Modular boundaries; no global god-class; no direct plugin-format SDK calls from UI or analytics layer.
- Feature flags and versioned schemas. Build smallest coherent vertical slice first.
- Test-driven development: synthetic plugins/apps, deterministic fixtures, unit/integration/e2e contract tests, concurrency and crash-injection tests.
- Continuous native builds on Windows/Linux/macOS where agents exist; explicit skipped-platform declarations.
- Persist existing user work before refactors; use focused branches, PRs and clean merges. No speculative breaking migrations.
- Benchmark runner overhead separately from target performance. Report wall vs audio time.
- No fabricated measured results, no fake coverage/compliance, no hidden test exclusions.
- Focus on developer ergonomics: one command to run smoke, one command to reproduce failure, one exportable bug package.
- Log malformed generator cases independently so harness bugs are not blamed on the target.
- Validate licenses and actual API capabilities before committing dependencies.

## Required deliverables in the next design phase

Produce and store in this repo:
1. docs/DESIGN_SPEC.md — developer-ready product specification: scope, personas, workflows, requirements, explicit non-goals, glossary, assumptions, risks, priority and measurable acceptance criteria.
2. docs/ARCHITECTURE.md — diagrams (Mermaid/ASCII), process boundaries, data flow, adapter interfaces, threading model, licensing rationale and architecture decision records.
3. docs/TEST_MATRIX.md — complete initial test suite matrix by platform/format/capability, exact test oracle and fixture needs.
4. specs/ULTIMATE_FUZZ_STRESS.md — normative full state machine, RNG contract, dwell-time semantics, text fuzz specification, seed/replay, watchdog, output artifacts and acceptance tests.
5. docs/DATA_CONTRACTS.md — versioned descriptors, results formats, example cases, errors, schema migration.
6. docs/UX_SPEC.md — screens, controls, accessibility, status and interactions.
7. docs/ROADMAP.md — milestone-by-milestone vertical slices, dependencies, effort/risk/priority, pass/fail acceptance, research needed and exit gates.
8. docs/RESEARCH_NOTES.md — dated primary-source source ledger and integration vs reimplementation analysis.
9. specs/INDEX.md — one-page list and status for each needed subsystem specification (see existing spec index) with owner, dependency, inputs/outputs, validation plan.
10. docs/DECISIONS.md — concise ADRs listing accepted decisions, rejected alternatives and open questions.

Every spec must contain: user story; scope/non-goals; assumptions; interface/API contract; state transitions; failure modes; test fixtures; unit/integration/e2e test plan; cross-platform requirements; resources; acceptance criteria; dependencies; unresolved questions; and staged implementation tasks. The design must label what is implemented versus planned. Do not mark any subsystem as complete until measured tests prove it.

## Design workflow / execution sequence

1. Inspect repo, prior decisions and outstanding work; preserve anything existing.
2. Research architecture-critical unknowns; log sources and licensing.
3. Evaluate 2–3 feasible host/orchestrator stacks with a reasoned tradeoff table.
4. Draft whole-product scope, process architecture and versioned interface contracts.
5. Write Ultimate Fuzz Stress spec **first** among detailed subsystem specifications, with pseudocode of the case loop, deterministic seed derivation, journal and replay.
6. Draft the UX and capability matrix, then test matrix, report schema and CI strategy.
7. Partition remaining details into separate subsystem specs in dependency order.
8. Perform adversarial design review: audio-thread blocking, invalid host calls, automation-thread errors, GUI fragility, process restarts, RNG drift, crashes before logs flush, silent false negatives, platform-specific differences.
9. Cross-check requirements against architecture and tests; remove unsupported or speculative claims.
10. Commit design documentation in reviewable changes, update a persistent handoff and concise next-step register. Do not implement product code in this design-only task.

## Design review exit gate

The design is reviewable when a developer unfamiliar with the project can:
- identify exact MVP format/platform support and exclusions;
- implement a correct CLAP/VST3 out-of-process worker with a versioned protocol;
- define and execute a reproducible full-snapshot randomized one-second case;
- inspect a case manifest after a hard crash and replay it;
- distinguish a plugin crash, harness bug, DSP anomaly and untestable GUI control;
- understand text-field discovery limits and exact string generation options;
- create the first synthetic failing plugin test;
- calculate what coverage was actually observed and what remains unknown;
- see clear specifications and test gates for every future module;
- deliver initial Windows builds without requiring the entire long-term roadmap.

If requirements conflict, make a documented tradeoff and state why. If research cannot verify a capability, record it as a question and defer the dependent feature. The ultimate target is a reliable, maintainable, well-instrumented audio quality engineering platform, not a random-clicking macro tool.
