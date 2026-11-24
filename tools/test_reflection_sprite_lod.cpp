#include "../src/vr/reflection_sprite_lod.h"
#include <cassert>
#include <cstdio>
int main(){using namespace reflection_sprite_lod;
 enabled=true;distance=50;pixels=2;
 assert(keep(49,0,false));assert(!keep(50,1.99f,false));assert(keep(50,2,false));
 assert(!keep(80,2.49f,true));assert(keep(80,2.5f,true));assert(keep(0,0,true));
 assert(keep(80,INFINITY,true));enabled=false;assert(keep(80,0,true));enabled=true;pixels=0;assert(keep(80,0,true));pixels=2;
 rendercull::Rect water={-1,-1,1,1};assert(overlapPixels({0,0,.02f,.02f},water,100,100)<2);
 assert(overlapPixels({0,0,.001f,.5f},water,100,100)>2); // thin but long
 assert(overlapPixels({0,0,1,1},{.99f,.99f,1,1},100,100)<2);
 assert(overlapPixels({0,0,1,1},{-1,-1,-.5f,-.5f},100,100)==0);
 assert(std::isinf(overlapPixels({0,0,1,1},water,0,100)));
 assert(overlapPixels({0,0,.02f,.02f},water,400,400)>2); // high resolution
 float stereo=std::max(overlapPixels({0,0,.02f,.02f},water,100,100),overlapPixels({0,0,.08f,.08f},water,100,100));assert(keep(80,stereo,false));
 load("/data/local/tmp/lod-settings-missing");assert(enabled&&distance==50&&pixels==2);
 std::ofstream("/data/local/tmp/lod-settings-test/reflection-sprite-lod-distance")<<200;
 std::ofstream("/data/local/tmp/lod-settings-test/reflection-sprite-lod-half-pixels")<<99;
 load("/data/local/tmp/lod-settings-test");assert(distance==100&&pixels==32);
 std::ofstream("/data/local/tmp/lod-settings-test/reflection-sprite-lod-half-pixels")<<-1;
 load("/data/local/tmp/lod-settings-test");assert(pixels==0);
 puts("PASS reflection sprite LOD: distance, pixel threshold, hysteresis, stereo, thin shapes, water overlap, resolution, disabled/zero, settings defaults and clamps");
}
