#define main capture_entry
#ifdef TEST185
#include "../build/integration-185/capture.c"
#define LP 0x5304c4
#define LD 0x5305c8
#define CO 42
#define PO 48
#else
#include "../src/bridge/capture.c"
#define LP 0x529518
#define LD 0x52962c
#define CO 46
#define PO 52
#endif
#undef main
#include <assert.h>
static void put(uint32_t a,const void*p,size_t n){assert(pwrite(fd,p,n,a)==(ssize_t)n);}
static void p32(uint32_t a,uint32_t v){put(a,&v,4);}
static void p16(uint32_t a,uint16_t v){put(a,&v,2);}
int main(){
 char name[]="/data/local/tmp/lightning-test-XXXXXX";fd=mkstemp(name);assert(fd>=0);unlink(name);assert(!ftruncate(fd,0x700000));
 uint32_t rec=0x10000,def=0x20000,cols=0x30000,refs=0x50000,seq=0x60000;
 p16(LP+24,104);p16(LP+CO,1);p32(LP+PO,rec);p32(LD,def);p32(MVR_COLLECTIONS,cols);p32(MVR_TICK,1);
 p16(rec,17);p16(rec+4,0);p16(rec+88,8);p32(rec+76,10);
 p32(rec+28,4096);p32(rec+40,1024);p32(rec+52,3072);
 p16(def+20,0);p16(def+22,1);p16(def+26,512);p16(def+28,256);p16(def+30,2);p16(def+32,1);p16(def+38,12743);p16(def+56,2);p16(def+60,3);
 uint32_t head=cols+3*320;p32(head+128,2);
#ifdef TEST185
 p32(head+8,0x40000);p32(head+132,refs-0x40000);p32(refs+64,seq-0x40000);p32(refs+128+64,seq-0x40000);
#else
 p32(head+140,refs);p32(refs+72,seq);p32(refs+128+72,seq);
#endif
 p16(seq+10,2);
 unsigned char out[PARTICLE_MAX*PARTICLE_ROW],again[PARTICLE_MAX*PARTICLE_ROW];unsigned n=collect_lightning(out,0);assert(n>=16&&n<=64);assert(collect_lightning(again,0)==n&&!memcmp(out,again,n*64));
 for(unsigned i=0;i<n;i++){unsigned char*p=out+i*64;assert(u32(p+60)==4&&u32(p+52)==3&&u32(p+12)<2&&u32(p+28)==65535&&u32(p+32)==256);}
 p32(MVR_TICK,2);n=collect_lightning(again,0);assert(n&&memcmp(out,again,64*n));
 p16(def+56,1);p16(def+20,65535);p32(rec+76,2);n=collect_lightning(out,0);assert(n>0&&n<8);for(unsigned i=0;i<n;i++)assert(u32(out+i*64+40)<1200);
 p32(def,1);p32(def+16,512);p32(rec+76,100);assert(collect_lightning(out,0)==0);
 p32(def,0);p32(rec+76,10);assert(collect_lightning(out,PARTICLE_MAX-1)==PARTICLE_MAX);
 p16(rec,0);assert(collect_lightning(out,0)==0);p16(rec,17);p16(def+60,255);assert(collect_lightning(out,0)==0);p16(def+60,3);p16(LP+CO,33);assert(collect_lightning(out,0)==0);
 puts("PASS lightning pool, cubic endpoints, travel window, fork textures, tick-stable geometry, removal, bounds and capacity");close(fd);
}
