#include "../src/vr/live_lights.h"
#include "../src/vr/surface_compositor.h"
#include <cassert>
#include <cstdio>
int main(){
 std::vector<uint8_t>b(4+64+4);uint32_t count=1,v[16]={123,2,0x10001,1,2,3,0,0,2,2,65535,0,0,0,0,0};
 memcpy(b.data(),&count,4);memcpy(b.data()+4,v,64);std::fill(b.begin()+68,b.end(),255);
 std::vector<LiveLight> lights;assert(parseLiveLights(b.data(),b.size(),lights));assert(lights.size()==1&&lights[0].pixels.size()==4);
 for(size_t n=0;n<b.size();n++){assert(!parseLiveLights(b.data(),n,lights));assert(lights.size()==1&&lights[0].source==123);}
 auto bad=b;bad[8]=4;assert(!parseLiveLights(bad.data(),bad.size(),lights));bad=b;bad.push_back(0);assert(!parseLiveLights(bad.data(),bad.size(),lights));
 surface::Compositor c;c.w=128;c.h=128;c.base.assign(128*128,100);c.indices.assign(128*128,17);c.base[65*128+65]=80;c.shade=c.base;
 c.masks[0]={0,0,0,8,8,std::vector<uint8_t>(64,128)};c.masks[1]={1,0,0,8,8,std::vector<uint8_t>(64,255)};
 surface::Stamp shadow{1,0,63*64,63*64,65535},light{2,1,63*64,63*64,65535};
 c.compose({shadow});auto shaded=c.shade;c.compose({shadow,light});auto lit=c.shade;assert(lit[65*128+65]==255);assert(c.indices[65*128+65]==17);
 c.compose({shadow,light});assert(c.tiles.empty()&&c.shade==lit);c.compose({shadow});assert(c.shade==shaded);c.compose({});assert(c.shade==c.base);
 int offset;assert(surface::place(0,1,0,0,offset,0)==0);assert(surface::place(0,1,0,0,offset,32768)<surface::place(0,1,0,0,offset));
 puts("PASS atomic light parser, truncation, invalid domain, light saturation/removal, shadow and persistent base preservation");
}
