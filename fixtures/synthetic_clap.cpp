#include <clap/clap.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <thread>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

namespace {
enum : clap_id { kGain=100, kMode=101, kFault=102 };
struct Instance { clap_plugin_t plugin{}; double gain=1.0; double mode=0.0; double fault=0.0; double sample_rate=48000; };
const clap_plugin_descriptor_t descriptor{CLAP_VERSION,"org.soundgrind.synthetic","SoundGrind Synthetic CLAP","SoundGrind","https://github.com/djshellshoxxx/soundgrind","","","0.1.0","Deterministic test target",nullptr};
Instance* self(const clap_plugin_t* p) { return static_cast<Instance*>(p->plugin_data); }
bool init(const clap_plugin_t*) { return true; }
void destroy(const clap_plugin_t* p) { delete self(p); }
bool activate(const clap_plugin_t* p,double sr,uint32_t,uint32_t) { self(p)->sample_rate=sr; return true; }
void deactivate(const clap_plugin_t*) {}
bool start(const clap_plugin_t*) { return true; }
void stop(const clap_plugin_t*) {}
void reset(const clap_plugin_t*) {}
void apply_events(Instance* s,const clap_process_t* p) {
  if(!p->in_events)return;
  for(uint32_t n=0;n<p->in_events->size(p->in_events);++n) {
    const auto* e=p->in_events->get(p->in_events,n);
    if(e&&e->space_id==CLAP_CORE_EVENT_SPACE_ID&&e->type==CLAP_EVENT_PARAM_VALUE) {
      const auto* v=reinterpret_cast<const clap_event_param_value_t*>(e);
      if(v->param_id==kGain)s->gain=std::clamp(v->value,0.0,2.0);
      if(v->param_id==kMode)s->mode=std::clamp(v->value,0.0,2.0);
      if(v->param_id==kFault)s->fault=std::clamp(v->value,0.0,4.0);
    }
  }
}
clap_process_status process(const clap_plugin_t* p,const clap_process_t* pr) {
  auto* s=self(p); apply_events(s,pr);
  if(s->fault==2.0) {
#ifdef _WIN32
    TerminateProcess(GetCurrentProcess(),0xEE);
#else
    std::abort();
#endif
  }
  if(s->fault==3.0) std::this_thread::sleep_for(std::chrono::seconds(30));
  for(uint32_t ch=0; ch<(pr->audio_outputs_count?pr->audio_outputs[0].channel_count:0); ++ch) {
    auto* out=pr->audio_outputs[0].data32[ch];
    const float* in=(pr->audio_inputs_count&&ch<pr->audio_inputs[0].channel_count)?pr->audio_inputs[0].data32[ch]:nullptr;
    for(uint32_t f=0;f<pr->frames_count;++f) {
      const float x=in?in[f]:0.0f;
      float y=x*static_cast<float>(s->gain);
      if(s->mode==1.0)y=-y; else if(s->mode==2.0)y=0.0f;
      if(s->fault==1.0)y=std::numeric_limits<float>::quiet_NaN();
      if(s->fault==4.0)y+=0.125f;
      out[f]=y;
    }
  }
  return CLAP_PROCESS_CONTINUE;
}
const void* get_extension(const clap_plugin_t* p,const char* id);
void on_main(const clap_plugin_t*) {}

uint32_t param_count(const clap_plugin_t*) { return 3; }
bool param_info(const clap_plugin_t*,uint32_t ix,clap_param_info_t* out) {
  if(ix>2||!out)return false; std::memset(out,0,sizeof(*out));
  if(ix==0){out->id=kGain;out->flags=CLAP_PARAM_IS_AUTOMATABLE;std::strncpy(out->name,"Gain",sizeof(out->name)-1);out->min_value=0;out->max_value=2;out->default_value=1;}
  if(ix==1){out->id=kMode;out->flags=CLAP_PARAM_IS_AUTOMATABLE|CLAP_PARAM_IS_STEPPED|CLAP_PARAM_IS_ENUM;std::strncpy(out->name,"Mode",sizeof(out->name)-1);out->min_value=0;out->max_value=2;out->default_value=0;}
  if(ix==2){out->id=kFault;out->flags=CLAP_PARAM_IS_HIDDEN|CLAP_PARAM_IS_STEPPED|CLAP_PARAM_IS_ENUM;std::strncpy(out->name,"Fixture Fault",sizeof(out->name)-1);out->min_value=0;out->max_value=4;out->default_value=0;}
  return true;
}
bool get_value(const clap_plugin_t* p,clap_id id,double* v) { if(id==kGain)*v=self(p)->gain;else if(id==kMode)*v=self(p)->mode;else if(id==kFault)*v=self(p)->fault;else return false;return true; }
bool value_text(const clap_plugin_t*,clap_id,double v,char* b,uint32_t n) { if(n<2)return false; std::snprintf(b,n,"%.3f",v); return true; }
bool text_value(const clap_plugin_t*,clap_id,const char* t,double* v) { if(!t||!v)return false;try{*v=std::stod(t);return true;}catch(...){return false;} }
void flush_params(const clap_plugin_t* p,const clap_input_events_t* in,const clap_output_events_t*) {
  if(!in)return; for(uint32_t n=0;n<in->size(in);++n){auto* e=in->get(in,n);if(e&&e->type==CLAP_EVENT_PARAM_VALUE){auto* v=reinterpret_cast<const clap_event_param_value_t*>(e);auto* s=self(p);if(v->param_id==kGain)s->gain=v->value;else if(v->param_id==kMode)s->mode=v->value;else if(v->param_id==kFault)s->fault=v->value;}}
}
const clap_plugin_params_t params{param_count,param_info,get_value,value_text,text_value,flush_params};
uint32_t audio_count(const clap_plugin_t*,bool) { return 1; }
bool audio_get(const clap_plugin_t*,uint32_t ix,bool input,clap_audio_port_info_t* info) {
  if(ix||!info)return false;std::memset(info,0,sizeof(*info));info->id=0;std::strncpy(info->name,input?"Input":"Output",sizeof(info->name)-1);info->flags=CLAP_AUDIO_PORT_IS_MAIN;info->channel_count=1;info->port_type=CLAP_PORT_MONO;info->in_place_pair=0;return true;
}
const clap_plugin_audio_ports_t audio_ports{audio_count,audio_get};
bool save_state(const clap_plugin_t* p,const clap_ostream_t* stream) {
  const double vals[]{self(p)->gain,self(p)->mode,self(p)->fault}; return stream->write(stream,vals,sizeof(vals))==sizeof(vals);
}
bool load_state(const clap_plugin_t* p,const clap_istream_t* stream) {
  double vals[3]{};if(stream->read(stream,vals,sizeof(vals))!=sizeof(vals))return false;auto* s=self(p);s->gain=vals[0];s->mode=vals[1];s->fault=vals[2];return true;
}
const clap_plugin_state_t state{save_state,load_state};
const void* get_extension(const clap_plugin_t*,const char* id) {
  if(!std::strcmp(id,CLAP_EXT_PARAMS))return &params;
  if(!std::strcmp(id,CLAP_EXT_AUDIO_PORTS))return &audio_ports;
  if(!std::strcmp(id,CLAP_EXT_STATE))return &state;
  return nullptr;
}
const clap_plugin_t* create_plugin(const clap_plugin_factory_t*,const clap_host_t* host,const char* id) {
  (void)host; if(std::strcmp(id,descriptor.id))return nullptr;auto* s=new Instance;
  s->plugin={&descriptor,s,init,destroy,activate,deactivate,start,stop,reset,process,get_extension,on_main};
  return &s->plugin;
}
uint32_t plugin_count(const clap_plugin_factory_t*) { return 1; }
const clap_plugin_descriptor_t* plugin_descriptor(const clap_plugin_factory_t*,uint32_t i) { return i==0?&descriptor:nullptr; }
const clap_plugin_factory_t factory{plugin_count,plugin_descriptor,create_plugin};
bool entry_init(const char*) { return true; }
void entry_deinit() {}
const void* entry_factory(const char* id) { return !std::strcmp(id,CLAP_PLUGIN_FACTORY_ID)?&factory:nullptr; }
}

extern "C" CLAP_EXPORT const clap_plugin_entry_t clap_entry{CLAP_VERSION,entry_init,entry_deinit,entry_factory};
