#pragma once
#include <cstdint>
#include <cstring>
#include <vector>
struct LiveLight {uint32_t source,domain,key;int32_t x,y,z,ox,oy;unsigned w,h,alpha,transfer,phase,heightMode;std::vector<uint8_t> pixels;};
static bool parseLiveLights(const uint8_t*b,size_t n,std::vector<LiveLight>&out){
 if(n<4||n>262144)return false;uint32_t count;memcpy(&count,b,4);if(count>256)return false;size_t p=4;std::vector<LiveLight> next;
 for(unsigned i=0;i<count;i++){
  if(n-p<64)return false;uint32_t v[16];memcpy(v,b+p,64);p+=64;
  if(v[1]<1||v[1]>3||v[2]>=0x1000000||!v[8]||!v[9]||v[8]>256||v[9]>256||v[10]>65535||v[11]>255||v[12]>65535||v[13]>2||v[14]||v[15])return false;
  int32_t ox=(int32_t)v[6],oy=(int32_t)v[7];if(ox<-65536||ox>65536||oy<-65536||oy>65536)return false;
  size_t bytes=v[8]*v[9];if(bytes>n-p)return false;
  LiveLight l{v[0],v[1],v[2],(int32_t)v[3],(int32_t)v[4],(int32_t)v[5],ox,oy,v[8],v[9],v[10],v[11],v[12],v[13],{}};l.pixels.assign(b+p,b+p+bytes);p+=bytes;next.push_back(std::move(l));
 }
 if(p!=n)return false;out.swap(next);return true;
}
