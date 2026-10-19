# SoundGrind — Ultimate Fuzz Stress Engine

Spec ID: SG-FUZZ-001
Status: PROVISIONAL DESIGN DRAFT, unimplemented
Date: 2026-10-19
Dependency: docs/PRODUCT_REQUIREMENTS.md and prompts/MASTER_DESIGN_PROMPT.md
Normative words: MUST indicates initial acceptance requirement, SHOULD preferred, MAY future enhancement.

## Purpose

For a chosen audio plugin or audio application, repeatedly assign independently randomized **complete settings states** to every controllable exposed parameter, optionally fuzz discovered writable text inputs, stimulate the target and observe it for a configurable duration. Default evaluation dwell is **1.000 second**. If a fault appears, preserve the exact input, environment and ordered actions so a developer can replay the case. This engine provides **stochastic testing**, not exhaustive enumeration.

## Terms / timing semantics

- **Campaign:** one named sequence of cases with one immutable target manifest, generator version, configuration, master seed and bounded run policy.
- **Case:** one generated snapshot plus an ordered action sequence and observation interval.
- **Parameter:** native exposed typed control via host protocol, adapter or documented test bridge. Custom UI widgets are not automatically native parameters.
- **Complete settings snapshot:** assignment to every enabled, supported writable parameter in the discovered state schema, plus an explicit uncovered/excluded table.
- **Text field:** independently discovered writable text entry with locator/injection capability, possibly not part of plugin parameter interface.
- **Dwell:** seconds (default 1.000) between effective setting application/settle completion and observation end.
- **Offline plugin:** dwell is audio process duration in samples, usually sample_rate * dwell, rounded by an explicitly documented policy; actual number of processed samples MUST be logged. Wall time may be shorter.
- **Live app:** dwell is real monotonic wall-clock time. It MUST not be presented as processed audio duration without a stream clock.
- **Settle time:** independently configurable optional phase for state to stabilize, not hidden inside dwell.
- **Fail-safe budget:** separate case timeout, worker start/stop timeout, and overall campaign time, not automatically 1 second.

## Configuration defaults and controls

| Key | Default | Validation / meaning |
| --- | --- | --- |
| profile | valid-complete-state | Uses legal domains and all available writable controls |
| dwell_ms | 1000 | Integer or duration representation; UI shows 1.0 s; user can override, value > 0 |
| campaign_seed | generated and persisted | Stable explicit 128-bit+ seed/bytes; cryptographic unpredictability not required |
| max_cases | UI explicit finite default pending UX decision | Never imply infinite run accidentally; CLI may offer until-cancelled |
| max_wall_time | unset unless user chooses | Finite CLI/job policy recommended |
| reset_mode | fresh_session per case or controlled restore, subject to runtime cost | Must disclose restart costs and actual lifecycle |
| stimulus | deterministic safe fixture | Audio/MIDI+transport capabilities gate selections |
| audio_output | offline/virtual muted | No physical audible output by default |
| mode | valid_domain | Invalid API parameter values only in separate explicit negative suite |
| text_enabled | off unless mapped and user enables | Optional with controlled text profile |
| screenshot_policy | failures only | Avoid overhead and data leaks |
| failure_policy | continue after one crash with recovery limits | No infinite crash loop |
| minimum_journal | before_apply | Pre-apply snapshot and sequence are flushed durably |
| output_retention | bounded configurable | Preserve known failure artifacts until user-directed cleanup |
| ui_poll_ms | adaptive, not in audio callback | Must not block audio worker |

The specification MUST explicitly define numeric limits and default finite run budget before implementation. Do not invent a false 'safe' universal maximum; choose through UX/performance experiments.

## Discovery and schema

Native plugin adapter enumerates stable parameter IDs, writable/readable, kind (float/integer/bool/enum), valid min/max/step, normalization, logarithmic conversion semantics, human unit, default, display label, dependencies and known invalid combinations. For format-native normalized parameters, retain **both normalized and native textual/display representation** where supported.

The standalone app adapter enumerates accessibility/test-bridge controls and any documented API setting descriptors; automation-backed sliders may provide only apparent screen bounds or text constraints. Mark discovered bounds as authoritative, inferred or user-provided. Unknown bounds require a mapping or manual scope decision and MUST NOT be silently sampled as verified legal.

On every target-binary change, invalidate or revalidate descriptor/mapping hashes. Allow user exclusion and required correlated constraints with reasons. Every case records included/excluded mapping by ID.

## Deterministic sampler

Use an explicit named algorithm, pinned version and well-documented byte order, plus domain-separated seeds:
- per case derived from immutable campaign master seed and case index;
- per field derived from case seed and stable field ID;
- separate streams for parameter values, string values, audio stimulus, MIDI/transport timing and scheduling.
Avoid dependence on hash-map iteration order, thread scheduling, language runtime RNG defaults or plugin enumeration order. Sort stable IDs or rely on keyed derivation. Version changes MUST record compatibility and retain old replay decoder.

