#define main capture_entry
#include "../src/bridge/capture.c"
#undef main
#include <assert.h>
static void put(uint32_t a,const void*p,size_t n){assert(pwrite(fd,p,n,a)==(ssize_t)n);}
static void put32(uint32_t a,uint32_t v){put(a,&v,4);}
int main(void){
 char name[]="/data/local/tmp/myth-palette-test-XXXXXX";fd=mkstemp(name);assert(fd>=0);unlink(name);
 unsigned char actor[432]={0},object[160]={0},desc[20]={0},pixels[1024]={0},out[128*1028];
 uint32_t base=0x10000,table=0x30000,palette=0x40000,descriptor=0x50000;
 put32(0x52b4d4,base);put32(0x561860,descriptor);put32(base+14*320+192,2);put32(base+14*320+196,table);put32(table+4,palette);
 actor[0]=1;actor[216]=2;actor[228]=1;object[0]=1;object[42]=14;object[44]=1;memcpy(object+80,object,80);
 for(unsigned format=1;format<=3;format++){
  desc[2]=format;put(descriptor,desc,20);
  for(unsigned i=0;i<256;i++){if(format==3){uint32_t v=(i<<16)|((255-i)<<8)|73;memcpy(pixels+i*4,&v,4);}else{uint16_t v=format==2?0xf81f:0x7c1f;memcpy(pixels+i*2,&v,2);}}
  put(palette+148,pixels,format==3?1024:512);assert(collect_palettes(actor,2,object,2,out)==1);assert(u32(out)==(14|256));
  for(unsigned i=0;i<256;i++){unsigned char*p=out+4+i*4;assert(p[3]==255);if(format==3){assert(p[0]==i&&p[1]==255-i&&p[2]==73);}else assert(p[0]==255&&p[1]==0&&p[2]==255);}
 }
 object[44]=127;object[124]=127;assert(!collect_palettes(actor,2,object,2,out));
 object[44]=1;put32(table+4,0);assert(!collect_palettes(actor,2,object,2,out));
 put32(table+4,palette);desc[2]=4;put(descriptor,desc,20);assert(!collect_palettes(actor,2,object,2,out));
 close(fd);puts("PASS palette capture RGB888/565/555, duplicate pairs, invalid variant/pointer/format");return 0;
}
