#pragma once
// All-pass wall/thread-CPU accounting. Nested rows are explicitly not additive.
struct SpriteWaitProfile {
 struct Stat{double wall=0,cpu=0,peak=0,peakCpu=0;unsigned calls=0,slow=0;};Stat stats[2][12]{};int kind=0;
 static double threadTime(){timespec t{};clock_gettime(CLOCK_THREAD_CPUTIME_ID,&t);return t.tv_sec+t.tv_nsec/1e9;}
 void add(int k,int step,double w,double c){auto&s=stats[k][step];s.wall+=w;s.cpu+=c;s.calls++;if(w>=.005)s.slow++;if(w>s.peak){s.peak=w;s.peakCpu=c;}}
 void report(unsigned frames){const char*n[]={"whole_sprite_pass","bind_all","lighting","shadow_lod_uniforms","shadow_transform_uniforms","static_depth_bind","live_depth_bind","owner_bind","prepare","visibility_batch","upload","stream_draw"};for(int k=0;k<2;k++)for(int i=0;i<12;i++){auto&s=stats[k][i];if(s.calls)LOG("Sprite wait kind=%s step=%s wall_ms_frame=%.4f thread_ms_frame=%.4f max_ms=%.4f max_thread_ms=%.4f calls=%u over5ms=%u",k?"reflection":"normal",n[i],s.wall*1000/std::max(1u,frames),s.cpu*1000/std::max(1u,frames),s.peak*1000,s.peakCpu*1000,s.calls,s.slow);s={};}}
};
static SpriteWaitProfile spriteWait;
struct SpriteWaitScope {
 bool on;int kind,step;double wall=0,cpu=0;
 SpriteWaitScope(int k,int s):on(diagnosticEnabled),kind(k),step(s){if(on){wall=seconds();cpu=SpriteWaitProfile::threadTime();}}
 void next(int s){if(on){double c=SpriteWaitProfile::threadTime(),w=seconds();spriteWait.add(kind,step,w-wall,c-cpu);wall=w;cpu=c;}step=s;}
 ~SpriteWaitScope(){next(step);}
};
