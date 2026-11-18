#pragma once
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#ifdef __ANDROID__
#include <android/log.h>
#endif
struct SunAngleAdjustment {
 bool active=false,dirty=false,moving=false;
 float azimuth=0,elevation=30; double lastLog=0,noticeUntil=0;
 std::string path,level;
 static float axis(float v){return std::copysign(std::clamp((std::fabs(v)-.18f)/.82f,0.f,1.f),v);}
 void direction(float* v) const {constexpr float rad=.01745329252f;float a=azimuth*rad,e=elevation*rad;v[0]=std::sin(a)*std::cos(e);v[1]=std::sin(e);v[2]=std::cos(a)*std::cos(e);}
 void record(const char* reason,double now){
  float v[3];direction(v);char line[512];snprintf(line,sizeof(line),"Sun angle reason=%s level=%s active=%d azimuth=%.4f elevation=%.4f direction=%.7f,%.7f,%.7f mono=%.3f",reason,level.c_str(),active,azimuth,elevation,v[0],v[1],v[2],now);
#ifdef __ANDROID__
  __android_log_print(ANDROID_LOG_INFO,"MythVR","%s",line);
#endif
  if(!path.empty())if(FILE*f=fopen(path.c_str(),"a")){fprintf(f,"%s\n",line);fclose(f);}lastLog=now;
 }
 void init(const float*v,const char*root,const std::string&name){
  *this=SunAngleAdjustment{};level=name;if(root&&*root)path=std::string(root)+"/sun-angle-adjustment.log";
  float n=std::sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);if(n>0){azimuth=std::atan2(v[0],v[2])*57.29577951f;if(azimuth<0)azimuth+=360;elevation=std::asin(std::clamp(v[1]/n,-1.f,1.f))*57.29577951f;}
 }
 void toggle(double now){active=!active;moving=false;noticeUntil=now+5;record(active?"on":"off",now);}
 void step(float x,float y,float dt,double now){
  if(!active)return;float dx=axis(x),dy=axis(y);bool changed=dx!=0||dy!=0;
  if(changed){dt=std::clamp(dt,0.f,.05f);azimuth=std::fmod(azimuth+dx*15*dt+360,360.f);elevation=std::clamp(elevation+dy*10*dt,2.f,85.f);dirty=true;if(now-lastLog>=1)record("adjust",now);}
  else if(moving)record("stop",now);moving=changed;
 }
 std::string label()const{char s[100];snprintf(s,sizeof(s),"Sun %s  Az %.1f  El %.1f",active?"ADJUST":"OFF",azimuth,elevation);return s;}
};
static SunAngleAdjustment sunAngleAdjustment;
