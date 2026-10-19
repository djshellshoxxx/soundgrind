# SoundGrind Research Notes and Reuse Candidates

Checked: 2026-10-19. Primary sources are favored; versions, licensing and capabilities must be rechecked immediately before implementation. This is a research snapshot, not dependency approval.

## High-leverage existing projects

| Tool / source | Confirmed relevant capability | Fit for SoundGrind | License / diligence |
| --- | --- | --- | --- |
| Tracktion pluginval | Cross-platform GUI/headless plugin validation; separate-process execution; VST3/AU and formats supported by its build; CI use; an existing parameter-fuzz test | Integrate as optional external validator early, use as inspiration for worker supervision and format validation; avoid recreating mature checks | pluginval README identifies GPLv3. Prefer standalone subprocess invocation with clear license assessment; do not copy/link its code into a proprietary design without appropriate review |
| free-audio clap-validator | CLAP validation plus multiprocess fuzzer, random parameters/note/transport, reproducible seeds and crash isolation | Invoke as a first-class validator; compare native SoundGrind fuzz results with it; study seed replay and host compliance | Project identifies MIT license. CLI and output shape need version-specific verification |
| Steinberg vst3sdk | VST3 API and integrated plugin test host/validator | Native VST3 worker and official test integration; avoid inventing protocol semantics | SDK current README and LICENSE identify MIT; trademark usage is a separate concern |
| dfl/clap-trap | Headless CLAP tests, render, benchmark and state commands | Candidate complementary headless utility after code/license health review | License and maintenance status not yet verified |
| CLAP specification/parameter extension | Protocol-level distinctions between main-thread readback and audio-process parameter events | Critical for correct per-format worker design and case-application legality | Check exact protocol revision and details before shipping |
| LLVM libFuzzer | In-process coverage-guided fuzzing, seeds, corpus minimization, sanitizer-assisted crash detection | Optional white-box parser/state decoder fuzz harnesses; not a replacement for black-box plugin host fuzz loop | Compiler-instrumented target required. LLVM notes original libFuzzer feature development ceased, with bug fixes maintained; evaluate current alternatives |
| AFL++ | Instrumented feedback fuzzing, corpus, persistent mode, crash/coverage tools | Source-enabled plugin-state and file-parser fuzz harnesses; complement randomized valid-state campaigns | Verify current license and platform compatibility |
| Microsoft UI Automation | Programmatic inspection/control of many Windows desktop UI elements | Core Windows standalone-app GUI automation and accessibility introspection | Custom-drawn plugin UIs may not expose semantic controls; a test hook or calibrated fallback is needed |

## Primary links and precise observations

1. Tracktion pluginval: https://github.com/Tracktion/pluginval
   - README documents GUI and headless modes, separate-process validation, VST/AU/VST3 testing and Linux/Windows/macOS.
   - README also demonstrates a plugin test subclass named FuzzParametersTest and states GPLv3 licensing.
   - Check actual currently supported plugin formats in the shipping build, not only docs/CLAUDE.md.

2. free-audio clap-validator: https://github.com/free-audio/clap-validator
   - README describes validate and a dedicated multi-process fuzz subcommand with reproducer seed.
   - The fuzz command varies parameter values, note events and transport changes, checking crash/hang/spec violations.
   - The manifest https://github.com/free-audio/clap-validator/blob/master/Cargo.toml identifies MIT licensing and, at research time, version 0.4.1.
   - Separate value from SoundGrind: shared dashboard and artifacts; controls spanning plugins + apps + GUI + DSP; full-snapshot configurable-dwell campaigns.

3. Steinberg VST3 SDK: https://github.com/steinbergmedia/vst3sdk
   - README identifies SDK version 3.8.x and included test host/validator.
   - Official validator guidance: https://steinbergmedia.github.io/vst3_dev_portal/pages/FAQ/Miscellaneous.html
   - SDK licensing: https://github.com/steinbergmedia/vst3sdk/blob/master/LICENSE.txt and README identify MIT terms. Check specific dependencies and branding guidance independently.
   - Integration strategy: use official test binary as validator and SDK to implement SoundGrind's own worker when needed.

4. Headless CLAP helper: https://github.com/dfl/clap-trap
   - README describes validate, benchmark, render and state tests.
   - Candidate integration only; no firm dependency until maintenance, license and behavior are reviewed.

