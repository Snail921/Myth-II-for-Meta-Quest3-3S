#pragma once
#include "interaction_math.h"
struct OrbitCamera {
 bool active=false;Vec3 pivot{},rebaseOffset{};float rebaseRemaining=0;
 void begin(const std::vector<Vec3>&selected,Vec3 eye,Vec3 forward,Vec3 horizontalFallback,const Terrain&t,float distance=15.f){
  pivot={};rebaseRemaining=0;rebaseOffset={};
  if(!selected.empty()){for(auto&p:selected)pivot=add(pivot,p);pivot=times(pivot,1.f/selected.size());}
  else{
   // Unselected orbit always uses a fixed horizontal distance, independent of ground intersection.
   forward.y=0;if(dot(forward,forward)<.0001f)forward=horizontalFallback;
   pivot=add(eye,times(normalized(forward),std::clamp(distance,0.f,100.f)));pivot.y=t.elevation(pivot.x,pivot.z);
  }
  active=true;
 }
 void follow(Vec3&rig,Vec3 center){Vec3 delta=sub(center,pivot);rig.x+=delta.x;rig.z+=delta.z;pivot=center;}
 // A death rebases the axis over one second. Surviving units keep moving.
 void followSurvivors(Vec3&rig,Vec3 center,bool removed,bool any,float dt){
  if(!any){rebaseRemaining=0;return;}
  if(removed){rebaseOffset=sub(pivot,center);rebaseRemaining=1.f;}
  rebaseRemaining=std::max(0.f,rebaseRemaining-std::max(0.f,dt));
  follow(rig,add(center,times(rebaseOffset,rebaseRemaining)));
 }
 // Preserve world altitude over descending terrain; only push up to avoid penetration.
 void constrainHeight(Vec3&rig,float ground,float headY,float&offset){
  rig.y=std::max(rig.y,ground+.2f-std::min(0.f,headY));offset=rig.y-ground;
 }
 bool step(Vec3&rig,float&yaw,Vec3 localHead,float angle,const Terrain&t){
  if(!active||!std::isfinite(angle))return false;
  Vec3 next=add(pivot,turn(sub(rig,pivot),angle));float nextYaw=yaw+angle;
  Vec3 eye=add(next,turn(localHead,nextYaw));float gx=mythGameX(eye.x,t.ox,t.scale),gy=eye.z/t.scale+t.oy;
  // Reject a boundary-crossing step instead of clamping and changing the radius.
  if(!std::isfinite(gx)||!std::isfinite(gy)||gx<.5f||gy<.5f||gx>t.width-1.5f||gy>t.height-1.5f)return false;
  next.y=rig.y;rig=next;yaw=nextYaw;return true;
 }
};
