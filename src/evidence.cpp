#include "internal.hpp"
#include <algorithm>
#include <fstream>
#include <random>
#include <sstream>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace soundgrind::internal {
Json load_json(const std::filesystem::path& p) {
  std::ifstream in(p); if (!in) throw std::runtime_error("cannot open JSON: " + p.string());
  return Json::parse(in);
}
void write_json_atomic(const std::filesystem::path& p, const Json& j) {
  if(!p.parent_path().empty())std::filesystem::create_directories(p.parent_path());
  auto tmp=p; tmp += ".tmp";
  { std::ofstream out(tmp,std::ios::binary|std::ios::trunc); if(!out)throw std::runtime_error("cannot write: "+tmp.string()); out<<j.dump(2)<<'\n'; out.flush(); if(!out)throw std::runtime_error("failed to flush: "+tmp.string()); }
#ifdef _WIN32
  HANDLE h=CreateFileW(tmp.c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
  if(h==INVALID_HANDLE_VALUE)throw std::runtime_error("cannot reopen evidence file for durable flush");
  const bool synced=FlushFileBuffers(h)!=0;CloseHandle(h);if(!synced)throw std::runtime_error("FlushFileBuffers failed for evidence file");
#else
  int fd=open(tmp.c_str(),O_RDWR);if(fd<0)throw std::runtime_error("cannot reopen evidence file for durable flush");
  const bool synced=fsync(fd)==0;close(fd);if(!synced)throw std::runtime_error("fsync failed for evidence file");
#endif
#ifdef _WIN32
  std::error_code ec; std::filesystem::remove(p,ec);
#endif
  std::filesystem::rename(tmp,p);
}
std::string make_id(std::uint64_t seed,std::uint64_t i) { std::ostringstream s; s<<std::hex<<seed<<'-'<<i; return s.str(); }

Json make_snapshot(const Json& descriptors, const CampaignConfig& c,
                   std::uint64_t state_index, const std::string& fault) {
  Json values=Json::object(); Json excluded=Json::array();
  std::mt19937_64 rng(c.seed ^ (0x9e3779b97f4a7c15ULL*(state_index+1)));
  for(const auto& p:descriptors.value("parameters",Json::array())) {
    if(!p.value("writable",false)) { excluded.push_back({{"id",p.value("id","")},{"reason","parameter is not writable"}}); continue; }
    const auto id=p.value("id",std::string{});
    const double lo=p.value("min",0.0), hi=p.value("max",1.0), step=p.value("step",0.0), def=p.value("default",lo);
    double v=def;
    if(state_index>0) {
      if(step>0 && hi>=lo) {
        const auto first=static_cast<std::int64_t>(std::ceil(lo));
        const auto last=static_cast<std::int64_t>(std::floor(hi));
        if(last>=first){std::uniform_int_distribution<std::int64_t> d(first,last);v=static_cast<double>(d(rng));}
      } else if(hi>lo) { std::uniform_real_distribution<double> d(lo,hi); v=d(rng); }
    }
    if(id=="fixture.fault") {
      if(fault=="nan")v=1; else if(fault=="crash")v=2; else if(fault=="hang")v=3; else if(fault=="oracle")v=4;
      else v=0;
    }
    values[id]=v;
  }
  return {{"values",values},{"excluded",excluded},{"seed",c.seed ^ (0x9e3779b97f4a7c15ULL*(state_index+1))}};
}

Json make_case_record(const CampaignRequest& req,const CampaignResult& campaign,
                      std::uint64_t state,std::uint64_t round,const Json& snapshot,
                      std::uint64_t index) {
  const auto sample_target=(static_cast<std::uint64_t>(req.config.dwell_ms)*req.config.sample_rate+999)/1000;
  const auto blocks=(sample_target+req.config.block_size-1)/req.config.block_size;
  const auto samples=blocks*req.config.block_size;
  return {{"schema",kSchemaVersion},{"engine_version",kEngineVersion},
    {"campaign_id",campaign.campaign_id},{"case_id",make_id(req.config.seed,index)},
    {"case_index",index},{"target_path",req.target.string()},{"target_sha256",campaign.target_sha256},
    {"profile",req.config.profile},{"module_ids",campaign.selected_modules},
    {"state_index",state},{"round_index",round},{"campaign_seed",req.config.seed},
    {"case_seed",snapshot.value("seed",req.config.seed)},{"generator_version","uniform-domain-v1"},
    {"parameter_snapshot",snapshot.value("values",Json::object())},
    {"excluded_parameters",snapshot.value("excluded",Json::array())},
    {"sample_rate",req.config.sample_rate},{"block_size",req.config.block_size},
    {"requested_dwell_ms",req.config.dwell_ms},{"planned_samples",samples},
    {"effective_dwell_ms",1000.0*static_cast<double>(samples)/req.config.sample_rate},
    {"fixture_id","constant-0.25-mono-v1"},{"target_format","CLAP"},
    {"module_versions",[&](){Json v=Json::object();for(const auto& m:registered_modules())if(std::find(campaign.selected_modules.begin(),campaign.selected_modules.end(),m.id)!=campaign.selected_modules.end())v[m.id]=m.version;return v;}()},
    {"fixture_fault",req.config.fixture_fault.value_or("none")},{"status","PREPARED"},
    {"module_results",Json::array()},{"artifacts",Json::array()}};
}
}
