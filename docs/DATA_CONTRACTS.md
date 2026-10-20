# M1 data contracts

## Stable API

Public types are declared in `include/soundgrind/engine.hpp`. The main API is:

```cpp
CampaignResult run_campaign(const CampaignRequest& request);
CampaignResult replay_case(const CampaignRequest& request,
                           const std::filesystem::path& saved_case,
                           bool allow_target_mismatch = false);
```

The profile names and module IDs are public values. `CampaignConfig` defaults to profile `level1`, seed 1, one unique state, one round, 1,000 ms, 48 kHz, 64 frames, 5,000 ms worker timeout and 1,000 max cases. The CLI uses three rounds for the default `level1` smoke run.

## Case record (`soundgrind.case.v1`)

Required fields include:

```json
{
  "schema": "soundgrind.case.v1",
  "engine_version": "0.1.0",
  "campaign_id": "seed-time-token",
  "case_id": "seed-index",
  "target_path": "plugin.clap",
  "target_sha256": "hex SHA-256",
  "target_format": "CLAP",
  "plugin_identity": {"id": "vendor.plugin", "name": "Plugin", "vendor": "Vendor", "version": "1.0"},
  "module_ids": ["fixture.oracle"],
  "module_versions": {"fixture.oracle": "1.0.0"},
  "state_index": 0,
  "round_index": 0,
  "campaign_seed": 42,
  "case_seed": 42,
  "generator_version": "uniform-domain-v1",
  "parameter_snapshot": {"gain": 0.75},
  "excluded_parameters": [],
  "fixture_id": "constant-0.25-mono-v1",
  "sample_rate": 48000,
  "block_size": 64,
  "requested_dwell_ms": 1,
  "planned_samples": 64,
  "status": "PREPARED",
  "module_results": [],
  "artifacts": []
}
```

The source case file is flushed before the worker starts. On completion, `status`, sample observations, module results, classification and worker log paths are added. `UNSUPPORTED` and `INCONCLUSIVE` remain distinct from `PASS`.

## Worker request and response

Worker requests use JSON and operations `inspect` or `test`. The coordinator passes the target path, sample rate, block size, dwell and a complete snapshot. Worker responses include plugin identity, capabilities, audio ports, parameter descriptors, `module_results`, processed samples and effective dwell. CLAP calls remain in the adapter and its worker process.

## Verdicts and classifications

Verdicts: `PASS`, `FAIL`, `WARNING`, `SKIPPED`, `UNSUPPORTED`, `INCONCLUSIVE`, `INFRASTRUCTURE_ERROR`, `CANCELLED`.

Strong failure classifications used in M1: `non_finite_output`, `fixture_oracle_mismatch`, `parameter_application_mismatch`, `state_roundtrip_failure`, `worker_crash`, `hang_or_timeout`, `worker_error`, `module_failure`.

## Campaign result and HTML

`campaign.json` preserves campaign config, target SHA-256, planned and completed case counts, per-case evidence and summary counts. `report.html` is a self-contained rendering of the JSON result. It escapes JSON before inserting it into HTML. The report does not claim code coverage or correctness beyond the registered assertions actually exercised.
