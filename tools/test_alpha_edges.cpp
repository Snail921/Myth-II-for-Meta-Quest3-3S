#include "../src/vr/alpha_edges.h"
#include <fstream>
#include <iterator>
#include <cstdio>
#include <cassert>
#include <cstring>
int main(int argc,char**argv){std::ifstream f(argv[1],std::ios::binary);unsigned w,h;f.read((char*)&w,4);f.read((char*)&h,4);std::vector<uint8_t>p((std::istreambuf_iterator<char>(f)),{}),old=p;extendAlphaEdges(p,w,h,true);unsigned fixed=0;
for(unsigned y=0;y<h;y++)for(unsigned x=0;x<w;x++){size_t i=(y*w+x)*4;assert(p[i+3]==old[i+3]);if(old[i+3]){assert(!memcmp(p.data()+i,old.data()+i,4));continue;}bool edge=false;for(int j=-1;j<=1;j++)for(int k=-1;k<=1;k++){unsigned xx=(int(x)+k+int(w))%w,yy=(int(y)+j+int(h))%h;edge|=old[(yy*w+xx)*4+3]>0;}if(edge){assert(!(p[i]==0&&p[i+1]==0&&p[i+2]==255));fixed++;}}
assert(fixed>0);printf("PASS original wire: %u transparent edge pixels repaired; every alpha and opaque pixel unchanged\n",fixed);}
