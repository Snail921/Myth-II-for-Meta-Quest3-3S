#include "../src/vr/interaction_math.h"
#include <cstdio>
#include <cstdlib>
static void check(bool ok,const char*name){if(!ok){fprintf(stderr,"FAIL %s\n",name);exit(1);}}
static bool near(float a,float b){return fabsf(a-b)<.002f;}
int main(){
 Terrain t;t.width=3;t.height=3;t.heights={0,0,0,0,4,0,0,0,0};
 check(near(t.elevation(-.75f,.25f),1),"even triangle one");check(near(t.elevation(-.25f,.75f),1),"even triangle two");
 check(near(t.elevation(-1.25f,.25f),1),"odd triangle one");check(near(t.elevation(-1.75f,.75f),1),"odd triangle two");
 Vec3 hit{};float d=0;check(t.ray({-.5f,10,.5f},{0,-1,0},hit,d)&&near(hit.y,2)&&near(d,8),"terrain ray exact surface");
 check(!t.ray({-.5f,10,.5f},{0,1,0},hit,d),"sky no ground");check(!t.ray({-1,10,-1},{0,-1,0},hit,d),"outside map no ground");
 check(!t.contains(-2,1)&&t.contains(-1,1),"map outer cells");check(near(raySphere({0,0,5},{0,0,-1},{0,0,0},.5f),4.5f),"unit front intersection");
 check(raySphere({2,0,5},{0,0,-1},{0,0,0},.5f)<0,"miss unit");
 Vec3 v=turn({0,0,-1},3.1415926535f/2);check(near(v.x,-1)&&near(v.z,0),"yaw transform");
 // Exact inverse for raw game movement coordinates after handedness correction.
 for(int raw=0;raw<224*512;raw+=37){float x=raw/512.f;check(lroundf(mythGameX(mythWorldX(x,187.2f,1),187.2f,1)*512)==raw,"movement inverse");}
 check(mythWorldX(190,187.2f,1)<mythWorldX(185,187.2f,1),"original +x is VR left");
 // Asymmetric heightfield catches accidental texture/height or inverse reflection mismatch.
 t.heights={0,2,4,0,2,4,0,2,4};check(near(t.elevation(-.25f,.5f),.5f)&&near(t.elevation(-1.25f,.5f),2.5f),"asymmetric heights");
 puts("PASS 11 terrain/ray/camera math checks");
}
