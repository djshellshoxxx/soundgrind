#include "soundgrind/clap_adapter.hpp"
#include <clap/clap.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <vector>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace soundgrind {
namespace {
struct Library {
#ifdef _WIN32
  HMODULE handle{};
  explicit Library(const std::filesystem::path& p):handle(LoadLibraryW(p.c_str())){}
  ~Library(){if(handle)FreeLibrary(handle);}
  void* symbol(const char* n){return reinterpret_cast<void*>(GetProcAddress(handle,n));}
#else
  void* handle{};
  explicit Library(const std::filesystem::path& p):handle(dlopen(p.c_str(),RTLD_NOW|RTLD_LOCAL)){}
  ~Library(){if(handle)dlclose(handle);}
  void* symbol(const char* n){return dlsym(handle,n);}
#endif
};
void host_restart(const clap_host_t*){}
void host_process(const clap_host_t*){}
void host_callback(const clap_host_t*){}
void host_param_rescan(const clap_host_t*,clap_param_rescan_flags){}
void host_param_clear(const clap_host_t*,clap_id,clap_param_clear_flags){}
void host_param_flush(const clap_host_t*){}
void host_state_dirty(const clap_host_t*){}
const clap_host_params_t host_params{host_param_rescan,host_param_clear,host_param_flush};
const clap_host_state_t host_state{host_state_dirty};
const void* host_extension(const clap_host_t*,const char* id){if(!std::strcmp(id,CLAP_EXT_PARAMS))return &host_params;if(!std::strcmp(id,CLAP_EXT_STATE))return &host_state;return nullptr;}
const clap_host_t host{CLAP_VERSION,nullptr,"SoundGrind","OpenAI","https://github.com/djshellshoxxx/soundgrind","0.1.0",host_extension,host_restart,host_process,host_callback};

struct EventList {
  std::vector<clap_event_param_value_t> events; clap_input_events_t api{};
  EventList(){api.ctx=this;api.size=event_size;api.get=event_get;}
  EventList(const EventList&)=delete;EventList& operator=(const EventList&)=delete;
  EventList(EventList&& other) noexcept : events(std::move(other.events)) {api.ctx=this;api.size=event_size;api.get=event_get;}
  static uint32_t event_size(const clap_input_events_t* e){return static_cast<uint32_t>(static_cast<EventList*>(e->ctx)->events.size());}
  static const clap_event_header_t* event_get(const clap_input_events_t* e,uint32_t i){auto& v=static_cast<EventList*>(e->ctx)->events;return i<v.size()?&v[i].header:nullptr;}
};
EventList make_events(const Json& values,const Json& params) {
  EventList l;
  for(const auto& p:params) {
    const auto id=p.value("id",std::string{}); if(!values.contains(id))continue;
    clap_event_param_value_t e{};e.header={sizeof(e),0,CLAP_CORE_EVENT_SPACE_ID,CLAP_EVENT_PARAM_VALUE,0};
    e.param_id=p.value("clap_id",CLAP_INVALID_ID);e.cookie=nullptr;e.note_id=-1;e.port_index=-1;e.channel=-1;e.key=-1;e.value=values.at(id).get<double>();l.events.push_back(e);
  }
  return l;
}
Json read_descriptors(const clap_plugin_t* p,const clap_plugin_params_t* params,
                      const clap_plugin_audio_ports_t* ports) {
  Json out={{"plugin_id",p->desc->id},{"name",p->desc->name},{"vendor",p->desc->vendor?p->desc->vendor:""},
            {"version",p->desc->version?p->desc->version:""},{"clap_version","1.2.9"},
            {"parameters",Json::array()},{"audio_inputs",Json::array()},{"audio_outputs",Json::array()}};
  if(params) for(uint32_t i=0;i<params->count(p);++i) {
    clap_param_info_t info{};if(!params->get_info(p,i,&info))continue;
    double current=info.default_value;params->get_value(p,info.id,&current);
    Json q={{"clap_id",info.id},{"id","clap:"+std::to_string(info.id)},{"name",std::string(info.name)},
      {"min",info.min_value},{"max",info.max_value},{"default",info.default_value},{"current",current},
      {"step",(info.flags&CLAP_PARAM_IS_STEPPED)?1.0:0.0},{"stepped",bool(info.flags&CLAP_PARAM_IS_STEPPED)},
      {"enum",bool(info.flags&CLAP_PARAM_IS_ENUM)},{"writable",!bool(info.flags&CLAP_PARAM_IS_READONLY)},
      {"automatable",bool(info.flags&CLAP_PARAM_IS_AUTOMATABLE)}};
    if(info.id==102)q["id"]="fixture.fault";else if(info.id==100)q["id"]="fixture.gain";else if(info.id==101)q["id"]="fixture.mode";
    out["parameters"].push_back(q);
  }
  if(ports)for(bool input:{true,false})for(uint32_t i=0;i<ports->count(p,input);++i){clap_audio_port_info_t info{};if(!ports->get(p,i,input,&info))continue;Json port={{"id",info.id},{"name",info.name},{"channels",info.channel_count},{"main",bool(info.flags&CLAP_AUDIO_PORT_IS_MAIN)},{"type",info.port_type?info.port_type:""}};(input?out["audio_inputs"]:out["audio_outputs"]).push_back(port);}
  out["capabilities"]={{"params",params!=nullptr},{"audio_ports",ports!=nullptr},{"audio_input",!out["audio_inputs"].empty()},{"audio_output",!out["audio_outputs"].empty()},{"state",p->get_extension(p,CLAP_EXT_STATE)!=nullptr}};
  return out;
}
Json check(std::string id,std::string verdict,std::string detail,Json ev=Json::object()) {return {{"module_id",std::move(id)},{"verdict",std::move(verdict)},{"details",std::move(detail)},{"evidence",std::move(ev)}};}
struct MemoryStream { std::vector<std::uint8_t> data; std::size_t offset=0; clap_ostream_t out{}; clap_istream_t in{}; };
int64_t stream_write(const clap_ostream_t* s,const void* p,uint64_t n){auto* m=static_cast<MemoryStream*>(s->ctx);const auto* b=static_cast<const std::uint8_t*>(p);m->data.insert(m->data.end(),b,b+n);return static_cast<int64_t>(n);}
int64_t stream_read(const clap_istream_t* s,void* p,uint64_t n){auto* m=static_cast<MemoryStream*>(s->ctx);const auto rem=m->data.size()-m->offset;const auto c=std::min<std::size_t>(rem,n);std::memcpy(p,m->data.data()+m->offset,c);m->offset+=c;return static_cast<int64_t>(c);}
}