5. CLAP parameter protocol source: https://github.com/free-audio/clap/blob/main/include/clap/ext/params.h
   - Distinguishes main-thread parameter reads, audio-process automation events, and flush calls outside process.
   - A fuzz controller must schedule changes through protocol-correct threads/events; arbitrary direct writes from GUI/background threads are wrong.

6. LLVM official docs: https://llvm.org/docs/LibFuzzer.html
   - Corpus evolution, deterministic fuzz targets, sanitizer flags, crash artifacts, parallel workers and minimizing corpus.
   - White-box fuzzing should be implemented as a different adapter/mode than full-app one-second dwell because in-process fuzz targets are normally much shorter and require resettable state.
   - Original libFuzzer feature development has stopped per official LLVM documentation; consider modern maintained tools during implementation.

7. AFL++ official docs: https://github.com/AFLplusplus/AFLplusplus/blob/stable/docs/fuzzing_in_depth.md
   - Persistent mode, instrumentation and coverage reporting are relevant to malformed files, presets, structured state blobs and parser APIs.
   - Differentiates real code coverage from SoundGrind's boundary/parameter coverage.

8. Windows UI Automation official overview: https://learn.microsoft.com/windows/win32/winauto/entry-uiauto-win32
   - UI Automation exposes elements and operations for many applications and supports automated UI tests.
   - Client developer guidance: https://learn.microsoft.com/en-us/windows/win32/winauto/uiauto-clientportal
   - Semantically accessible controls are best; screenshot-coordinate fallback is inherently susceptible to DPI, theme and window position.

9. Windows GUI example FlaUI-based testing: https://github.com/ac0z/flaui-desktop-automation
   - A recent open-source example of page objects, semantic locators, screenshots on failure and test runners. Read as an implementation example, not proof of universal desktop GUI coverage.
   - Platform is Windows. Evaluate official FlaUI upstream/license and current .NET compatibility.

10. pluginval docs: https://github.com/Tracktion/pluginval/blob/develop/docs/Testing%20plugins%20with%20pluginval.md
    - Separating validation in its own process matters to keep the user-facing tool alive after plugin failures.

## Early design guidance derived from sources

**Integrate, do not blindly reimplement:** use pluginval / clap-validator / SDK validators as subprocess tools with their exact version, command line and raw output as attached provenance. These validate different aspects and no one tool should be credited with tests it did not run.

**Core unique engine:** persist complete parameter snapshots + one-second default test cycles + text mapping and fuzz + DSP stimuli + child crash journal + deterministic replay + cross-adapter reporting.

**Be strict about protocols:** changing parameters during process is not the same as setting a UI value or serialized state. Define allowed changes per format and thread, with sample-accurate events where supported.

**Use two independent fuzz tracks:** (a) black-box or gray-box randomized *valid states* inside plugin/GUI/app runtime; (b) separate instrumented parser/state/byte fuzzing with AFL++/libFuzzer or another chosen maintained fuzzer. Combining these into one undifferentiated metric would confuse coverage and timing.

**Measure realistically:** one-second dwell is best specified as 1 second of processed audio for offline plugin renders and 1 second elapsed wall time for standalone live apps. Be explicit about sample-rate conversion, tail, setup and teardown. Offline rendering need not consume one second of wall-clock time.

## Known research gaps / mandatory follow-up

- Native VST3 and CLAP C++ host library comparison, including ABI, shutdown/editor API behavior and licensing.
- JUCE Commercial/AGPL license implications versus alternative GUI toolkit and whether a JUCE host can meet every desired CLAP requirement without external bindings.
- Official Apple AU validation invocation and entitlements; native macOS build requirements.
- Linux LV2 host and GUI automation integration; AT-SPI support and virtual display strategy.
- Exact default thresholds for various plugin classes; must be fixture-dependent rather than universal THD or RMS guesses.
- APIs for external automation in named DAWs. Do not claim blanket DAW support.
- Mechanisms for reliable crash minidumps on Windows, core dumps on Linux and crash reports on macOS.
- Whether sandboxing is provided by job objects, restricted tokens, VM isolation or user-configured environment; process isolation is crash containment, not malware containment.
- Reproducibility and license review for third-party tool binaries that SoundGrind might redistribute.

## Reuse decision template

For each candidate document: source URL + commit/tag; dependency graph and license; tested OS/arch; performance and stability; native API vs CLI; required modifications; feature overlap; integration maintenance cost; upstream churn; concrete prototype test; verdict **adopt / invoke / study only / reject**; supporting evidence.

Every new design subsystem spec must update this file with relevant research findings, without deleting older conclusions silently.
