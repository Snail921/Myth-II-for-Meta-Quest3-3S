#include "../src/vr/reflection_sprite_lod.h"
#include <cassert>
#include <cstdio>
#include <random>
#include <chrono>
int main(){using namespace reflection_sprite_lod;std::mt19937 rng(17);std::uniform_real_distribution<float> coord(-100,100),extent(.001f,4);
 std::array<std::array<float,16>,2> matrices;
 rendercull::Frustum full[2],cropped[2];rendercull::Rect water[2]={{-.6f,-.7f,.5f,.2f},{-.57f,-.7f,.53f,.2f}};
 for(int eye=0;eye<2;eye++){float a=.7f,c=cosf(a),s=sinf(a);matrices[eye]={c,0,1.001f*s,s,0,1,0,0,s,0,-1.001f*c,-c,eye?.03f:-.03f,0,-.1f,0};full[eye].set(matrices[eye],{-1,-1,1,1},-INFINITY);cropped[eye].set(matrices[eye],water[eye],-INFINITY);}
 std::vector<rendercull::Bounds> boxes;
 for(int i=0;i<4096;i++){float x=coord(rng),y=coord(rng)*.2f,z=coord(rng),dx=extent(rng),dy=extent(rng),dz=extent(rng);rendercull::Bounds b;b.add(x-dx,y-dy,z-dz);b.add(x+dx,y+dy,z+dz);boxes.push_back(b);}
 auto old=[&](const rendercull::Bounds&b,float threshold){float size=0;for(int eye=0;eye<2;eye++)size=std::max(size,overlapPixels(rendercull::projectedBounds(matrices[eye],b),water[eye],2064,2208));return size>=threshold;};
 auto fast=[&](const rendercull::Bounds&b,float threshold){for(int eye=0;eye<2;eye++)if(projectedAtLeast(matrices[eye],full[eye],b,water[eye],2064,2208,threshold))return true;return false;};
 unsigned compared=0,oldCandidates=0,newCandidates=0;
 for(auto&b:boxes){oldCandidates+=full[0].visible(b)||full[1].visible(b);newCandidates+=cropped[0].visible(b)||cropped[1].visible(b);for(float threshold:{.5f,1.f,2.f,2.5f,4.f,8.f,10.f}){assert(old(b,threshold)==fast(b,threshold));compared++;}}
 assert(newCandidates<oldCandidates);
 volatile unsigned oldResult=0,newResult=0;auto now=[](){return std::chrono::steady_clock::now();};
 auto start=now();for(int repeat=0;repeat<100;repeat++)for(auto&b:boxes)if(full[0].visible(b)||full[1].visible(b)){bool keep=old(b,2.f);if(keep&&(cropped[0].visible(b)||cropped[1].visible(b)))++oldResult;}
 double oldMs=std::chrono::duration<double,std::milli>(now()-start).count();
 start=now();for(int repeat=0;repeat<100;repeat++)for(auto&b:boxes)if(cropped[0].visible(b)||cropped[1].visible(b))if(fast(b,2.f))++newResult;
 double newMs=std::chrono::duration<double,std::milli>(now()-start).count();assert(oldResult==newResult);
 printf("PASS %u old/new decisions including near-camera crossings; candidates %u -> %u; retained result %u == %u; 100 synthetic frames %.3f -> %.3f ms (not film FPS)\n",compared,oldCandidates,newCandidates,unsigned(oldResult),unsigned(newResult),oldMs,newMs);
}
