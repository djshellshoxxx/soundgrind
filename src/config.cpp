#include "soundgrind/engine.hpp"
#include <stdexcept>

namespace soundgrind {
std::string to_string(Verdict v) {
  switch (v) {
    case Verdict::pass: return "PASS";
    case Verdict::fail: return "FAIL";
    case Verdict::warning: return "WARNING";
    case Verdict::skipped: return "SKIPPED";
    case Verdict::unsupported: return "UNSUPPORTED";
    case Verdict::inconclusive: return "INCONCLUSIVE";
    case Verdict::infrastructure_error: return "INFRASTRUCTURE_ERROR";
    case Verdict::cancelled: return "CANCELLED";
  }
  return "INCONCLUSIVE";
}

std::vector<std::string> profile_modules(const std::string& profile) {
  if (profile == "level1" || profile == "smoke")
    return {"clap.lifecycle", "audio.finite_output", "fixture.oracle",
            "lifecycle.repeat", "params.readback"};
  if (profile == "level2" || profile == "features")
    return {"clap.lifecycle", "audio.finite_output", "fixture.oracle",
            "lifecycle.repeat", "params.readback", "params.boundaries",
            "params.normalized_mapping", "processing.block_sizes", "state.roundtrip"};
  if (profile == "level3" || profile == "stress" || profile == "fuzz1" ||
      profile == "fuzz2" || profile == "grind")
    return {"clap.lifecycle", "audio.finite_output", "fixture.oracle",
            "params.readback", "params.boundaries", "params.normalized_mapping", "processing.block_sizes"};
  throw std::invalid_argument("unknown profile: " + profile);
}

void validate_config(const CampaignConfig& c) {
  if (c.profile.empty()) throw std::invalid_argument("profile cannot be empty");
  if (!c.modules.empty()) {
    for (const auto& id : c.modules) {
      const auto modules = registered_modules();
      bool known = false;
      for (const auto& m : modules) if (m.id == id) known = true;
      if (!known) throw std::invalid_argument("unknown test module: " + id);
    }
  }
  if (c.unique_states == 0 || c.rounds_per_state == 0)
    throw std::invalid_argument("states and rounds must both be at least 1");
  if (c.dwell_ms == 0 || c.dwell_ms > 600000)
    throw std::invalid_argument("dwell-ms must be within 1..600000");
  if (c.dwell_min_ms == 0 || c.dwell_max_ms < c.dwell_min_ms || c.dwell_max_ms > 600000)
    throw std::invalid_argument("dwell interval must satisfy 1 <= min <= max <= 600000");
  if (c.sample_rate < 8000 || c.sample_rate > 384000)
    throw std::invalid_argument("sample-rate must be within 8000..384000");
  if (c.block_size == 0 || c.block_size > 65536)
    throw std::invalid_argument("block-size must be within 1..65536");
  if (c.timeout_ms < 50 || c.timeout_ms > 600000)
    throw std::invalid_argument("timeout-ms must be within 50..600000");
  if (c.max_cases == 0) throw std::invalid_argument("max-cases must be at least 1");
}

std::uint64_t planned_case_count(const CampaignConfig& c, std::size_t selected_module_count) {
  (void)selected_module_count;
  const auto states = static_cast<std::uint64_t>(c.unique_states);
  const auto rounds = static_cast<std::uint64_t>(c.rounds_per_state);
  if (states > UINT64_MAX / rounds) throw std::overflow_error("planned case count overflow");
  return states * rounds;
}
}
