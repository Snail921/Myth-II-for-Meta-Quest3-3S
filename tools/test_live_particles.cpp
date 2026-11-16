#include "../src/vr/live_particles.h"
#include <cassert>
#include <cstdio>
int main(){
 uint32_t count=1;LiveParticle v={1,1997670900,0,0,100,200,300,65535,256,256,0,0,0,1,0,0};std::vector<uint8_t>b(68);memcpy(b.data(),&count,4);memcpy(b.data()+4,v.data(),64);
 std::vector<LiveParticle> out;assert(parseParticles(b.data(),b.size(),out));assert(out.size()==1&&out[0][6]==300);
 for(size_t n=0;n<b.size();n++){assert(!parseParticles(b.data(),n,out));assert(out.size()==1&&out[0][0]==1);}
 auto bad=b;uint32_t huge=2049;memcpy(bad.data(),&huge,4);assert(!parseParticles(bad.data(),bad.size(),out));bad=b;memset(bad.data()+4+32,0,4);assert(!parseParticles(bad.data(),bad.size(),out));bad=b;bad.push_back(0);assert(!parseParticles(bad.data(),bad.size(),out));
 v={1,123,0,1,500,600,700,14336u|(18432u<<16),192,8,uint32_t(-65536),5,65536,71,uint32_t(-65*65536),2};memcpy(b.data()+4,v.data(),64);assert(parseParticles(b.data(),b.size(),out)&&out[0][15]==2);v[9]=0;memcpy(b.data()+4,v.data(),64);assert(!parseParticles(b.data(),b.size(),out));
 v={1,123,16,0,512,1024,1536,30000,51,512,32,0,0,50,0,3};memcpy(b.data()+4,v.data(),64);assert(parseParticles(b.data(),b.size(),out)&&out[0][15]==3);
 for(unsigned badSegments:{0u,2u,129u}){v[10]=badSegments;memcpy(b.data()+4,v.data(),64);assert(!parseParticles(b.data(),b.size(),out));}
 std::vector<uint8_t> empty(4);assert(parseParticles(empty.data(),4,out)&&out.empty());puts("PASS particle/ring atomic parsing, truncation, oversized count, invalid scale/segments, extra bytes and despawn");
}
