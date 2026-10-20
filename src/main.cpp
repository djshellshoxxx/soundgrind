#include "internal.hpp"
#include <iostream>
#include <map>
#include <set>
#include <sstream>

namespace {
using namespace soundgrind;
std::vector<std::string> split(const std::string& s,char delim) { std::vector<std::string> v;std::stringstream ss(s);std::string x;while(std::getline(ss,x,delim))if(!x.empty())v.push_back(x);return v; }
void help(){std::cout<<"SoundGrind M1 (CLAP, offline)\n\n"
  <<"Commands:\n  inspect --target PLUGIN.clap [--worker PATH]\n"
  <<"  test --target PLUGIN.clap [--profile level1|level2|level3|fuzz1|fuzz2|grind] [options]\n"
  <<"  replay --target PLUGIN.clap --case CASE.json [--out DIR] [--allow-target-mismatch]\n"
  <<"  report --input campaign.json --out report.html\n\n"
  <<"Test options: --modules ID,ID --states N --rounds N --dwell-ms N --sample-rate N\n"
  <<"              --block-size N --seed N --timeout-ms N --max-cases N --fixture-fault nan|crash|hang|oracle\n"
  <<"              --worker PATH --out DIR --preview\n";}
std::uint64_t number(const std::string& s,const std::string& name){std::size_t n=0;auto x=std::stoull(s,&n);if(n!=s.size())throw std::invalid_argument("invalid number for "+name);return x;}
std::uint32_t number32(const std::string& s,const std::string& name){auto n=number(s,name);if(n>UINT32_MAX)throw std::invalid_argument(name+" exceeds 32-bit range");return static_cast<std::uint32_t>(n);}
Json inspect(const std::filesystem::path& target,const std::filesystem::path& worker,const std::filesystem::path& out,std::uint32_t timeout){
  std::filesystem::create_directories(out);auto req=out/"inspect.request.json",res=out/"inspect.response.json",err=out/"inspect.stderr.log";
  internal::write_json_atomic(req,{{"operation","inspect"},{"target",target.string()}});
  auto c=internal::run_child(worker,{"--request",req.string(),"--result",res.string()},timeout,err);
  if(c.timed_out||c.exit_code!=0)throw std::runtime_error("CLAP inspection worker failed; stderr: "+err.string());return internal::load_json(res);
}
bool any_failure(const Json& j){for(const auto& c:j.value("cases",Json::array()))for(const auto& m:c.value("module_results",Json::array()))if(m.value("verdict","")=="FAIL"||m.value("verdict","")=="INFRASTRUCTURE_ERROR")return true;return false;}
}

int main(int argc,char** argv){
  try {
    if(argc<2){help();return 2;}const std::string command=argv[1];if(command=="--help"||command=="help"){help();return 0;}
    std::map<std::string,std::string> args;std::set<std::string> switches{"--preview","--allow-target-mismatch"};
    for(int i=2;i<argc;++i){std::string key=argv[i];if(key.rfind("--",0)!=0)throw std::invalid_argument("unexpected argument: "+key);if(switches.count(key)){args[key]="true";continue;}if(i+1>=argc)throw std::invalid_argument("missing value for "+key);args[key]=argv[++i];}
    auto get=[&](const std::string& k,const std::string& d=std::string{}){auto it=args.find(k);return it==args.end()?d:it->second;};
    if(command=="report"){if(get("--input").empty()||get("--out").empty())throw std::invalid_argument("report requires --input and --out");write_html_report(get("--input"),get("--out"));std::cout<<"Wrote "<<get("--out")<<"\n";return 0;}
    const auto target=get("--target");if(target.empty())throw std::invalid_argument(command+" requires --target");
    auto executable=std::filesystem::absolute(argv[0]);std::string worker_name="soundgrind_worker";
#ifdef _WIN32
    worker_name+=".exe";
#endif
    auto worker=std::filesystem::path(get("--worker",(executable.parent_path()/worker_name).string()));
    CampaignConfig c;c.profile=get("--profile","level1");c.seed=number(get("--seed","1"),"seed");c.unique_states=number32(get("--states",(c.profile=="level1"||c.profile=="smoke")?"1":"10"),"states");
    c.rounds_per_state=number32(get("--rounds",(c.profile=="level1"||c.profile=="smoke")?"3":"1"),"rounds");c.dwell_ms=number32(get("--dwell-ms","1000"),"dwell-ms");c.dwell_min_ms=number32(get("--dwell-min-ms","1"),"dwell-min-ms");c.dwell_max_ms=number32(get("--dwell-max-ms","5000"),"dwell-max-ms");c.sample_rate=number32(get("--sample-rate","48000"),"sample-rate");c.block_size=number32(get("--block-size","64"),"block-size");c.timeout_ms=number32(get("--timeout-ms","5000"),"timeout-ms");c.max_cases=number32(get("--max-cases","1000"),"max-cases");c.preview_only=args.count("--preview")!=0;
    if(args.count("--modules")){c.modules=split(get("--modules"),',');if(c.modules.empty())throw std::invalid_argument("--modules needs at least one module ID");}if(args.count("--fixture-fault")){c.fixture_fault=get("--fixture-fault");if(*c.fixture_fault!="nan"&&*c.fixture_fault!="crash"&&*c.fixture_fault!="hang"&&*c.fixture_fault!="oracle")throw std::invalid_argument("unknown fixture fault mode");}
    const auto output=std::filesystem::path(get("--out","soundgrind-results"));
    if(command=="inspect"){auto j=inspect(target,worker,output,c.timeout_ms);j["target_sha256"]=sha256_file(target);std::cout<<j.dump(2)<<'\n';return 0;}
    CampaignRequest req{target,worker,output,c,{}};
    if(command=="test"){run_campaign(req);auto j=internal::load_json(output/"campaign.json");std::cout<<j.dump(2)<<'\n';return any_failure(j)?1:0;}
    if(command=="replay"){const auto case_file=get("--case");if(case_file.empty())throw std::invalid_argument("replay requires --case CASE.json");replay_case(req,case_file,args.count("--allow-target-mismatch")!=0);auto j=internal::load_json(output/"campaign.json");std::cout<<j.dump(2)<<'\n';return any_failure(j)?1:0;}
    throw std::invalid_argument("unknown command: "+command);
  }catch(const std::exception& e){std::cerr<<"soundgrind: "<<e.what()<<'\n';return 2;}
}
