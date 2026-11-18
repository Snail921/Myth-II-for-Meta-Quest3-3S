#pragma once
#include "sprite_api_profile.h"
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
 bool initialized=false,supported=false;int active=-1;unsigned serial[16]{},samples[16]{},invalid=0,skipped=0;double sum[16]{},peak[16]{};
 void init(){if(initialized)return;initialized=true;GLint n=0;glGetIntegerv(GL_NUM_EXTENSIONS,&n);for(int i=0;i<n;i++)if(!strcmp((const char*)glGetStringi(GL_EXTENSIONS,i),"GL_EXT_disjoint_timer_query"))supported=true;
 #define LOAD(member,type,name) member=(type)eglGetProcAddress(name)
 LOAD(gen,PFNGLGENQUERIESEXTPROC,"glGenQueriesEXT");LOAD(begin,PFNGLBEGINQUERYEXTPROC,"glBeginQueryEXT");LOAD(end,PFNGLENDQUERYEXTPROC,"glEndQueryEXT");LOAD(available,PFNGLGETQUERYOBJECTUIVEXTPROC,"glGetQueryObjectuivEXT");LOAD(result,PFNGLGETQUERYOBJECTUI64VEXTPROC,"glGetQueryObjectui64vEXT");
 #undef LOAD
 supported=supported&&gen&&begin&&end&&available&&result;
 if(supported){auto get=(PFNGLGETQUERYIVEXTPROC)eglGetProcAddress("glGetQueryivEXT");GLint bits=0;if(get)get(GL_TIME_ELAPSED_EXT,GL_QUERY_COUNTER_BITS_EXT,&bits);supported=bits>0;}
 if(supported)for(auto&s:slots)gen(1,&s.id);LOG("GPU timer supported=%d sample_every=17 eye_passes nonblocking=1",supported);}
 void poll(){if(!supported)return;GLint disjoint=0;glGetIntegerv(GL_GPU_DISJOINT_EXT,&disjoint);if(disjoint){invalid++;for(auto&s:slots)s.pending=false;for(int i=0;i<16;i++){sum[i]=peak[i]=0;samples[i]=0;}return;}
 for(auto&s:slots)if(s.pending){GLuint ready=0;available(s.id,GL_QUERY_RESULT_AVAILABLE_EXT,&ready);if(!ready)continue;GLuint64 ns=0;result(s.id,GL_QUERY_RESULT_EXT,&ns);double ms=ns/1e6;sum[s.kind]+=ms;peak[s.kind]=std::max(peak[s.kind],ms);samples[s.kind]++;s.pending=false;}}
 void start(int kind){if(!diagnosticEnabled)return;init();if(!supported)return;if(++serial[kind]%17)return;poll();for(int i=0;i<8;i++)if(!slots[i].pending){active=i;slots[i].kind=kind;begin(GL_TIME_ELAPSED_EXT,slots[i].id);return;}skipped++;}
 void stop(){if(active<0)return;end(GL_TIME_ELAPSED_EXT);slots[active].pending=true;active=-1;}
 void report(){poll();for(int k:{4,12})if(samples[k])LOG("Sprite GPU kind=%s ms_sampled_pass=%.3f peak_ms=%.3f samples=%u",k==4?"normal":"reflection",sum[k]/samples[k],peak[k],samples[k]);for(int i=0;i<16;i++){if(samples[i])LOG("Stage GPU id=%d ms_per_call=%.3f peak_ms=%.3f samples=%u",i,sum[i]/samples[i],peak[i],samples[i]);sum[i]=peak[i]=0;samples[i]=0;}LOG("Stage GPU disjoint=%u skipped=%u",invalid,skipped);invalid=skipped=0;}
 void release(){stop();if(supported){auto del=(PFNGLDELETEQUERIESEXTPROC)eglGetProcAddress("glDeleteQueriesEXT");if(del)for(auto&s:slots)del(1,&s.id);}*this=GPUProfile{};}

};
static GPUProfile gpuProfile;

struct StageProfile {
 double waits[5]{},cpu[16]{},started=0;unsigned calls[16]{};int active=-1;
 static const char* name(int i){static const char*n[]={"sky_setup","terrain","water","models","sprites","effects","controls","shadow_map"};return n[i%8];}
 void begin(int i){if(!diagnosticEnabled)return;end();active=i;if(spriteApi.enabled)ATrace_beginSection(i>=8?"MythVR/reflection-stage":"MythVR/normal-stage");started=seconds();gpuProfile.start(i);}
 void end(){if(active<0)return;gpuProfile.stop();if(spriteApi.enabled)ATrace_endSection();cpu[active]+=seconds()-started;calls[active]++;active=-1;}
 void report(unsigned frames){LOG("Frame wait CPU ms_frame xr_wait=%.3f swap_acquire_wait=%.3f resolve=%.3f flush_release=%.3f xr_end=%.3f",waits[0]*1000/frames,waits[1]*1000/frames,waits[2]*1000/frames,waits[3]*1000/frames,waits[4]*1000/frames);for(auto&w:waits)w=0;long pages=0,resident=0;if(FILE*f=fopen("/proc/self/statm","r")){if(fscanf(f,"%ld %ld",&pages,&resident)==2)LOG("Process memory rss_kib=%ld virtual_kib=%ld",resident*(sysconf(_SC_PAGESIZE)/1024),pages*(sysconf(_SC_PAGESIZE)/1024));fclose(f);}for(int i=0;i<16;i++){if(calls[i])LOG("Stage CPU id=%d name=%s reflection=%d ms_frame=%.3f calls=%u",i,name(i),i>=8,cpu[i]*1000/std::max(1u,frames),calls[i]);cpu[i]=0;calls[i]=0;}}
};
static StageProfile stageProfile;

// Fine CPU timings sample whole sprite passes, not every frame; no GPU readback.
struct SpriteDetailProfile {
 unsigned serial[2]{},passes[2]{},ops[2][10]{};double ms[2][10]{};
 bool sample(int kind){bool on=diagnosticEnabled&&++serial[kind]%17==0;if(on)passes[kind]++;return on;}
 void report(){static const char*n[]={"setup","hd_lookup_uniform","normal_bind","texture_bind","opaque_draw","blend_query_setup","soft_draw","cleanup","prepare_batch","upload"};for(int k=0;k<2;k++){if(passes[k])for(int i=0;i<10;i++)if(ops[k][i])LOG("Sprite detail kind=%s step=%s ms_sampled_pass=%.4f calls=%u passes=%u",k?"reflection":"normal",n[i],ms[k][i]/passes[k],ops[k][i],passes[k]);passes[k]=0;for(int i=0;i<10;i++){ms[k][i]=0;ops[k][i]=0;}}}
};
static SpriteDetailProfile spriteDetail;
struct SpriteDetailScope {
 bool enabled;int kind,stage;double start=0;
 SpriteDetailScope(bool on,int k,int s):enabled(on),kind(k),stage(s){if(on)start=seconds();}
 void next(int s){if(enabled){double now=seconds();spriteDetail.ms[kind][stage]+=(now-start)*1000;spriteDetail.ops[kind][stage]++;start=now;stage=s;}}
 ~SpriteDetailScope(){next(stage);}
};
