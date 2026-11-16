#include "../src/vr/orbit_camera.h"
#include <cassert>
#include <cstdio>
static float radius(Vec3 a,Vec3 b){float x=a.x-b.x,z=a.z-b.z;return sqrtf(x*x+z*z);}
int main(){
 Terrain t;t.width=t.height=100;t.scale=1;t.heights.assign(10000,0);t.rebuildRayCeiling();
 Vec3 rig{-50,5,60},head{.3f,1.6f,.2f};float yaw=.2f;Vec3 eye=add(rig,turn(head,yaw));OrbitCamera orbit;
 orbit.begin({{-52,0,50},{-48,0,50}},eye,{0,-.5f,-1},{0,0,-1},t);assert(orbit.pivot.x==-50&&orbit.pivot.z==50);
 float r=radius(eye,orbit.pivot);Vec3 relative=turn(sub(orbit.pivot,eye),-yaw);
 for(int i=0;i<360;i++){assert(orbit.step(rig,yaw,head,.01745329252f,t));eye=add(rig,turn(head,yaw));assert(fabsf(radius(eye,orbit.pivot)-r)<.001f);Vec3 view=turn(sub(orbit.pivot,eye),-yaw);assert(fabsf(view.x-relative.x)<.001f&&fabsf(view.z-relative.z)<.001f);assert(rig.y==5);}
 orbit.follow(rig,{-45,4,52});eye=add(rig,turn(head,yaw));assert(fabsf(radius(eye,orbit.pivot)-r)<.001f);assert(rig.y==5);
 orbit.begin({}, {-50,5,50},normalized({0,-1,-1}),{0,0,-1},t);assert(fabsf(orbit.pivot.z-35)<.01f);
 orbit.begin({}, {-50,5,50},{0,1,0},{1,0,0},t);assert(fabsf(orbit.pivot.x+35)<.001f&&orbit.pivot.z==50);
 orbit.begin({{-1,0,1}},{-1,5,5},{0,0,-1},{0,0,-1},t);rig={-1,5,5};yaw=0;assert(!orbit.step(rig,yaw,{0,0,0},1.5707963f,t));assert(rig.x==-1&&rig.z==5&&yaw==0);
 orbit.pivot={-50,0,50};rig={-50,5,60};orbit.followSurvivors(rig,{-40,0,50},true,true,0);assert(orbit.pivot.x==-50);
 orbit.followSurvivors(rig,{-40,0,50},false,true,.5f);assert(orbit.pivot.x==-45&&rig.x==-45);
 orbit.followSurvivors(rig,{-39,0,50},false,true,.5f);assert(orbit.pivot.x==-39&&rig.x==-39);
 orbit.followSurvivors(rig,{},true,false,.5f);assert(orbit.pivot.x==-39&&rig.x==-39);
 float offset=5;rig.y=5;orbit.constrainHeight(rig,0,1.6f,offset);assert(rig.y==5&&offset==5);
 orbit.constrainHeight(rig,-4,1.6f,offset);assert(rig.y==5&&offset==9);
 orbit.constrainHeight(rig,6,1.6f,offset);assert(fabsf(rig.y-6.2f)<.0001f&&fabsf(offset-.2f)<.0001f);
 orbit.constrainHeight(rig,6,-.4f,offset);assert(rig.y-.4f>=6.1999f);
 orbit.begin({}, {-50,5,50},{0,0,-1},{0,0,-1},t,0);rig={-50,5,50};yaw=0;assert(orbit.step(rig,yaw,{0,0,0},.5f,t));assert(rig.x==-50&&rig.z==50);
 orbit.begin({}, {-50,5,50},{0,0,-1},{0,0,-1},t,100);assert(orbit.pivot.z==-50);
 puts("PASS configurable unselected orbit: default15, zero in-place yaw, 100m");
 puts("PASS orbit holds altitude downhill, clears rising terrain and re-bases exit offset");
 puts("PASS one-second death-axis lerp, moving survivors, final death freezes axis");
 puts("PASS orbit centroid, 360-degree radius/bearing, room-scale offset, moving pivot, fallback axes and map boundary");
}
