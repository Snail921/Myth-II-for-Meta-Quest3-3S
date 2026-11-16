#include "../src/vr/surface_compositor.h"
#include <cassert>
#include <cstdio>
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);unsigned n;assert(fread(&n,4,1,f)==1);for(unsigned i=0;i<n;i++){int v[6];assert(fread(v,24,1,f)==1);int offset;int strength=surface::place(v[0],v[1],v[2],v[3],offset);if(strength!=v[4]||(strength&&offset!=v[5])){printf("FAIL h=%d kind=%d transfer=%d phase=%d strength=%d expected=%d offset=%d expected=%d\n",v[0],v[1],v[2],v[3],strength,v[4],offset,v[5]);return 1;}}fclose(f);printf("PASS %u original shadow height/transfer cases\n",n);}
