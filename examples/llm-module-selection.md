# Select existing SoundGrind modules from an LLM workflow

The coding assistant does not generate a test harness. It chooses SoundGrind's modules and supplies project-specific command values. For example, after confirming that the plugin is a CLAP build and that the intended checks are oracle and parameter readback:

```sh
soundgrind test \
  --target ./build/MyPlugin.clap \
  --profile level1 \
  --modules fixture.oracle,params.readback \
  --states 8 \
  --rounds 2 \
  --dwell-ms 1000 \
  --seed 30442 \
  --max-cases 16 \
  --out ./test-results
```

The assistant then reads `test-results/campaign.json` and `test-results/report.html`, identifies module failures and unsupported assertions, and uses the saved case path for replay:

```sh
soundgrind replay --target ./build/MyPlugin.clap \
  --case ./test-results/cases/CASE_ID.json \
  --out ./replay-results
```

For a stress run, change `--profile` to `level3` or `fuzz1` and configure states, rounds, dwell, seed and a strict `--max-cases` bound. The test module implementation remains in SoundGrind.
