/* Original persistent landscape planes, NOT the view-local volatile shadow buffer.
 * One sweep takes 11 accepted snapshots; transfer only changed 256x256 tile pairs.
 * All cache/scan updates are committed after the snapshot is delivered. */
#define SURFACE_TILES 256u
#define SURFACE_PIXELS 65536u
#define SURFACE_PAIR (2u*SURFACE_PIXELS)
#define SURFACE_ROW (4u+SURFACE_PAIR)
#define SURFACE_BATCH 4u
typedef struct {
 uint64_t epoch;uint32_t mesh,collection,refs,last_tick,cursor;
 unsigned char valid[SURFACE_TILES];unsigned char*cache;
} SurfaceCache;
typedef struct {
 uint64_t epoch;uint32_t mesh,collection,refs,tick,next,count;int reset,valid;
 unsigned char data[SURFACE_BATCH*SURFACE_ROW];
} SurfacePending;
static int surface_read(uint32_t a,void*p,size_t n){return a>=0x10000&&n<=SURFACE_PIXELS&&a<=UINT32_MAX-n&&pread(fd,p,n,a)==(ssize_t)n;}
static void surface_collect(SurfaceCache*c,SurfacePending*p,uint32_t tick,uint64_t now){
 memset(p,0,offsetof(SurfacePending,data));
 unsigned char meshptr[4],baseptr[4],mesh[1004],head[320],refs[SURFACE_TILES*2*128],verify[4];
 if(!optional_read(0x529bd4,meshptr,4)||!optional_read(0x52b4d4,baseptr,4))return;
 uint32_t mp=u32(meshptr),bp=u32(baseptr);
 if(!surface_read(mp,mesh,sizeof(mesh))||u16(mesh+1000)>=256)return;
 unsigned tiles=u16(mesh+8)*u16(mesh+10);if(!tiles||tiles>SURFACE_TILES)return;size_t refbytes=tiles*2*128;
 uint32_t cp=bp+u16(mesh+1000)*320;
 if(!surface_read(cp,head,sizeof(head))||!(u32(head+4)&256)||u32(head+96)!=tiles*2)return;
 uint32_t rp=u32(head+108);if(!surface_read(rp,refs,refbytes))return;
 p->reset=!c->epoch||mp!=c->mesh||cp!=c->collection||rp!=c->refs||tick<c->last_tick;
 p->epoch=p->reset?(now?now:1):c->epoch;p->mesh=mp;p->collection=cp;p->refs=rp;p->tick=tick;
 uint32_t cursor=p->reset?0:c->cursor;
 for(unsigned j=0;j<SURFACE_BATCH;j++){
  unsigned tile=(cursor+j)%tiles;unsigned char*r=p->data+p->count*SURFACE_ROW;
  memcpy(r,&tile,4);
  for(unsigned plane=0;plane<2;plane++){
   unsigned ref=tile*2+plane;uint32_t pixels=u32(refs+ref*128+96);
   /* The original landscape accessor uses +0x60 raw rows with fixed 256 stride. */
   if(!surface_read(pixels,r+4+plane*SURFACE_PIXELS,SURFACE_PIXELS))return;
  }
  if(p->reset||!c->valid[tile]||memcmp(c->cache+tile*SURFACE_PAIR,r+4,SURFACE_PAIR))p->count++;
 }
 if(!optional_read(0x529bd4,verify,4)||u32(verify)!=mp)return;
 if(!optional_read(cp+108,verify,4)||u32(verify)!=rp)return;
 /* Pointers can change during a reload without the pool headers changing. */
 unsigned char refs_after[SURFACE_TILES*2*128];if(!surface_read(rp,refs_after,refbytes)||memcmp(refs,refs_after,refbytes))return;
 p->next=(cursor+SURFACE_BATCH)%tiles;p->valid=1;
}
static void surface_commit(SurfaceCache*c,const SurfacePending*p){
 if(!p->valid)return;
 if(p->reset)memset(c->valid,0,sizeof(c->valid));
 c->epoch=p->epoch;c->mesh=p->mesh;c->collection=p->collection;c->refs=p->refs;c->last_tick=p->tick;c->cursor=p->next;
 for(unsigned i=0;i<p->count;i++){const unsigned char*r=p->data+i*SURFACE_ROW;unsigned tile=u32(r);memcpy(c->cache+tile*SURFACE_PAIR,r+4,SURFACE_PAIR);c->valid[tile]=1;}
}