Sampling:
- float uniform legal physical/normalized range; optional log-aware distribution;
- bool either legal state; enums sample defined legal tokens;
- integer sample legal inclusive stepped lattice, with range overflow checks;
- boundary bias: min/max/default/near-min/near-max where meaningful;
- pairwise coverage mode: seeded covering array over appropriately bucketed domains;
- mutation: take a saved valid snapshot then perturb one or more fields and retain original;
- dependency: apply constraints and resample deterministically when possible, otherwise record rejected invalid candidate;
- text: choose length, encoding and characters based on field kind/profile, logging exact produced input.

Native parameter **automation** is a separate timeline contract; not every format supports writing every value from every thread at arbitrary time. The adapter MUST provide a lawful application sequence or fail the case as unsupported/invalid harness plan.

## State machine

States: CREATED -> PREFLIGHT -> READY -> CASE_PREPARE -> JOURNAL_SYNC -> APPLY -> SETTLE -> OBSERVE -> COLLECT -> CLASSIFY -> COMMIT -> CASE_PREPARE; termination: STOPPING -> FINALIZE -> COMPLETED. Exception substates: WORKER_FAILED -> RECOVER -> RESUME or ABORTED; CANCELLED / INFRASTRUCTURE_ERROR are terminal statuses.

Any abnormal termination between JOURNAL_SYNC and COMMIT must leave a pending case record identifiable by case ID and full intended state. On restart, supervisor reconciles pending case to crash/unknown or reruns it in a fresh worker as policy dictates; it must never erase it or classify it as PASS.

Illustrative algorithm, not executable implementation:

    validate_campaign_config()
    manifest = discover_target_and_capabilities()
    create_durable_campaign_record(manifest, master_seed, generator_version)
    while not stop_condition_met():
        case_id = next_monotonic_case_id()
        case_seed = derive_seed(master_seed, case_id, generator_version)
        snapshot = sample_all_included_controls(manifest, case_seed)
        text_inputs = sample_mapped_text_fields(manifest, case_seed)
        actions = build_protocol_legal_plan(snapshot, text_inputs, stimulus)
        preapply_journal_and_fsync(case_id, snapshot, actions, environment_hashes)
        worker = get_or_create_isolated_worker(reset_policy)
        apply_result = worker.apply(actions)
        collect_readback_and_apply_errors(apply_result)
        optional_settle()
        result = worker.observe(dwell_ms, stimulus, audio_monitor, watchdog)
        persist_telemetry_and_event_log(result)
        classify_and_commit(case_id, result)
        if result.crash_or_hang:
            preserve_artifacts_and_recover_worker()
            enforce_retry_limit()
        checkpoint_campaign_progress()

Use a phase-aware timeout and explicit worker cancellation. Synchronous fsync may be amortized per-case only when the user opts for a weaker crash-evidence guarantee; write-ahead durability must be documented.

## Observation oracles

Minimum P0: child exit status/signals, explicit process/application error, deadline timeout, hard hang, host protocol error, finite output sample scan, out-of-bounds channel/channel-layout mismatches, unexpected silent output only when a test fixture provides expected non-silence, callback time/deadline telemetry where available, resource snapshots.

P1: correlated GUI/DSP readback; screenshot compare; spectral tests, noise, clipping, instability and clicks with fixture-specific thresholds; memory growth across multiple cycles; invalid persistent settings; note-off/midi anomalies; recorded validator external results.

Classify PASS only for tested checks. A case with inaccessible text or GUI controls may pass the tests executed while leaving those capabilities UNSUPPORTED.

Crash event classification includes worker crash vs plugin crash (unknown attribution until stack evidence). Generator and harness assertions are INFRASTRUCTURE_ERROR and must never be mislabeled plugin defects.

## Text profiles and transport details

Field detection order: app test API/hooks -> accessibility semantics -> explicit user control maps -> image/OCR-assisted heuristics only as reviewable optional experiment.

Fields are typed: arbitrary text, short label, search, numeric editor, filename, filesystem path, preset name, multiline, secrets (excluded), special format, other. Apply constraints by profile. Useful lengths: 0/1/2/near nominal max/nominal max/oversized. Unicode cases: BMP/non-BMP, combining sequences, grapheme clusters, bidirectional text, whitespace, CR/LF, multibyte encodings, emoji, long repeated substrings. Preserve escape-safe display and **exact byte/code-point representation** for replay. Screen-typed characters have different limits than API-submitted strings; record exact injection mechanism, resulting readback and truncation.

Never default to pasting generated path strings into app dialogs that can write outside a disposable workspace. File/preset mutation tests must use temporary test fixtures and explicit allowlisted directories. For destructive operations, require separate authorization.

## Audio/MIDI stimulus profiles

P0 deterministic silence, impulse, normalized sine, noise, mono/stereo and a short note-on/note-off MIDI fixture. Later chirps, multitone, pulses, program change/control messages, dense chord transitions, transport tempo jumps, randomized buffer sizes and asynchronous parameter automation. Attach original input and exact generated seed; preserve channel layout and sample format metadata. Default output should not reach loudspeakers or external devices.

## Results and replay contract

