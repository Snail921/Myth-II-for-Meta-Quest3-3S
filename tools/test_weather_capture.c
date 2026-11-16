#define main capture_entry
#include "../src/bridge/capture.c"
#undef main
#include <assert.h>
static void put(uint32_t a,const void*p,size_t n){assert(pwrite(fd,p,n,a)==(ssize_t)n);}
static void p32(uint32_t a,uint32_t v){put(a,&v,4);}
static void p16(uint32_t a,uint16_t v){put(a,&v,2);}
int main(){
 char name[]="/data/local/tmp/weather-test-XXXXXX";fd=mkstemp(name);assert(fd>=0);unlink(name);assert(!ftruncate(fd,0x600000));
 uint32_t def=0x10000,pool=0x20000,cols=0x30000,wind=0x50000;
 p32(0x54f928,def);p32(0x52b4d4,cols);p16(def+120,3);p16(def+20,0);p16(def+92,8);p16(def+26,192);p32(def,5);p32(def+12,14336);p32(def+16,18432);
 p16(0x52ac38+24,36);p16(0x52ac38+46,2);p32(0x52ac38+52,pool);p16(pool,7);p16(pool+4,1);p32(pool+8,500);p32(pool+12,600);p32(pool+16,700);p32(pool+24,-65536);p32(pool+28,65536);p32(pool+32,-65*65536);
 p32(0x544348,wind);p32(wind+44,32768);
 unsigned char out[PARTICLE_MAX*PARTICLE_ROW];assert(collect_weather(out,0)==1);assert(u32(out+60)==2&&u32(out+16)==500&&u32(out+20)==600&&u32(out+24)==700);assert(u32(out+28)==(14336u|(18432u<<16)));assert(u32(out+36)==8&&u32(out+40)==(uint32_t)-32768&&u32(out+48)==65536&&u32(out+56)==(uint32_t)(-65*65536));
 p16(pool+2,7);p16(def+20,1);p16(def+26,47);p16(def+94,1536);p16(def+96,0);assert(collect_weather(out,0)==1);assert(u32(out+8)==1&&u32(out+32)==47&&((u32(out+44)>>8)&63)==7&&(u32(out+44)>>16)==1536);
 p16(pool+2,64);assert(collect_weather(out,0)==0);p16(pool+2,0);
 p16(def+92,0);assert(collect_weather(out,0)==0);p16(def+92,8);p16(0x52ac38+24,88);assert(collect_weather(out,0)==0);p32(0x54f928,0);assert(collect_weather(out,0)==0);
 close(fd);puts("PASS weather original pool: frame/position, periodic cells, fade limits, wind velocity, invalid state and no weather");
}
