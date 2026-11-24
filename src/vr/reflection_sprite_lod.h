#pragma once
#include "render_culling.h"
#include <fstream>
#include <cstdint>
#include <map>
#include <tuple>
namespace reflection_sprite_lod {
static bool enabled=true;static float distance=50,pixels=2;
struct History {bool hidden=false;uint64_t seen=0;};
using Key=std::tuple<unsigned,unsigned,unsigned,unsigned>;
static std::map<Key,History> history;
static unsigned projectedEyes=0,projectedCorners=0,earlyKept=0;
static unsigned tested=0,nearKept=0,rejected=0,emptyPasses=0;static double cpu=0;
inline float overlapPixels(rendercull::Rect object,rendercull::Rect water,int w,int h){
 if(w<=0||h<=0)return INFINITY;
 if(!object.valid()||!water.valid())return 0;
 float width=std::min(object.r,water.r)-std::max(object.l,water.l),height=std::min(object.t,water.t)-std::max(object.b,water.b);
 if(width<=0||height<=0)return 0;
 return std::max(width*w*.5f,height*h*.5f);
}
// Same bounds/near-plane semantics as projectedBounds, but stop once the
// expanding rectangle proves retention. A later corner can only enlarge it.
inline bool projectedAtLeast(const std::array<float,16>&m,const rendercull::Frustum&full,const rendercull::Bounds&bounds,rendercull::Rect water,int w,int h,float threshold){
 if(w<=0||h<=0)return true;
 if(!water.valid()||!full.visible(bounds))return false;
 ++projectedEyes;rendercull::Rect result;
 for(unsigned i=0;i<8;i++){
  ++projectedCorners;float v[]={i&1?bounds.hi[0]:bounds.lo[0],i&2?bounds.hi[1]:bounds.lo[1],i&4?bounds.hi[2]:bounds.lo[2],1},p[4]{};
  for(int r=0;r<4;r++)for(int c=0;c<4;c++)p[r]+=m[c*4+r]*v[c];
  if(p[3]<=1e-6f||p[2]+p[3]<=0)return overlapPixels({-1,-1,1,1},water,w,h)>=threshold;
  result.add(std::clamp(p[0]/p[3],-1.f,1.f),std::clamp(p[1]/p[3],-1.f,1.f));
  if(overlapPixels(result,water,w,h)>=threshold){if(i<7)++earlyKept;return true;}
 }
 return false;
}
inline bool keep(float meters,float size,bool wasHidden){
 if(!enabled||pixels<=0||!std::isfinite(meters)||!std::isfinite(size)||meters<distance)return true;
 return size >= pixels*(wasHidden?1.25f:1.f);
}
static void load(const char*root){
 auto read=[&](const char*name,int fallback,int high){std::ifstream f(std::string(root)+"/"+name);int v=fallback;if(!(f>>v))v=fallback;return std::clamp(v,0,high);};
 enabled=read("reflection-sprite-lod.enabled",1,1)!=0;distance=float(read("reflection-sprite-lod-distance",50,100));pixels=read("reflection-sprite-lod-half-pixels",4,64)*.5f;
 history.clear();projectedEyes=projectedCorners=earlyKept=0;tested=nearKept=rejected=emptyPasses=0;cpu=0;
}
}
