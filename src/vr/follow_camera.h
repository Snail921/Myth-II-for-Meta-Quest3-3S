#pragma once
#include "orbit_camera.h"
struct FollowCamera {
 bool remembered=false;float rememberedDistance=15;
 float startingDistance(float configured,bool reuse)const{return reuse&&remembered?rememberedDistance:configured;}
 bool active=false;float distance=15;Vec3 outward{0,0,1};
 static bool lateralBreak(float x){return std::isfinite(x)&&fabsf(x)>=.5f;}
 static float axis(float value){return copysignf(std::clamp((fabsf(value)-.18f)/.82f,0.f,1.f),value);}
 void begin(Vec3 eye,Vec3 pivot,Vec3 forward,float requested,bool continuous,bool aimBeforeJump=false,Vec3 horizontalFallback={0,0,-1}){
  active=continuous;distance=std::clamp(requested,0.f,100.f);outward=aimBeforeJump?sub(eye,pivot):times(forward,-1);outward.y=0;
  if(dot(outward,outward)<.0001f){outward=times(forward,-1);outward.y=0;if(dot(outward,outward)<.0001f){outward=times(horizontalFallback,-1);outward.y=0;}if(dot(outward,outward)<.0001f)outward={0,0,1};}
  outward=normalized(outward);
 }
 void adjust(float y,float speed,float dt,bool remember=false){float before=distance;distance=std::clamp(distance-axis(y)*speed*std::max(0.f,dt),0.f,100.f);if(remember&&distance!=before){rememberedDistance=distance;remembered=true;}}
 void rotate(float angle){outward=turn(outward,angle);}
 void place(Vec3&rig,float&yaw,Vec3 head,Vec3 localForward,Vec3 pivot,const Terrain&t,bool aim){
  Vec3 eye=add(pivot,times(outward,distance));
  eye.x=std::clamp(eye.x,mythWorldX(t.width-1.5f,t.ox,t.scale),mythWorldX(.5f,t.ox,t.scale));
  eye.z=std::clamp(eye.z,(-t.oy+.5f)*t.scale,(t.height-1.5f-t.oy)*t.scale);
  Vec3 toward=sub(pivot,eye);toward.y=0;localForward.y=0;
  if(aim&&distance>0&&dot(toward,toward)>.0001f&&dot(localForward,localForward)>.0001f)
   yaw=atan2f(toward.x,toward.z)-atan2f(localForward.x,localForward.z);
  Vec3 offset=turn(head,yaw);rig.x=eye.x-offset.x;rig.z=eye.z-offset.z;
 }
};
