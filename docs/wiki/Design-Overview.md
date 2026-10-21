# Design Overview

The flagship feature is **Ultimate Fuzz Stress** (`specs/ULTIMATE_FUZZ_STRESS.md`). It randomises every supported controllable
parameter with an editable dwell time (default 1 second), captures evidence and triages failures. Behaviour is deterministic
and reproducible, and it is a stochastic search rather than an exhaustive enumeration (README).

The spec's sections cover: purpose; timing semantics; configuration defaults; discovery and schema; a deterministic sampler;
a state machine; observation oracles; text profiles and transport; audio and MIDI stimulus profiles; results and replay;
coverage, efficiency and stop policy; mock fixtures and acceptance tests; work decomposition; and open decisions.
