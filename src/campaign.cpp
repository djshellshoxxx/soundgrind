#include "internal.hpp"
#include <algorithm>
#include <random>
#include <random>
#include <sstream>

namespace soundgrind {
namespace {
Json worker_call(const CampaignRequest& r,const Json& payload,const std::string& stem) {
  const auto base=r.output_dir/"worker";std::filesystem::create_directories(base);
  const auto request=base/(stem+".request.json"),response=base/(stem+".response.json"),err=base/(stem+".stderr.log");
  internal::write_json_atomic(request,payload);
  auto child=internal::run_child(r.worker,{"--request",request.string(),"--result",response.string()},r.config.timeout_ms,err);
  if(child.timed_out)throw std::runtime_error("worker_timeout:"+std::to_string(child.exit_code)+":"+err.string());
  if(child.exit_code!=0)throw std::runtime_error("worker_exit_"+std::to_string(child.exit_code)+":"+err.string());
  return internal::load_json(response);
}
std::vector<std::string> choose_modules(const CampaignConfig& c) {
  auto ids=c.modules.empty()?profile_modules(c.profile):c.modules;
  if(c.profile=="fuzz2") { std::mt19937_64 g(c.seed);std::shuffle(ids.begin(),ids.end(),g);const auto count=std::max<std::size_t>(1,ids.size()/2);ids.resize(count);std::sort(ids.begin(),ids.end()); }
  std::sort(ids.begin(),ids.end());ids.erase(std::unique(ids.begin(),ids.end()),ids.end());return ids;
}
Json module_outcomes(const Json& observed,const std::vector<std::string>& selected) {
  Json out=Json::array();std::vector<std::string> seen;
  for(const auto& id:selected){bool found=false;for(const auto& m:observed.value("module_results",Json::array()))if(m.value("module_id","")==id){out.push_back(m);seen.push_back(id);found=true;break;}
    if(!found)out.push_back({{"module_id",id},{"verdict","UNSUPPORTED"},{"details","No applicable observation was returned by the CLAP adapter."},{"evidence",Json::object()}});}
  return out;
}
}

CampaignResult run_campaign(const CampaignRequest& request) {
  validate_config(request.config);
  if(request.target.empty()||!std::filesystem::exists(request.target))throw std::invalid_argument("target path does not exist");
  if(request.worker.empty()||!std::filesystem::exists(request.worker))throw std::invalid_argument("worker executable does not exist");
  if(request.output_dir.empty())throw std::invalid_argument("output directory is required");
  CampaignResult result;result.campaign_id=internal::make_id(request.config.seed,static_cast<std::uint64_t>(std::chrono::system_clock::now().time_since_epoch().count()));
  result.target_path=request.target.string();result.target_sha256=sha256_file(request.target);result.profile=request.config.profile;result.seed=request.config.seed;result.selected_modules=choose_modules(request.config);
  if(result.selected_modules.empty())throw std::invalid_argument("at least one test module must be selected");
  for(const auto& id:result.selected_modules){auto ms=registered_modules();if(std::none_of(ms.begin(),ms.end(),[&](const auto& m){return m.id==id;}))throw std::invalid_argument("unknown test module: "+id);}
  result.planned_cases=planned_case_count(request.config,result.selected_modules.size());
  if(result.planned_cases>request.config.max_cases)throw std::invalid_argument("planned cases exceed --max-cases; lower states/rounds or raise the explicit budget");
  result.configuration={{"schema",kSchemaVersion},{"profile",request.config.profile},{"seed",request.config.seed},
    {"unique_states",request.config.unique_states},{"rounds_per_state",request.config.rounds_per_state},{"dwell_ms",request.config.dwell_ms},
    {"dwell_min_ms",request.config.dwell_min_ms},{"dwell_max_ms",request.config.dwell_max_ms},
    {"sample_rate",request.config.sample_rate},{"block_size",request.config.block_size},{"timeout_ms",request.config.timeout_ms},
    {"max_cases",request.config.max_cases},{"modules",result.selected_modules},{"fixture_fault",request.config.fixture_fault.value_or("none")}};
  std::filesystem::create_directories(request.output_dir);
  Json campaign={{"schema",kSchemaVersion},{"engine_version",kEngineVersion},{"campaign_id",result.campaign_id},
    {"target",{{"path",result.target_path},{"sha256",result.target_sha256}}},{"profile",result.profile},
    {"configuration",result.configuration},{"planned_cases",result.planned_cases},{"completed_cases",0},{"cases",Json::array()}};
  internal::write_json_atomic(request.output_dir/"campaign.json",campaign);
  Json inspection;
  try { inspection=worker_call(request,{{"operation","inspect"},{"target",result.target_path}},"inspect"); }
  catch(const std::exception& e) {throw std::runtime_error(std::string("target inspection failed: ")+e.what());}
  internal::write_json_atomic(request.output_dir/"target.json",inspection);
  if(request.config.preview_only){
    campaign["preview"]={{"unique_states",request.config.unique_states},{"rounds_per_state",request.config.rounds_per_state},{"planned_cases",result.planned_cases},{"max_cases",request.config.max_cases},{"selected_modules",result.selected_modules},{"target",inspection}};
    internal::write_json_atomic(request.output_dir/"campaign.json",campaign);return result;
  }
  std::uint64_t index=0;
  for(std::uint64_t state=0;state<request.config.unique_states;++state) {
    const bool random_first=request.config.profile=="level3"||request.config.profile=="stress"||request.config.profile=="fuzz1"||request.config.profile=="fuzz2"||request.config.profile=="grind";
    const auto snapshot=request.replay_snapshot.value_or(internal::make_snapshot(inspection,request.config,state+(random_first?1:0),request.config.fixture_fault.value_or("")));
    for(std::uint64_t round=0;round<request.config.rounds_per_state;++round,++index) {
      CampaignRequest case_request=request;
      if(request.config.profile=="grind") { std::mt19937_64 dwell_rng(request.config.seed ^ (0xd6e8feb86659fd93ULL*(index+1))); std::uniform_int_distribution<std::uint32_t> d(request.config.dwell_min_ms,request.config.dwell_max_ms); case_request.config.dwell_ms=d(dwell_rng); }
      auto record=internal::make_case_record(case_request,result,state,round,snapshot,index);
      record["plugin_identity"]={{"id",inspection.value("plugin_id",std::string{})},{"name",inspection.value("name",std::string{})},{"vendor",inspection.value("vendor",std::string{})},{"version",inspection.value("version",std::string{})}};
      record["prepared_at_unix_ms"]=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
      record["last_worker_event"]="case record flushed before worker launch";
      if(request.config.profile=="grind")record["randomized_dwell_bounds_ms"]={{"min",request.config.dwell_min_ms},{"max",request.config.dwell_max_ms}};
      record["status"]="PREPARED";
      const auto case_path=request.output_dir/"cases"/(record["case_id"].get<std::string>()+".json");
      internal::write_json_atomic(case_path,record); // write-ahead evidence before the plugin worker starts
      try {
        auto observed=worker_call(case_request,{{"operation","test"},{"target",result.target_path},{"sample_rate",case_request.config.sample_rate},{"block_size",case_request.config.block_size},{"dwell_ms",case_request.config.dwell_ms},{"parameter_snapshot",snapshot["values"]}},"case-"+std::to_string(index));
        record["module_results"]=module_outcomes(observed,result.selected_modules);
        record["processed_samples"]=observed.value("processed_samples",record.value("planned_samples",0ull));
        record["actual_dwell_ms"]=observed.value("actual_dwell_ms",record.value("effective_dwell_ms",0.0));
        record["worker_exit_code"]=0;record["worker_stderr_path"]=(request.output_dir/"worker"/("case-"+std::to_string(index)+".stderr.log")).string();
        record["last_worker_event"]="worker exited after writing response";
        const bool failed=std::any_of(record["module_results"].begin(),record["module_results"].end(),[](const auto& m){auto v=m.value("verdict","");return v=="FAIL"||v=="INFRASTRUCTURE_ERROR";});
        record["status"]=failed?"FAIL":"COMPLETED";
        if(failed) { for(const auto& m:record["module_results"])if(m.value("verdict","")=="FAIL"){const auto id=m.value("module_id","");record["classification"]=id=="audio.finite_output"?"non_finite_output":id=="fixture.oracle"?"fixture_oracle_mismatch":id=="params.readback"?"parameter_application_mismatch":id=="state.roundtrip"?"state_roundtrip_failure":"module_failure";break;} }
      } catch(const std::exception& e) {
        const std::string why=e.what();record["status"]="FAIL";record["classification"]=why.rfind("worker_timeout:",0)==0?"hang_or_timeout":(why.rfind("worker_exit_",0)==0?"worker_crash":"worker_error");
        record["details"]=why;record["worker_exit_code"]=nullptr;
        if(why.rfind("worker_timeout:",0)==0){try{record["worker_exit_code"]=std::stoi(why.substr(15,why.find(':',15)-15));}catch(...) {}}
        else if(why.rfind("worker_exit_",0)==0){try{record["worker_exit_code"]=std::stoi(why.substr(12,why.find(':',12)-12));}catch(...) {}}
        record["worker_stderr_path"]=(request.output_dir/"worker"/("case-"+std::to_string(index)+".stderr.log")).string();
        record["last_worker_event"]=record["classification"]=="hang_or_timeout"?"supervisor terminated worker after deadline":"worker exited without a successful response";
        for(const auto& id:result.selected_modules)record["module_results"].push_back({{"module_id",id},{"verdict",record["classification"]=="hang_or_timeout"||record["classification"]=="worker_crash"?"FAIL":"INFRASTRUCTURE_ERROR"},{"details",why},{"evidence",{{"case_record",case_path.string()},{"stderr",record["worker_stderr_path"]}}}});
      }
      internal::write_json_atomic(case_path,record);
      result.cases.push_back(record);++result.completed_cases;campaign["completed_cases"]=result.completed_cases;campaign["cases"].push_back(record);
      internal::write_json_atomic(request.output_dir/"campaign.json",campaign);
    }
  }
  std::uint64_t pass=0,fail=0,unsupported=0;
  for(const auto& c:result.cases)for(const auto& m:c.value("module_results",Json::array())){auto v=m.value("verdict","");if(v=="PASS")++pass;else if(v=="FAIL")++fail;else if(v=="UNSUPPORTED")++unsupported;}
  campaign["summary"]={{"pass",pass},{"fail",fail},{"unsupported",unsupported},{"planned_cases",result.planned_cases},{"completed_cases",result.completed_cases}};
  internal::write_json_atomic(request.output_dir/"campaign.json",campaign);
  write_html_report(request.output_dir/"campaign.json",request.output_dir/"report.html");
  return result;
}

CampaignResult replay_case(const CampaignRequest& request,const std::filesystem::path& saved,bool allow_mismatch) {
  auto source=internal::load_json(saved);const auto hash=sha256_file(request.target);
  if(hash!=source.value("target_sha256",std::string{})&&!allow_mismatch)throw std::runtime_error("target SHA-256 mismatch; replay stopped (use --allow-target-mismatch to record an override)");
  CampaignRequest replay=request;replay.config.profile=source.value("profile",request.config.profile);replay.config.seed=source.value("case_seed",request.config.seed);replay.config.dwell_ms=source.value("requested_dwell_ms",request.config.dwell_ms);replay.config.sample_rate=source.value("sample_rate",request.config.sample_rate);replay.config.block_size=source.value("block_size",request.config.block_size);replay.config.unique_states=1;replay.config.rounds_per_state=1;
  replay.config.modules=source.value("module_ids",std::vector<std::string>{});
  replay.replay_snapshot=Json{{"values",source.value("parameter_snapshot",Json::object())},{"excluded",source.value("excluded_parameters",Json::array())},{"seed",source.value("case_seed",request.config.seed)}};
  const auto out=request.output_dir;std::filesystem::create_directories(out);
  auto completed=run_campaign(replay);
  auto json=internal::load_json(out/"campaign.json");json["replay_source"]=std::filesystem::absolute(saved).string();json["source_target_sha256"]=source.value("target_sha256",std::string{});json["target_hash_override"]=allow_mismatch&&hash!=source.value("target_sha256",std::string{});json["replay_source_case_id"]=source.value("case_id",std::string{});internal::write_json_atomic(out/"campaign.json",json);write_html_report(out/"campaign.json",out/"report.html");
  return completed;
}
}
