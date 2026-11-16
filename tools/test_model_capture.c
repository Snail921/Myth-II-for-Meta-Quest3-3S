#define main capture_entry
#include "../src/bridge/capture.c"
#undef main
#include <assert.h>
static void put(uint32_t a,const void*p,size_t n){assert(pwrite(fd,p,n,a)==(ssize_t)n);}
int main(void){
 char name[]="/data/local/tmp/myth-model-test-XXXXXX";fd=mkstemp(name);assert(fd>=0);unlink(name);
 unsigned char header[64]={0},records[396]={0},definition[64]={0},out[512*36];
 uint16_t stride=132,count=3,owner=1,staticOwner=0xffff;uint32_t pool=0x10000,defs=0x20000,key=0x30316267;
 memcpy(header+24,&stride,2);memcpy(header+46,&count,2);memcpy(header+52,&pool,4);
 records[0]=1;memcpy(records+70,&staticOwner,2);records[132]=2;memcpy(records+132+70,&owner,2);
 for(int i=0;i<12;i++)records[132+8+i]=(unsigned char)(i+1);
 for(int i=0;i<18;i++)records[132+96+i]=(unsigned char)(i+16);
 memcpy(definition+4,&key,4);put(0x529d18,header,64);put(0x529ec4,&defs,4);put(pool,records,sizeof(records));put(defs,definition,64);
 assert(collect_models(out)==1);assert(u32(out)==key);assert(!memcmp(out+4,records+140,12));assert(!memcmp(out+16,records+228,18));assert(u16(out+34)==owner);
 records[132]=0;put(pool,records,sizeof(records));assert(collect_models(out)==0);
 header[24]=0;put(0x529d18,header,64);assert(collect_models(out)==0xffffffff);
 close(fd);puts("PASS animated model capture: static/inactive exclusion, geometry/position/matrix/owner, empty/invalid pool");return 0;
}
