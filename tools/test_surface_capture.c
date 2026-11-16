#define main capture_entry
#include "../src/bridge/capture.c"
#undef main
#include <assert.h>
static void put(uint32_t a,const void*p,size_t n){assert(pwrite(fd,p,n,a)==(ssize_t)n);}
static void put32(uint32_t a,uint32_t v){put(a,&v,4);}
int main(void){
 char name[]="/data/local/tmp/myth-surface-test-XXXXXX";fd=mkstemp(name);assert(fd>=0);unlink(name);
 unsigned char mesh[1004]={0},head[320]={0},refs[84*128]={0},pixels[65536];
 uint32_t mp=0x30000,cp=0x20000,rp=0x40000,raw=0x1000000;
 mesh[8]=7;mesh[10]=6;head[5]=1;head[96]=84;memcpy(head+108,&rp,4);
 for(unsigned i=0;i<84;i++){uint32_t ptr=raw+i*65536;memcpy(refs+i*128+96,&ptr,4);memset(pixels,i,sizeof(pixels));put(ptr,pixels,sizeof(pixels));}
 put(mp,mesh,sizeof(mesh));put(cp,head,sizeof(head));put(rp,refs,sizeof(refs));put32(0x529bd4,mp);put32(0x52b4d4,cp);
 SurfaceCache c={0};c.cache=malloc(SURFACE_TILES*SURFACE_PAIR);SurfacePending*p=malloc(sizeof(*p));assert(c.cache&&p);
 surface_collect(&c,p,10,100);assert(p->valid&&p->reset&&p->count==4);assert(!c.epoch&&!c.cursor);
 surface_collect(&c,p,10,101);assert(p->valid&&p->count==4);surface_commit(&c,p);assert(c.epoch==101&&c.cursor==4);
 unsigned count=4;for(unsigned i=0;i<10;i++){surface_collect(&c,p,11+i,102+i);assert(p->valid);count+=p->count;surface_commit(&c,p);}assert(count==42);
 for(unsigned i=0;i<42;i++)assert(c.valid[i]);
 surface_collect(&c,p,21,200);assert(p->valid&&!p->count);surface_commit(&c,p);
 unsigned char blood=199;put(raw+10*65536+123,&blood,1);count=0;
 for(unsigned i=0;i<11;i++){surface_collect(&c,p,22+i,201+i);assert(p->valid);if(p->count){assert(p->count==1&&u32(p->data)==5&&p->data[4+123]==199);}count+=p->count;surface_commit(&c,p);}assert(count==1);
 surface_collect(&c,p,1,999);assert(p->valid&&p->reset&&p->epoch==999&&p->count==4);surface_commit(&c,p);assert(!c.valid[5]);
 put32(cp+108,0);surface_collect(&c,p,2,1000);assert(!p->valid);surface_commit(&c,p);assert(c.epoch==999);
 // A second map uses five by seven tiles; wrap and cache limits follow that map.
 mesh[8]=5;mesh[10]=7;head[96]=70;put(mp,mesh,sizeof(mesh));put(cp,head,sizeof(head));put32(cp+108,rp);memset(&c.valid,0,sizeof(c.valid));c.epoch=0;
 count=0;for(unsigned i=0;i<9;i++){surface_collect(&c,p,100+i,2000+i);assert(p->valid);count+=p->count;surface_commit(&c,p);}assert(count==35);
 for(unsigned i=0;i<35;i++)assert(c.valid[i]);for(unsigned i=35;i<SURFACE_TILES;i++)assert(!c.valid[i]);
 free(p);free(c.cache);close(fd);puts("PASS persistent capture: bounded initial sweep, no premature commit, unchanged suppression, blood delta, tick rollback, bad pointer");
}
