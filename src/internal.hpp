#pragma once

#include "soundgrind/engine.hpp"
#include <chrono>

namespace soundgrind::internal {
struct ChildResult {
  int exit_code{-1};
  bool timed_out{false};
  std::string stderr_text;
};
ChildResult run_child(const std::filesystem::path& executable,
                      const std::vector<std::string>& args,
                      std::uint32_t timeout_ms,
                      const std::filesystem::path& stderr_path);
Json make_snapshot(const Json& descriptors, const CampaignConfig& config,
                   std::uint64_t state_index, const std::string& fixture_fault);
Json make_case_record(const CampaignRequest& request, const CampaignResult& result,
                      std::uint64_t state_index, std::uint64_t round_index,
                      const Json& snapshot, std::uint64_t case_index);
Json load_json(const std::filesystem::path& path);
void write_json_atomic(const std::filesystem::path& path, const Json& value);
std::string make_id(std::uint64_t seed, std::uint64_t index);
}
