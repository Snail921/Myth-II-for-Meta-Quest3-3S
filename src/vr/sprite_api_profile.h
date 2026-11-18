#pragma once
#include <android/trace.h>
#include <time.h>
#include <unistd.h>
// Opt-in, sampled CPU timing; never synchronizes with the GPU.
struct SpriteApiProfile {
 bool enabled=false;unsigned serial[3]{},calls=0,slow=0,stored=0;
 struct Event{const char*name;int kind;unsigned texture,count;uint64_t boot,mono;double wall,cpu;};Event events[8]{};
 static uint64_t clockNs(clockid_t id){timespec t{};clock_gettime(id,&t);return uint64_t(t.tv_sec)*1000000000ull+t.tv_nsec;}
 void load(const char*root){*this=SpriteApiProfile{};char path[512];snprintf(path,sizeof(path),"%s/diagnostic-api-waits.enabled",root);if(FILE*f=fopen(path,"rb")){enabled=fgetc(f)=='1';fclose(f);}enabled=enabled&&diagnosticEnabled;LOG("API wait diagnostic enabled=%d sample_every=17 slow_ms=0.5 max_events_per_report=8 pid=%d tid=%d boot_ns=%llu mono_ns=%llu",enabled,getpid(),gettid(),(unsigned long long)clockNs(CLOCK_BOOTTIME),(unsigned long long)clockNs(CLOCK_MONOTONIC));}
 bool sample(int kind){return enabled&&++serial[kind]%17==0;}
 template<class F> void measure(bool on,int kind,const char*name,unsigned texture,unsigned count,F fn){
  if(!on){fn();return;}ATrace_beginSection(name);uint64_t boot=clockNs(CLOCK_BOOTTIME),mono=clockNs(CLOCK_MONOTONIC),cpu=clockNs(CLOCK_THREAD_CPUTIME_ID);fn();uint64_t endCpu=clockNs(CLOCK_THREAD_CPUTIME_ID),end=clockNs(CLOCK_MONOTONIC);ATrace_endSection();calls++;double w=(end-mono)/1e6,c=(endCpu-cpu)/1e6;if(w<.5)return;slow++;Event e{name,kind,texture,count,boot,mono,w,c};
  if(stored<8)events[stored++]=e;else{unsigned smallest=0;for(unsigned i=1;i<8;i++)if(events[i].wall<events[smallest].wall)smallest=i;if(w>events[smallest].wall)events[smallest]=e;}
 }
 void report(){if(!enabled)return;for(unsigned i=0;i<stored;i++){auto&e=events[i];LOG("API wait name=%s kind=%d tid=%d boot_ns=%llu mono_ns=%llu wall_ms=%.4f thread_ms=%.4f texture=%u count=%u",e.name,e.kind,gettid(),(unsigned long long)e.boot,(unsigned long long)e.mono,e.wall,e.cpu,e.texture,e.count);}LOG("API wait summary sampled_calls=%u slow=%u retained=%u omitted=%u",calls,slow,stored,slow-stored);calls=slow=stored=0;}
};
static SpriteApiProfile spriteApi;
