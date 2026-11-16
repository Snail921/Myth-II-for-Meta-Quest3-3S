#include <map>
#include <array>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <cassert>
#include <cstdio>
#include "../src/vr/hd_codec.h"
struct Activity{const char*internalDataPath;};struct android_app{Activity*activity;};
struct Object{float yaw=0,scale=1;unsigned col=1,seq=0,frame=0;};
struct Quad{unsigned flags=0,views=8,texture=0,seqflags=0;float scale=1,w=16,h=32,rx=8,ry=32;};
static uint64_t key(unsigned c,unsigned s,unsigned f,unsigned d){return uint64_t(c)<<32|s<<16|f<<8|d;}
static std::map<uint64_t,Quad>quads;
struct Source{unsigned w,h;std::vector<uint8_t>pairs;};static std::vector<Source>spriteIndices;
namespace hd{struct Alpha{unsigned w,h;std::vector<uint8_t>pixels;};struct Entry{std::array<uint8_t,128>meta;};static std::map<uint64_t,Alpha>alphas;static std::map<uint64_t,Entry>entries;}
namespace surface{struct Mask{unsigned kind;int ox,oy,w,h;std::vector<uint8_t>pixels;};}
static struct{std::map<unsigned,surface::Mask>masks;}groundEffects;
#include "../src/vr/detail_shadows.h"
int main(){detailShadowsEnabled=true;for(unsigned i=0;i<8;i++){Quad q;q.texture=i;quads[key(1,0,0,i)]=q;spriteIndices.push_back({16,32,std::vector<uint8_t>(16*32*2,255)});}Object o;
 unsigned first=detailShadow(o);assert(first);assert(detailShadowMasks.begin()->first[0]==5);auto&m=groundEffects.masks.at(first);assert(m.w>1&&m.h>1);assert(*std::max_element(m.pixels.begin(),m.pixels.end())>100);assert(detailShadow(o)==first);
 o.yaw=.25f;unsigned second=detailShadow(o);assert(second&&second!=first);bool correct=false;for(auto&k:detailShadowMasks)correct|=k.first[0]==3;assert(correct);
 detailShadowsEnabled=false;assert(detailShadow(o)==0);detailShadowsEnabled=true;detailShadowBuilds=4;o.yaw=.5f;assert(detailShadow(o)==0);
 puts("PASS detailed shadow silhouette, quarter-turn frame selection, cache, OFF and per-update work limit");}