Write append-only JSONL for per-case transitions and immutable per-failure bundles. At minimum each case stores:
- campaign_id / case_index / case_seed / RNG version;
- target binary SHA-256, plugin ID, app path, format and version;
- host/adapter version and OS/CPU/architecture;
- capability manifest/schema hash and parameter inventory;
- complete attempted settings, skipped/unmapped entries and apply/readback outcome;
- text inputs exact serialized form, injection method and readback where possible;
- stimulus fixture and exact hash, sample rate, buffer shape, events and transport;
- phase durations, desired dwell_ms, actual audio samples and monotonic elapsed;
- pass/fail/skip/unknown by oracle, raw measurements, error classification;
- dedup signature, stack/minidump/log/screenshot references and file hashes;
- replay command and manifest version.

Replay: use stored case file, exact compatible runner/protocol and archived input fixtures, verify hashes and target identity, reset correctly, reapply ordered actions and record agreement/disagreement across repeat count. A mismatch in target hash MUST be surfaced rather than silently accepted. When random external input (OS scheduling or device) is involved, report *best-effort replay*, not bitwise deterministic guarantee.

Minimizer: re-run using a majority/repetition threshold for intermittent failures; first eliminate nonessential events and fields, then shrink numeric fields to defaults/boundaries and text strings to shorter minimal examples, then reduce timings and sequences. Preserve original failing artifact and note that minimization can change reproduction probability.

## Coverage, campaign efficiency and stop policy

Track: total cases attempted/completed; complete snapshot cases; unique per-parameter values/buckets; observed min/max and boundary hit; pair coverage for quantized domains; supported/unmapped field ratio; GUI action/event classes; oracle pass/fail/skip count; unique crash fingerprints. Code branch/edge coverage is only shown with actual instrumentation. Do not present hash-collision-unsafe exact unique counts without error bounds; approximate cardinality estimators must label themselves approximate.

If two float sliders have effectively continuous domains, exhaustive enumeration is not a viable stop condition. 'Until every possible setting tested' is interpreted as repeating randomized samples until a user-defined time, case or measured coverage budget is reached, or manually stopped.

## Mock fixtures, edge cases and acceptance tests

Build synthetic plugins/apps that intentionally:
1. report 5 typed writable native parameters and verify all 5 sampled each iteration (even when one repeats a previous value);
2. expose 1 text field via test bridge and verify string generator with deterministic length/content;
3. crash when a documented exact boundary combination is applied;
4. hang on a named parameter sequence and exceed configurable watchdog;
5. emit NaN on a specific MIDI/parameter combination;
6. produce a background-thread crash during screenshot or editor close;
7. corrupt state on reload, detected by controlled state roundtrip;
8. return a readback value differing from attempted value due to quantization;
9. fail only after 50 iterations to prove state/history mode differs from fresh reset;
10. request unsupported architecture/format and get a truthful UNSUPPORTED result.

**P0 acceptance**:
- Default case observation duration is 1000 ms by design; test virtual elapsed/real app wall clock semantics, plus override to 250ms and 2500ms.
- One case applies a complete supported settings snapshot, not just one randomized parameter.
- Same seed, binary, generator version and fixture create same intended snapshot regardless of descriptor iteration order.
- Coordinator survives child crash and persists last attempted settings.
- A replay can reproduce a planted deterministic failure and reports actual reproduction success.
- An unmapped GUI text box is visibly uncovered, not counted as fuzzed.
- 10,000 mock cases can complete without corrupting results/IDs and without unbounded coordinator memory growth (performance limit to be benchmarked, not assumed).
- Full suite has test and output-schema validation, platform-specific integration tests and artifacts inspectable after cancellation.

## Engineering work decomposition

M0: research and schema/decision record.
M1: mocked deterministic sampler + complete-state generator + JSON manifest + P0 invariants.
M2: out-of-process worker and crash-proof write-ahead journal; synthetic child crashes.
M3: initial format-native plugin host (CLAP or VST3 selected by ADR) and safe offline audio fixture/dwell engine.
M4: second native plugin host adapter plus GUI/CLI controls and report exports.
M5: Windows standalone-app process and accessibility control adapter.
M6: text profiles, fuzz strategies, replay and minimizer.
M7: extended measurement/matrices, third-party validators, CI, cross-platform workers.

Each milestone requires isolated modular tests and a reviewable PR; no merging unless its stated acceptance gates are met.

## Known open decisions

1. RNG standard and binary seed serialization.
2. Initial host format order (VST3 vs CLAP) following prototype reliability study.
3. Duration rounding policy for sample-domain dwell.
4. Default finite UI campaign budget and relative worker reset costs.
5. Whether pre-apply write-ahead journal fsync is per case or batched as user-configurable durability tradeoff.
6. What is a valid cross-parameter dependency model without imposing plugin-vendor-specific semantics.
7. Minimum resource protection for standalone apps on Windows.
8. How to implement optional sample-accurate asynchronous control events without introducing forbidden thread writes.

Do not implement beyond design until these questions are explicitly resolved and documented.