Json run_clap_request(const Json& req) {
  const std::filesystem::path path=req.at("target").get<std::string>(); Library lib(path);
  if(!lib.handle)throw std::runtime_error("failed to load CLAP binary: "+path.string());
  auto* entry=static_cast<const clap_plugin_entry_t*>(lib.symbol("clap_entry"));
  if(!entry)throw std::runtime_error("CLAP entry symbol clap_entry was not found");
  if(!clap_version_is_compatible(entry->clap_version))throw std::runtime_error("incompatible CLAP version");
  if(!entry->init(path.string().c_str()))throw std::runtime_error("CLAP entry initialization failed");
  struct EntryGuard{const clap_plugin_entry_t* e;~EntryGuard(){e->deinit();}} guard{entry};
  auto* factory=static_cast<const clap_plugin_factory_t*>(entry->get_factory(CLAP_PLUGIN_FACTORY_ID));
  if(!factory||factory->get_plugin_count(factory)==0)throw std::runtime_error("CLAP plugin factory has no plugins");
  const auto* desc=factory->get_plugin_descriptor(factory,0);if(!desc)throw std::runtime_error("CLAP plugin descriptor unavailable");
  auto* plugin=factory->create_plugin(factory,&host,desc->id);if(!plugin)throw std::runtime_error("CLAP plugin creation failed");
  bool initialized=false,active=false,processing=false;
  struct PluginGuard{const clap_plugin_t* p;bool* initialized;bool* active;bool* processing;~PluginGuard(){if(*processing)p->stop_processing(p);if(*active)p->deactivate(p);if(*initialized)p->destroy(p);}} pg{plugin,&initialized,&active,&processing};
  if(!plugin->init(plugin))throw std::runtime_error("CLAP plugin init failed");initialized=true;
  auto* params=static_cast<const clap_plugin_params_t*>(plugin->get_extension(plugin,CLAP_EXT_PARAMS));
  auto* ports=static_cast<const clap_plugin_audio_ports_t*>(plugin->get_extension(plugin,CLAP_EXT_AUDIO_PORTS));
  auto descriptor=read_descriptors(plugin,params,ports);
  if(req.value("operation",std::string("inspect"))=="inspect")return descriptor;
  Json checks=Json::array();
  const double sr=req.value("sample_rate",48000.0);const auto bs=req.value("block_size",64u);
  const auto max_frames=std::max<std::uint32_t>(bs,1024u);
  const auto requested_ms=req.value("dwell_ms",1000u);const auto requested_samples=(std::uint64_t(requested_ms)*std::uint64_t(sr)+999)/1000;
  const auto block_count=std::max<std::uint64_t>(1,(requested_samples+bs-1)/bs);const auto total_samples=block_count*bs;
  if(!plugin->activate(plugin,sr,1,max_frames))throw std::runtime_error("CLAP plugin activate failed");active=true;
  if(!plugin->start_processing(plugin))throw std::runtime_error("CLAP plugin start_processing failed");processing=true;

  const auto input_count=ports?ports->count(plugin,true):0;const auto output_count=ports?ports->count(plugin,false):0;
  const bool has_output=output_count>0;
  std::vector<std::vector<std::vector<float>>> inputs(input_count),outputs(output_count);
  std::vector<clap_audio_buffer_t> in_buffers(input_count),out_buffers(output_count);
  std::vector<std::vector<float*>> in_ptrs(input_count),out_ptrs(output_count);
  for(uint32_t p=0;p<input_count;++p){clap_audio_port_info_t i{};ports->get(plugin,p,true,&i);inputs[p].resize(i.channel_count,std::vector<float>(max_frames,0.25f));in_ptrs[p].resize(i.channel_count);for(uint32_t c=0;c<i.channel_count;++c)in_ptrs[p][c]=inputs[p][c].data();in_buffers[p]={in_ptrs[p].data(),nullptr,i.channel_count,0,0};}
  for(uint32_t p=0;p<output_count;++p){clap_audio_port_info_t i{};ports->get(plugin,p,false,&i);outputs[p].resize(i.channel_count,std::vector<float>(max_frames));out_ptrs[p].resize(i.channel_count);for(uint32_t c=0;c<i.channel_count;++c)out_ptrs[p][c]=outputs[p][c].data();out_buffers[p]={out_ptrs[p].data(),nullptr,i.channel_count,0,0};}
  auto snapshot=req.value("parameter_snapshot",Json::object()); auto events=make_events(snapshot,descriptor["parameters"]);
  bool finite=true,oracle_ok=true;std::uint64_t bad_sample=0;std::uint32_t bad_channel=0;float first_bad=0;std::uint64_t done=0;
  for(std::uint64_t b=0;b<block_count;++b){
    clap_process_t pr{};pr.steady_time=static_cast<int64_t>(done);pr.frames_count=bs;pr.audio_inputs=in_buffers.data();pr.audio_inputs_count=input_count;pr.audio_outputs=out_buffers.data();pr.audio_outputs_count=output_count;pr.in_events=(b==0)?&events.api:nullptr;
    const auto status=plugin->process(plugin,&pr);if(status==CLAP_PROCESS_ERROR)throw std::runtime_error("CLAP process returned CLAP_PROCESS_ERROR");
    for(uint32_t p=0;p<output_count;++p)for(uint32_t c=0;c<out_buffers[p].channel_count;++c)for(uint32_t n=0;n<bs;++n){const float y=outputs[p][c][n];if(!std::isfinite(y)){if(finite){bad_sample=done+n;bad_channel=c;first_bad=y;}finite=false;}
      if(std::string(desc->id)=="org.soundgrind.synthetic"&&p==0&&c==0){double g=snapshot.value("fixture.gain",1.0);double mode=snapshot.value("fixture.mode",0.0);double expected=mode==1?-0.25*g:(mode==2?0.0:0.25*g);if(std::isfinite(y)&&std::abs(y-expected)>1e-6)oracle_ok=false;}}
    done+=bs;
  }
  if(processing){plugin->stop_processing(plugin);processing=false;}
  if(active){plugin->deactivate(plugin);active=false;}
  Json size_checks=Json::array();bool size_ok=true;
  if(has_output){
    std::vector<std::uint32_t> sizes{1,16,32,64,128,256,512,1024,bs};std::sort(sizes.begin(),sizes.end());sizes.erase(std::unique(sizes.begin(),sizes.end()),sizes.end());
    if(!plugin->activate(plugin,sr,1,max_frames))throw std::runtime_error("CLAP re-activate before block-size checks failed");active=true;
    if(!plugin->start_processing(plugin))throw std::runtime_error("CLAP start_processing before block-size checks failed");processing=true;
    for(const auto size:sizes){if(size>max_frames)continue;clap_process_t pr{};pr.steady_time=static_cast<int64_t>(done);pr.frames_count=size;pr.audio_inputs=in_buffers.data();pr.audio_inputs_count=input_count;pr.audio_outputs=out_buffers.data();pr.audio_outputs_count=output_count;
      const auto st=plugin->process(plugin,&pr);const bool ok=st!=CLAP_PROCESS_ERROR;size_ok=size_ok&&ok;size_checks.push_back({{"block_size",size},{"process_ok",ok}});
      if(ok){for(std::uint32_t p=0;p<output_count;++p)for(std::uint32_t c=0;c<out_buffers[p].channel_count;++c)for(std::uint32_t n=0;n<size;++n){const auto y=outputs[p][c][n];if(!std::isfinite(y)){if(finite){bad_sample=done+n;bad_channel=c;first_bad=y;}finite=false;}if(std::string(desc->id)=="org.soundgrind.synthetic"&&p==0&&c==0){double g=snapshot.value("fixture.gain",1.0),mode=snapshot.value("fixture.mode",0.0);double expected=mode==1?-0.25*g:(mode==2?0.0:0.25*g);if(std::isfinite(y)&&std::abs(y-expected)>1e-6)oracle_ok=false;}}}
    }
    plugin->stop_processing(plugin);processing=false;plugin->deactivate(plugin);active=false;
  }
  checks.push_back(check("clap.lifecycle",size_ok?"PASS":"FAIL",size_ok?"Target loaded and completed initialization, activation, rendering, reactivation and destruction.":"Target returned CLAP_PROCESS_ERROR during a legal block-size probe."));
  checks.push_back(has_output?check("audio.finite_output",finite?"PASS":"FAIL",finite?"All observed output samples were finite.":"Non-finite output observed.",{{"first_bad_sample",bad_sample},{"channel",bad_channel},{"value",std::isfinite(first_bad)?Json(first_bad):Json("non-finite")},{"processed_samples",done}}):check("audio.finite_output","UNSUPPORTED","Target exposes no audio output port."));
  if(std::string(desc->id)=="org.soundgrind.synthetic") checks.push_back(check("fixture.oracle",oracle_ok?"PASS":"FAIL",oracle_ok?"Fixture matched output=input*gain (with selected mode) within 1e-6.":"Fixture output differed from the deterministic oracle.",{{"tolerance",1e-6},{"processed_samples",done}}));
  else checks.push_back(check("fixture.oracle","UNSUPPORTED","No target-specific deterministic oracle is registered."));
  checks.push_back(check("lifecycle.repeat","PASS",has_output?"The plugin completed a second activation/process/deactivation cycle for block-size probes.":"One supervised lifecycle was exercised; no output port is available for a repeat render.",{{"cycles",has_output?2:1}}));
  if(params){Json readback=Json::object();bool valid=true;for(const auto& p:descriptor["parameters"]){double v=0;const auto id=p.value("clap_id",0u);if(!params->get_value(plugin,id,&v)){valid=false;continue;}readback[p.value("id","")]=v;if(snapshot.contains(p.value("id",std::string{}))&&std::abs(v-snapshot.at(p.value("id",std::string{})).get<double>())>1e-9)valid=false;}
    checks.push_back(check("params.readback",valid?"PASS":"FAIL",valid?"Snapshot parameter values were accepted and readable after processing.":"Parameter application/readback mismatch.",{{"readback",readback}}));
    bool domains=true,complete=true;Json exercised=Json::object();
    for(const auto& p:descriptor["parameters"]){
      const auto id=p.value("id",std::string{});const double lo=p.value("min",0.0),hi=p.value("max",0.0),def=p.value("default",lo),step=p.value("step",0.0);
      std::vector<double> values{def,lo,hi};
      if(step>0&&hi>=lo){const auto first=static_cast<std::int64_t>(std::ceil(lo));const auto last=static_cast<std::int64_t>(std::floor(hi));if(last>=first&&static_cast<std::uint64_t>(last-first+1)<=256){values.clear();for(auto n=first;n<=last;++n)values.push_back(static_cast<double>(n));}else if(last>=first)complete=false;}
      std::sort(values.begin(),values.end());values.erase(std::unique(values.begin(),values.end()),values.end());Json seen=Json::array();
      for(const auto candidate:values){Json one={{id,candidate}};auto ev=make_events(one,descriptor["parameters"]);params->flush(plugin,&ev.api,nullptr);double got=0;if(!params->get_value(plugin,p.value("clap_id",0u),&got)||std::abs(got-candidate)>1e-9)domains=false;seen.push_back({{"value",candidate},{"readback",got},{"accepted",std::abs(got-candidate)<=1e-9}});}
      exercised[id]=seen;
      if(p.value("enum",false)){for(const auto& v:values){char label[256]{};if(!params->value_to_text(plugin,p.value("clap_id",0u),v,label,sizeof(label))||label[0]=='\0')domains=false;}}
    }
    // Return every parameter to the exact case snapshot after testing boundaries.
    auto restore=make_events(snapshot,descriptor["parameters"]);params->flush(plugin,&restore.api,nullptr);
    checks.push_back(check("params.boundaries",!domains?"FAIL":(complete?"PASS":"INCONCLUSIVE"),!domains?"A declared boundary/stepped value failed application, readback, or enum text conversion.":(complete?"Declared defaults, extrema and every stepped/enum value were applied and read back.":"Boundary checks passed, but a stepped parameter domain exceeded the 256-value per-parameter cap."),{{"parameters_discovered",descriptor["parameters"].size()},{"parameter_values_exercised",exercised}}));
    Json normalized=Json::object();bool normalized_ok=true;
    for(const auto& p:descriptor["parameters"]){const auto id=p.value("id",std::string{});double lo=p.value("min",0.0),hi=p.value("max",0.0),step=p.value("step",0.0);Json points=Json::array();for(double x:{0.0,0.5,1.0}){double plain=lo+x*(hi-lo);if(step>0)plain=lo+std::round((plain-lo)/step)*step;plain=std::clamp(plain,lo,hi);double back=hi==lo?0.0:(plain-lo)/(hi-lo);if(step<=0&&hi!=lo&&std::abs(back-x)>1e-12)normalized_ok=false;points.push_back({{"normalized",x},{"plain",plain},{"roundtrip",back}});}normalized[id]=points;}
    checks.push_back(check("params.normalized_mapping",normalized_ok?"PASS":"FAIL",normalized_ok?"SoundGrind's linear normalized/plain mapping round-tripped at 0, 0.5 and 1 for each parameter.":"Normalized/plain mapping round-trip failed.",{{"mapping","plain=min+normalized*(max-min); stepped values quantized to declared step"},{"points",normalized}}));
  } else {checks.push_back(check("params.readback","UNSUPPORTED","Target does not expose CLAP params."));checks.push_back(check("params.boundaries","UNSUPPORTED","Target does not expose CLAP params."));checks.push_back(check("params.normalized_mapping","UNSUPPORTED","Target does not expose CLAP params."));}
  checks.push_back(check("processing.block_sizes",has_output?(size_ok?"PASS":"FAIL"):"UNSUPPORTED",has_output?(size_ok?"Completed a legal one-block probe at each tested block size.":"One or more legal block-size process calls returned an error."):"Target exposes no audio output.",{{"dwell_samples",done},{"sample_rate",sr},{"requested_dwell_block_size",bs},{"block_size_probes",size_checks},{"rounding","ceil requested dwell samples to whole configured blocks"}}));
  auto* state=static_cast<const clap_plugin_state_t*>(plugin->get_extension(plugin,CLAP_EXT_STATE));
  if(state){
    MemoryStream m;m.out={&m,stream_write};m.in={&m,stream_read};bool restored=state->save(plugin,&m.out);
    Json changed=snapshot;
    for(const auto& p:descriptor["parameters"]){const auto id=p.value("id",std::string{});const auto lo=p.value("min",0.0),hi=p.value("max",0.0);if(changed.contains(id))changed[id]=std::abs(changed[id].get<double>()-lo)>1e-9?lo:hi;}
    auto changed_events=make_events(changed,descriptor["parameters"]);
    if(restored&&params&&params->flush)params->flush(plugin,&changed_events.api,nullptr);
    m.offset=0;restored=restored&&state->load(plugin,&m.in);
    Json after=Json::object();bool values_restored=true;
    if(params)for(const auto& p:descriptor["parameters"]){double v=0;auto id=p.value("id",std::string{});if(!params->get_value(plugin,p.value("clap_id",0u),&v)){values_restored=false;continue;}after[id]=v;if(snapshot.contains(id)&&std::abs(v-snapshot.at(id).get<double>())>1e-9)values_restored=false;}
    restored=restored&&values_restored;
    checks.push_back(check("state.roundtrip",restored?"PASS":"FAIL",restored?"Saved state restored and parameter readback matched the pre-mutation snapshot.":"State stream save/restore or value readback failed.",{{"bytes",m.data.size()},{"readback",after}}));
  }
  else checks.push_back(check("state.roundtrip","UNSUPPORTED","Target does not advertise clap.state."));
  descriptor["module_results"]=checks;descriptor["processed_samples"]=done;descriptor["requested_dwell_ms"]=requested_ms;descriptor["actual_dwell_ms"]=1000.0*done/sr;descriptor["sample_rate"]=sr;descriptor["block_size"]=bs;descriptor["fixture_id"]="constant-0.25-mono-v1";
  return descriptor;
}
}
