#include "../src/vr/controller_tap.h"
#include <cassert>
#include <cstdio>
int main(){
 for(int direction:{-1,1}){ControllerTap tap;double t=1;auto update=[&](float distance,bool valid=true){t+=.02;return tap.update(direction==-1?Vec3{0,0,0}:Vec3{.32f-distance,0,0},direction==-1?Vec3{distance,0,0}:Vec3{.32f,0,0},t,valid);};assert(!update(.32));for(float d:{.30f,.28f,.26f,.24f,.22f,.20f,.18f,.16f})assert(!update(d));assert(update(.16)==direction);for(int i=0;i<40;i++)assert(!update(.16));assert(!update(.32));for(float d:{.30f,.28f,.26f,.24f,.22f,.20f,.18f,.16f})assert(!update(d));assert(update(.16)==direction);update(.32,false);assert(!update(.16));assert(!update(.16));}
 for(float scale:{.1f,.5f,1.f})for(int direction:{-1,1}){ControllerTap tap;double t=1;auto step=[&](float d){t+=.02*scale;return tap.update(direction==-1?Vec3{}:Vec3{(.32f-d)*scale,0,0},direction==-1?Vec3{d*scale,0,0}:Vec3{.32f*scale,0,0},t,true,scale);};assert(!step(.32));for(float d:{.30f,.28f,.26f,.24f,.22f,.20f,.18f,.16f})assert(!step(d));assert(step(.16)==direction);double fired=t;assert(fabs(tap.until-fired-.2)<1e-6);tap.update({0,0,0},{.32f*scale,0,0},fired+.08,true,scale);assert(tap.armed);assert(!tap.update({0,0,0},{.16f*scale,0,0},fired+.09,true,scale));assert(!tap.update({0,0,0},{.16f*scale,0,0},fired+.10,true,scale));}
 ControllerTap both;double t=1;for(int i=0;i<30;i++){float d=.32f-i*.006f;assert(!both.update({-d/2,0,0},{d/2,0,0},t+=.02,true));}assert(!both.update({-.073f,0,0},{.073f,0,0},t+=.02,true));
 ControllerTap slow;for(int i=0;i<100;i++)assert(!slow.update({0,0,0},{.32f-i*.002f,0,0},2+i*.02,true));
 std::array<unsigned,10> groups{};assert(cycleUnitGroup(groups,-1,1)==-1);groups[1]=3;groups[7]=2;assert(cycleUnitGroup(groups,-1,1)==1);assert(cycleUnitGroup(groups,-1,-1)==7);assert(cycleUnitGroup(groups,7,1)==1);assert(cycleUnitGroup(groups,1,-1)==7);groups[7]=0;assert(cycleUnitGroup(groups,1,1)==1);
 puts("PASS directional tap, separation/rearm, cooldown, tracking loss, symmetric motion, slow approach, sparse group wrap");
}

