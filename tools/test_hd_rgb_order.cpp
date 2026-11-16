#include "../src/vr/hd_codec.h"
#include <cassert>
#include <cstdio>
int main(){
 for(unsigned encoding:{2u,3u}){
  std::vector<uint8_t>b(52);b[1]=2;b[3]=1;b[6]=0x40;b[7]=0x20;b[17]=encoding;
  for(auto c:{std::array<uint8_t,4>{213,71,29,93},std::array<uint8_t,4>{9,102,241,255}}){b.push_back(c[0]);b.push_back(c[1]);b.push_back(c[2]);if(encoding==2)b.push_back(c[3]);}
  auto rgba=hd::rgba(hd::decode(b),{});
  if(encoding==3)assert((rgba==std::vector<uint8_t>{213,71,29,255,9,102,241,255}));
  else assert((rgba==std::vector<uint8_t>{29,71,213,93,241,102,9,255}));
  b.pop_back();bool rejected=false;try{hd::decode(b);}catch(const std::runtime_error&){rejected=true;}assert(rejected);
 }
 puts("PASS RGB24 RGB order, BGRA32 unchanged including alpha, truncated images rejected");
}
