#pragma once
#include <GLES2/gl2ext.h>
// Sample 1/17 reflection passes; scope switching includes early-continue paths.
struct SpriteProfile {
 double ms[4]{};unsigned passes=0,objects=0;uint64_t serial=0;
 bool sample(){return diagnosticEnabled&&(++serial%17)==0;}
 void report(){if(passes)LOG("Sprite profile samples=%u candidates=%u ms/sampled_eye scan_geometry=%.4f cull=%.4f palette=%.4f batch_submit=%.4f",passes,objects,ms[0]/passes,ms[1]/passes,ms[2]/passes,ms[3]/passes);for(auto&v:ms)v=0;passes=objects=0;}
};
static SpriteProfile spriteProfile;
struct SpriteScope {
 bool enabled;int stage=0;double start=0;
 explicit SpriteScope(bool on):enabled(on){if(on){start=seconds();spriteProfile.objects++;}}
 void next(int s){if(enabled){double now=seconds();spriteProfile.ms[stage]+=(now-start)*1000;start=now;stage=s;}}
 ~SpriteScope(){next(stage);}
};
struct GPUProfile {
 struct Slot{GLuint id=0;bool pending=false;int kind=0;};Slot slots[8];
 PFNGLGENQUERIESEXTPROC gen=nullptr;PFNGLBEGINQUERYEXTPROC begin=nullptr;PFNGLENDQUERYEXTPROC end=nullptr;
 PFNGLGETQUERYOBJECTUIVEXTPROC available=nullptr;PFNGLGETQUERYOBJECTUI64VEXTPROC result=nullptr;
 bool initialized=false,supported=false;int active=-1;unsigned serial[2]{},samples[2]{},invalid=0,skipped=0;double sum[2]{},peak[2]{};
 void init(){if(initialized)return;initialized=true;GLint n=0;glGetIntegerv(GL_NUM_EXTENSIONS,&n);for(int i=0;i<n;i++)if(!strcmp((const char*)glGetStringi(GL_EXTENSIONS,i),"GL_EXT_disjoint_timer_query"))supported=true;
 #define LOAD(member,type,name) member=(type)eglGetProcAddress(name)
 LOAD(gen,PFNGLGENQUERIESEXTPROC,"glGenQueriesEXT");LOAD(begin,PFNGLBEGINQUERYEXTPROC,"glBeginQueryEXT");LOAD(end,PFNGLENDQUERYEXTPROC,"glEndQueryEXT");LOAD(available,PFNGLGETQUERYOBJECTUIVEXTPROC,"glGetQueryObjectuivEXT");LOAD(result,PFNGLGETQUERYOBJECTUI64VEXTPROC,"glGetQueryObjectui64vEXT");
 #undef LOAD
 supported=supported&&gen&&begin&&end&&available&&result;
 if(supported){auto get=(PFNGLGETQUERYIVEXTPROC)eglGetProcAddress("glGetQueryivEXT");GLint bits=0;if(get)get(GL_TIME_ELAPSED_EXT,GL_QUERY_COUNTER_BITS_EXT,&bits);supported=bits>0;}
 if(supported)for(auto&s:slots)gen(1,&s.id);LOG("GPU timer supported=%d sample_every=17 eye_passes nonblocking=1",supported);}
 void poll(){if(!supported)return;GLint disjoint=0;glGetIntegerv(GL_GPU_DISJOINT_EXT,&disjoint);if(disjoint){invalid++;for(auto&s:slots)s.pending=false;sum[0]=sum[1]=peak[0]=peak[1]=0;samples[0]=samples[1]=0;return;}
 for(auto&s:slots)if(s.pending){GLuint ready=0;available(s.id,GL_QUERY_RESULT_AVAILABLE_EXT,&ready);if(!ready)continue;GLuint64 ns=0;result(s.id,GL_QUERY_RESULT_EXT,&ns);double ms=ns/1e6;sum[s.kind]+=ms;peak[s.kind]=std::max(peak[s.kind],ms);samples[s.kind]++;s.pending=false;}}
 void start(int kind){if(!diagnosticEnabled)return;init();if(!supported)return;if(++serial[kind]%17)return;poll();for(int i=0;i<8;i++)if(!slots[i].pending){active=i;slots[i].kind=kind;begin(GL_TIME_ELAPSED_EXT,slots[i].id);return;}skipped++;}
 void stop(){if(active<0)return;end(GL_TIME_ELAPSED_EXT);slots[active].pending=true;active=-1;}
 void report(){poll();LOG("GPU sampled_eye_ms normal=%.3f reflection=%.3f peak_normal=%.3f peak_reflection=%.3f samples=%u,%u disjoint=%u skipped=%u",samples[0]?sum[0]/samples[0]:0,samples[1]?sum[1]/samples[1]:0,peak[0],peak[1],samples[0],samples[1],invalid,skipped);sum[0]=sum[1]=peak[0]=peak[1]=0;samples[0]=samples[1]=invalid=skipped=0;}
};
static GPUProfile gpuProfile;
