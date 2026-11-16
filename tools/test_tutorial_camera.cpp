#include "../src/vr/tutorial_camera.h"
#include "../src/vr/interaction_math.h"
#include <cassert>
#include <cstdio>
int main(){
 TutorialCameraGate g;unsigned stage=123u<<16|52;
 for(int i=0;i<30;i++)assert(!g.update(stage,4,.02f));
 assert(!g.update(stage,0,.02f));
 for(int i=0;i<5;i++)assert(!g.update(stage,4,.02f));
 assert(g.update(stage,4,.03f)==4);
 assert(!g.update(stage+1,4,.02f));
 assert(!tutorialStickBits(.3f,.3f,6,2));assert(!tutorialStickBits(.8f,.8f,6,2));
 assert(tutorialStickBits(.9f,.2f,6,2)==128);assert(tutorialStickBits(.2f,.9f,6,2)==4);
 assert(tutorialStickBits(-.9f,.2f,4,8)==16);assert(tutorialStickBits(.2f,-.9f,4,8)==512);
 // HMD yaw compensation: original yaw zero points toward original +X (VR -X).
 for(float localYaw:{-2.f,0.f,1.3f})for(float originalYaw:{0.f,1.f,3.f,5.f}){
  float rigYaw=originalYaw+1.5707963268f-localYaw;
  Vec3 forward=turn(turn({0,0,-1},localYaw),rigYaw);
  assert(fabsf(forward.x+cosf(originalYaw))<.0001f&&fabsf(forward.z-sinf(originalYaw))<.0001f);
 }
 puts("PASS tutorial neutral/stage/dwell/dominant-axis and script camera yaw compensation");
}
