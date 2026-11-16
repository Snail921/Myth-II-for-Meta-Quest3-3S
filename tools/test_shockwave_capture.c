#define main capture_entry
#include "../src/bridge/capture.c"
#undef main
#include <assert.h>
static void put(uint32_t a,const void*p,size_t n){assert(pwrite(fd,p,n,a)==(ssize_t)n);}
static void put32(uint32_t a,uint32_t v){put(a,&v,4);}
static void put16(uint32_t a,uint16_t v){put(a,&v,2);}
int main(int argc,char**argv){
 assert(argc==2);char name[]="/data/local/tmp/myth-ring-XXXXXX";fd=mkstemp(name);assert(fd>=0);unlink(name);
 unsigned char zero[320]={0},out[PARTICLE_ROW*2];uint32_t effect=0x10000,def=0x20000,col=0x30000;
 put(0x529bf0,zero,64);put(effect,zero,32);put(def,zero,36);put(col,zero,320);
 put16(0x529bf0+24,32);put16(0x529bf0+46,1);put32(0x529bf0+52,effect);put32(0x529cbc,def);put32(0x52b4d4,col);
 put16(effect,1);put16(effect+8,12);put16(effect+10,24);put32(effect+12,100);
 put32(def,2);put16(def+6,1024);put16(def+10,51);put16(def+22,32);put16(def+24,512);put16(def+26,512);
 FILE*f=fopen(argv[1],"rb");assert(f);uint32_t count;assert(fread(&count,4,1,f)==1);
 for(unsigned i=0;i<count;i++){uint32_t r[6];assert(fread(r,4,6,f)==6);put32(effect+16,r[0]);unsigned n=collect_shockwaves(out,0);
  if(!r[3]||!r[5]){assert(!n);continue;}assert(n==1);assert(u32(out+16)==r[1]&&u32(out+20)==r[2]&&u32(out+32)==r[3]&&u32(out+24)==r[4]&&u32(out+28)==r[5]);assert(u32(out+36)==512&&u32(out+40)==32&&u32(out+60)==3);
 }
 fclose(f);put32(effect+16,1);put16(def+22,129);assert(!collect_shockwaves(out,0));put16(def+22,32);put32(def,0);assert(!collect_shockwaves(out,0));
 puts("PASS mesh-effect capture vs original oracle, expiry and malformed definition guards");close(fd);return 0;
}
