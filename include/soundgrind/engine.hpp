#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace soundgrind {
using Json = nlohmann::json;

inline constexpr const char* kSchemaVersion = "soundgrind.case.v1";
inline constexpr const char* kEngineVersion = "0.1.0";

enum class Verdict { pass, fail, warning, skipped, unsupported, inconclusive,
                     infrastructure_error, cancelled };
std::string to_string(Verdict verdict);

struct TestModule {
  std::string id;
  std::string version;
  std::string description;
  std::vector<std::string> required_capabilities;
};

struct ModuleResult {
  std::string module_id;
  Verdict verdict{Verdict::inconclusive};
  std::string details;
  Json evidence = Json::object();
};

struct CampaignConfig {
  std::string profile{"level1"};
  std::vector<std::string> modules;
  std::uint64_t seed{1};
  std::uint32_t unique_states{1};
  std::uint32_t rounds_per_state{1};
  std::uint32_t dwell_ms{1000};
  std::uint32_t dwell_min_ms{1};
  std::uint32_t dwell_max_ms{5000};
  std::uint32_t sample_rate{48000};
  std::uint32_t block_size{64};
  std::uint32_t timeout_ms{5000};
  std::uint32_t max_cases{1000};
  std::optional<std::string> fixture_fault;
  bool preview_only{false};
};

struct CampaignResult {
  std::string campaign_id;
  std::string target_path;
  std::string target_sha256;
  std::string profile;
  std::uint64_t seed{0};
  std::uint64_t planned_cases{0};
  std::uint64_t completed_cases{0};
  Json configuration = Json::object();
  std::vector<Json> cases;
  std::vector<std::string> selected_modules;
};

struct CampaignRequest {
  std::filesystem::path target;
  std::filesystem::path worker;
  std::filesystem::path output_dir;
  CampaignConfig config;
  std::optional<Json> replay_snapshot;
};

std::vector<TestModule> registered_modules();
std::vector<std::string> profile_modules(const std::string& profile);
void validate_config(const CampaignConfig& config);
std::uint64_t planned_case_count(const CampaignConfig& config,
                                 std::size_t selected_module_count);
CampaignResult run_campaign(const CampaignRequest& request);
CampaignResult replay_case(const CampaignRequest& request,
                           const std::filesystem::path& saved_case,
                           bool allow_target_mismatch = false);
void write_html_report(const std::filesystem::path& json_path,
                       const std::filesystem::path& html_path);
std::string sha256_file(const std::filesystem::path& path);
}
