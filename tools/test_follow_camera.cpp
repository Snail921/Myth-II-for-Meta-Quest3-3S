#include "../src/vr/follow_camera.h"
#include <cassert>
#include <cstdio>
static float radius(Vec3 a,Vec3 b){auto d=sub(a,b);return sqrtf(d.x*d.x+d.z*d.z);}
int main(){
 Terrain t;t.width=t.height=200;t.scale=1;t.heights.assign(40000,0);t.rebuildRayCeiling();
 Vec3 rig{-100,8,140},head{.4f,1.6f,.3f},pivot{-100,0,100},forward{.5f,0,-.8660254f};float yaw=.4f;
 FollowCamera f;auto eye=add(rig,turn(head,yaw));f.begin(eye,pivot,turn(forward,yaw),30,true);f.place(rig,yaw,head,forward,pivot,t,true);eye=add(rig,turn(head,yaw));assert(fabsf(radius(eye,pivot)-30)<.001f&&rig.y==8);
 auto toward=normalized(sub(pivot,eye));toward.y=0;toward=normalized(toward);auto facing=normalized(turn(forward,yaw));assert(dot(toward,facing)>.9999f);
 float initialYaw=yaw;for(int i=0;i<60;i++){pivot.x+=.1f;f.place(rig,yaw,head,{1,0,0},pivot,t,false);eye=add(rig,turn(head,yaw));assert(fabsf(radius(eye,pivot)-30)<.001f&&yaw==initialYaw);}
 assert(!FollowCamera::lateralBreak(.499f)&&!FollowCamera::lateralBreak(-.499f));assert(FollowCamera::lateralBreak(.5f)&&FollowCamera::lateralBreak(-.5f));
 f.adjust(1,3,1);assert(f.distance==27);f.adjust(-1,3,1);assert(f.distance==30);f.adjust(.17f,3,1);assert(f.distance==30);
 f.rotate(.5f);yaw+=.5f;f.place(rig,yaw,head,forward,pivot,t,false);eye=add(rig,turn(head,yaw));assert(fabsf(radius(eye,pivot)-30)<.001f);
 // Grip-pan changes yaw without changing follow axis side or eye position.
 Vec3 before=eye;float previousYaw=yaw;yaw+=.7f;f.place(rig,yaw,head,forward,pivot,t,false);eye=add(rig,turn(head,yaw));assert(radius(eye,before)<.001f&&fabsf(yaw-previousYaw-.7f)<.001f&&f.active);
 pivot.x+=2;f.place(rig,yaw,head,forward,pivot,t,false);eye=add(rig,turn(head,yaw));assert(fabsf(radius(eye,pivot)-30)<.001f);
 f.begin(eye,pivot,forward,0,true);initialYaw=yaw;f.place(rig,yaw,head,forward,pivot,t,true);eye=add(rig,turn(head,yaw));assert(radius(eye,pivot)<.001f&&yaw==initialYaw);
 f.begin(eye,pivot,forward,100,false);assert(!f.active&&f.distance==100);pivot={-.5f,0,.5f};f.place(rig,yaw,head,forward,pivot,t,true);eye=add(rig,turn(head,yaw));assert(eye.x<=-.5f&&eye.x>=-198.5f&&eye.z>=.5f&&eye.z<=198.5f);
 f.adjust(-1,3,100);assert(f.distance==100);f.adjust(1,3,100);assert(f.distance==0);
 // Both one-shot jump and follow entry preserve yaw when aiming is disabled.
 for(int continuous=0;continuous<2;continuous++){
  Vec3 jumpRig{-100,8,140},jumpPivot{-100,0,100};float jumpYaw=.7f;
  FollowCamera jump;auto jumpEye=add(jumpRig,turn(head,jumpYaw));jump.begin(jumpEye,jumpPivot,turn(forward,jumpYaw),15,continuous!=0);
  jump.place(jumpRig,jumpYaw,head,forward,jumpPivot,t,false);
  assert(jumpYaw==.7f&&jumpRig.y==8&&fabsf(radius(add(jumpRig,turn(head,jumpYaw)),jumpPivot)-15)<.001f);
 }
 // Target is horizontally centered for different gaze angles and initial sides.
 for(int k=0;k<8;k++)for(int continuous=0;continuous<2;continuous++){
  Vec3 p{-100,0,100},h{.4f,1.6f,-.3f},rig0{-60,10,160};float y=.31f;
  float a=k*6.283185307f/8;Vec3 local{sinf(a),-.8f,cosf(a)},world=turn(local,y);
  FollowCamera j;j.begin(add(rig0,turn(h,y)),p,world,15,continuous!=0);
  j.place(rig0,y,h,local,p,t,false);Vec3 delta=sub(p,add(rig0,turn(h,y)));delta.y=0;world.y=0;
  assert(dot(normalized(delta),normalized(world))>.99999f&&fabsf(radius(add(rig0,turn(h,y)),p)-15)<.001f&&y==.31f&&rig0.y==10);
 }
 FollowCamera vertical;vertical.begin({-90,5,120},{-100,0,100},{0,1,0},15,false,false,{1,0,0});assert(vertical.outward.x==-1&&vertical.outward.z==0);
 FollowCamera legacy;legacy.begin({-100,5,140},{-100,0,100},{1,0,0},15,false,true);assert(legacy.outward.x==0&&legacy.outward.z==1);
 FollowCamera memory;assert(memory.startingDistance(15,true)==15);
 memory.begin({0,0,20},{0,0,0},{0,0,-1},15,true);
 memory.adjust(.1f,3,1,true);assert(!memory.remembered);
 memory.adjust(1,3,1,true);assert(memory.startingDistance(15,true)==12);assert(memory.startingDistance(15,false)==15);
 memory.begin({0,0,20},{0,0,0},{0,0,-1},memory.startingDistance(15,true),false);assert(memory.distance==12&&!memory.active);
 memory.begin({0,0,20},{0,0,0},{0,0,-1},memory.startingDistance(15,true),true);assert(memory.distance==12&&memory.active);
 memory.adjust(-1,3,1,false);assert(memory.distance==15&&memory.startingDistance(15,true)==12);
 puts("PASS follow/warp aim, 30m radius, head freedom, moving axis, orbit, 0m, bounds and +/-0.5 lateral threshold");
}
