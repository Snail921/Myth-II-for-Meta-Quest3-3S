#pragma once
#include "interaction_math.h"
#include <array>
// Local tracking-space metres: locomotion and camera rotation never contribute.
struct ControllerTap {
 Vec3 left{},right{};double last=0,until=0,candidateUntil=0;bool sampled=false,armed=false;int candidate=0;
 void reset(){sampled=armed=false;candidate=0;}
 int update(Vec3 l,Vec3 r,double now,bool valid,float distanceScale=1){
  distanceScale=std::clamp(distanceScale,.1f,1.f);
  if(!valid){reset();return 0;}
  Vec3 delta=sub(r,l);float distance=sqrtf(dot(delta,delta));
  if(!sampled||now-last>.1||now<=last){left=l;right=r;last=now;sampled=true;armed=distance>.24f*distanceScale;candidate=0;return 0;}
  float dt=float(now-last);Vec3 vl=times(sub(l,left),1/dt),vr=times(sub(r,right),1/dt);left=l;right=r;last=now;
  if(dot(vl,vl)>9||dot(vr,vr)>9){reset();return 0;}
  if(distance>.24f*distanceScale){armed=true;candidate=0;}if(now<until||!armed)return 0;
  Vec3 axis=normalized(delta);float lc=dot(vl,axis),rc=-dot(vr,axis),closing=lc+rc;
  if(candidate){if(now>candidateUntil||distance>.20f*distanceScale){candidate=0;return 0;}if(closing<.12f){int result=candidate;candidate=0;armed=false;until=now+.20;return result;}return 0;}
  if(distance<.18f*distanceScale&&closing>.25f){
   if(rc>.25f&&rc>std::max(0.f,lc)*1.6f+.10f)candidate=-1;
   else if(lc>.25f&&lc>std::max(0.f,rc)*1.6f+.10f)candidate=1;
   candidateUntil=now+.18;
  }return 0;
 }
};
inline int cycleUnitGroup(const std::array<unsigned,10>&groups,int current,int direction){for(int n=1;n<=10;n++){int i=(current<0?(direction>0?n-1:10-n):(current+direction*n+20)%10);if(groups[i])return i;}return -1;}
