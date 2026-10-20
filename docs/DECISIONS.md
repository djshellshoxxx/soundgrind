# M1 architecture decisions

## ADR-001: Start with the CLAP worker

**Status:** Accepted for M1. Use the official CLAP C ABI as the first native plugin adapter. CLAP provides explicit plugin lifecycle, audio-port, parameter-event and state contracts, and can be driven without a physical audio device. The adapter is isolated behind `soundgrind_clap_adapter`; no CLAP calls belong in the scheduler or report renderer. VST3, desktop GUI, standalone application automation and DAW integrations remain outside this milestone.

## ADR-002: Reusable registered modules and profiles

**Status:** Accepted for M1. Stable module IDs plus independent profile composition are the public test-selection contract. Profiles only select IDs. The current adapter gathers CLAP observations and the campaign maps observations to requested module results. Adding a module must add its descriptor and evaluation; it should not alter campaign planning or report output.

## ADR-003: One supervised worker per case

**Status:** Accepted for M1. The coordinator writes a complete case file and request before launching `soundgrind_worker`. Worker timeouts and nonzero exits are converted into campaign failures without ending the coordinator. OS process supervision is crash containment, not a security sandbox.

## ADR-004: Offline, sample-count-based dwell

**Status:** Accepted for M1. No device is opened. Requested milliseconds become samples at the configured rate, then round up to a complete block. Reports include requested and actual sample-based dwell. Test processing is bounded by state count, rounds, maximum cases and worker timeout.

## ADR-005: Evidence is JSON first

**Status:** Accepted for M1. Versioned JSON is canonical. Case records are written before worker launch, flushed to storage and atomically replaced on completion. HTML is generated from saved JSON and is not the source of truth. Replays write to a separate directory.
