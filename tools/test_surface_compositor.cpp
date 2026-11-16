#include "../src/vr/surface_compositor.h"
#include <cassert>
#include <cstdio>
static void read(FILE*f,void*p,size_t n){assert(fread(p,1,n,f)==n);}
int main(int argc,char**argv){
 assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);unsigned count;read(f,&count,4);
 for(unsigned n=0;n<count;n++){int v[8];read(f,v,32);int size=v[0],w=v[1],h=v[2],stride=v[3];std::vector<uint8_t> dst(size*size),src(stride*h),expected(size*size);read(f,dst.data(),dst.size());read(f,src.data(),src.size());read(f,expected.data(),expected.size());surface::Mask m{(unsigned)v[6],0,0,w,h,{}};for(int y=0;y<h;y++)m.pixels.insert(m.pixels.end(),src.begin()+y*stride,src.begin()+y*stride+w);surface::apply(dst,size,size,m,{1,0,v[4],v[5],v[7]});assert(dst==expected);}
 assert(fgetc(f)==EOF);fclose(f);
 surface::Compositor c;c.w=147;c.h=139;c.base.resize(c.w*c.h);for(unsigned i=0;i<c.base.size();i++)c.base[i]=80+i%170;c.shade=c.base;
 for(unsigned k=0;k<3;k++){surface::Mask m{k,0,0,90,80,{}};for(int i=0;i<m.w*m.h;i++)m.pixels.push_back((i*7+k*13)%256);c.masks[k]=m;}
 for(int step=0;step<100;step++){
  std::vector<surface::Stamp> stamps={{1,2,50*64,45*64,65535},{2,0,step*71-1200,step*39-600,65279}};
  if(step<50)stamps.push_back({3,1,60*64+23,62*64+49,32768});
  if(step==99)stamps.clear();c.compose(stamps);auto expected=c.base;for(auto&s:stamps)surface::apply(expected,c.w,c.h,c.masks.at(s.mask),s);assert(c.shade==expected);
  c.compose(stamps);assert(c.tiles.empty());assert(c.shade==expected);
 }
 assert(c.shade==c.base);printf("PASS %u original stamp cases; 100 dirty-tile movement/overlap/deletion cases; unchanged snapshots\n",count);
}
