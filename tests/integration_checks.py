import json
import shutil
import subprocess
import sys
from pathlib import Path

cli, worker, target, outdir = map(Path, sys.argv[1:])
outdir.mkdir(parents=True, exist_ok=True)

def run(*args, ok=True):
    p = subprocess.run([str(cli), *map(str, args)], text=True, capture_output=True)
    if ok and p.returncode != 0:
        raise AssertionError(f"command failed ({p.returncode}): {p.stderr}\n{p.stdout}")
    if not ok and p.returncode == 0:
        raise AssertionError(f"command unexpectedly passed: {args}")
    return p

ins = json.loads(run("inspect", "--target", target, "--worker", worker,
                     "--out", outdir / "inspect").stdout)
assert ins["plugin_id"] == "org.soundgrind.synthetic"
assert ins["capabilities"]["params"] and ins["capabilities"]["state"]
assert len(ins["parameters"]) == 3

preview_dir = outdir / "preview"
run("test", "--target", target, "--worker", worker, "--out", preview_dir,
    "--profile", "level3", "--states", "3", "--rounds", "5", "--max-cases", "15", "--preview")
preview = json.loads((preview_dir / "campaign.json").read_text())
assert preview["planned_cases"] == 15
assert preview["preview"]["unique_states"] == 3
assert preview["preview"]["rounds_per_state"] == 5
too_many = run("test", "--target", target, "--worker", worker, "--out", outdir / "over-budget",
               "--profile", "level3", "--states", "3", "--rounds", "2", "--max-cases", "5", ok=False)
assert "planned cases exceed --max-cases" in too_many.stderr

campaign_dir = outdir / "fuzz1"
run("test", "--target", target, "--worker", worker, "--out", campaign_dir,
    "--profile", "fuzz1", "--states", "3", "--rounds", "2", "--dwell-ms", "1",
    "--seed", "800813", "--max-cases", "6", "--modules", "fixture.oracle,params.readback")
campaign = json.loads((campaign_dir / "campaign.json").read_text())
assert campaign["planned_cases"] == campaign["completed_cases"] == 6
assert len(campaign["cases"]) == 6
assert all(len(c["parameter_snapshot"]) == 3 for c in campaign["cases"])
domains = {p["id"]: p for p in ins["parameters"]}
for case in campaign["cases"]:
    for param_id, value in case["parameter_snapshot"].items():
        domain = domains[param_id]
        assert domain["min"] <= value <= domain["max"]
        if domain["stepped"]:
            assert value.is_integer()
assert all(c["processed_samples"] >= 48 for c in campaign["cases"])
assert all({m["module_id"] for m in c["module_results"]} == {"fixture.oracle", "params.readback"}
           for c in campaign["cases"])
assert campaign["cases"][0]["parameter_snapshot"] == campaign["cases"][1]["parameter_snapshot"]
assert campaign["cases"][0]["state_index"] == campaign["cases"][1]["state_index"]
assert campaign["cases"][0]["round_index"] != campaign["cases"][1]["round_index"]
assert campaign["cases"][0]["actual_dwell_ms"] >= 1.0

repeat_dir = outdir / "fuzz1-repeat"
run("test", "--target", target, "--worker", worker, "--out", repeat_dir,
    "--profile", "fuzz1", "--states", "3", "--rounds", "2", "--dwell-ms", "1",
    "--seed", "800813", "--max-cases", "6", "--modules", "fixture.oracle,params.readback")
repeat = json.loads((repeat_dir / "campaign.json").read_text())
assert [x["parameter_snapshot"] for x in campaign["cases"]] == [
    x["parameter_snapshot"] for x in repeat["cases"]]

source = sorted((campaign_dir / "cases").glob("*.json"))[-1]
source_before = source.read_bytes()
replay_dir = outdir / "replay"
run("replay", "--target", target, "--worker", worker, "--case", source,
    "--out", replay_dir)
replay = json.loads((replay_dir / "campaign.json").read_text())
assert replay["cases"][0]["parameter_snapshot"] == json.loads(source.read_text())["parameter_snapshot"]
assert replay["target_hash_override"] is False
assert source.read_bytes() == source_before

html = outdir / "manual-report.html"
run("report", "--input", campaign_dir / "campaign.json", "--out", html)
assert "SoundGrind campaign report" in html.read_text()
assert "fixture.oracle" in html.read_text()

tampered = outdir / "tampered.clap"
shutil.copyfile(target, tampered)
with tampered.open("ab") as f:
    f.write(b"tamper")
mismatch = run("replay", "--target", tampered, "--worker", worker, "--case", source,
               "--out", outdir / "mismatch", ok=False)
assert "SHA-256 mismatch" in mismatch.stderr
print("CLI inspect, preview/budget, deterministic snapshots, domain checks, dwell, replay/hash guard and HTML report passed")
