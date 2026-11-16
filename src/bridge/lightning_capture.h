/* ligh runtime state, 434 pool 529518 / 471b pool 5304c4.
 * 471b 459acb creates cubic controls; 459ed4 draws traveling, jittered segments.
 * Visual RNG is local and tick-seeded: never advance the engine RNG per VR eye.
 * Kind 4 FX row: start[4..6], end[10..12], scale[8], overlap[9]. */
#include <math.h>
static uint32_t lightning_random(uint32_t*s){*s=*s*1664525u+1013904223u;return *s>>16;}
static int lightning_range(uint32_t*s,int n){return n>0?(int)((uint64_t)lightning_random(s)*(unsigned)n>>16):0;}
static int32_t lightning_coord(const unsigned char*p,unsigned at){return (int32_t)u32(p+at);}
static double lightning_distance(const int32_t*a,const int32_t*b){double d=0;for(unsigned k=0;k<3;k++){double v=(double)b[k]-a[k];d+=v*v;}return sqrt(d);}
static void lightning_curve(const unsigned char*r,unsigned step,unsigned steps,int32_t*out){
 double t=(double)step/steps,s=1-t;
 for(unsigned k=0;k<3;k++)out[k]=(int32_t)(s*s*s*lightning_coord(r,16+k*4)+3*s*s*t*lightning_coord(r,40+k*4)+3*s*t*t*lightning_coord(r,52+k*4)+t*t*t*lightning_coord(r,28+k*4));
}
static unsigned lightning_segment(unsigned char*out,unsigned count,uint32_t id,uint32_t signature,unsigned col,unsigned palette,unsigned seq,unsigned frames,unsigned scale,unsigned overlap,const int32_t*a,const int32_t*b,uint32_t*seed){
 if(count>=PARTICLE_MAX||!frames||seq>255||!scale||scale>32767||overlap>32767)return count;
 if(a[0]==b[0]&&a[1]==b[1]&&a[2]==b[2])return count;
 uint32_t v[16]={id,signature,seq,(unsigned)lightning_range(seed,frames),a[0],a[1],a[2],65535,scale,overlap,b[0],b[1],b[2],col,palette,4};
 memcpy(out+count*PARTICLE_ROW,v,sizeof(v));return count+1;
}
static unsigned collect_lightning(unsigned char*out,unsigned count){
 unsigned char pool[64],after[64],r[104],def[64],head[320],seq[12];uint32_t address;
 if(!optional_read(0x529518,pool,64)||u16(pool+24)!=104||u16(pool+46)>32)return count;
 uint32_t defs=word(0x52962c),cols=word(MVR_COLLECTIONS);unsigned start=count;
 for(unsigned slot=0;slot<u16(pool+46)&&count<PARTICLE_MAX;slot++){
  if(!optional_read(u32(pool+52)+slot*104,r,104)||!u16(r)||u16(r+2)>=16||!u16(r+88)||u16(r+88)>1024)continue;
  if(!optional_read(defs+u16(r+2)*64,def,64))continue;
  int valid=1;for(unsigned at=16;at<64;at+=4){int32_t v=lightning_coord(r,at);if(v < -1048576 || v > 1048576)valid=0;}if(!valid)continue;
  unsigned col=u16(def+60),palette=u16(r+4),steps=u16(r+88),bolts=u16(def+56),scale=u16(def+28);
  if(col>=255||palette>=128||!bolts||bolts>32||!scale||scale>32767||!collection_header(cols,col,head,320))continue;
  uint32_t hash=2166136261u;unsigned offsets[]={64,96,112,128,252};for(unsigned k=0;k<5;k++)for(unsigned j=0;j<4;j++)hash=(hash^head[offsets[k]+j])*16777619u;
  unsigned mainseq=u16(def+22),forkseq=u16(def+20),mainframes=0,forkframes=0;
  if(mainseq<256&&collection_sequence(head,mainseq,seq,&address))mainframes=u16(seq+10);
  if(forkseq<256&&collection_sequence(head,forkseq,seq,&address))forkframes=u16(seq+10);
  if(mainframes>256||forkframes>256)continue;
  int32_t a[3],b[3];for(unsigned k=0;k<3;k++){a[k]=lightning_coord(r,16+4*k);b[k]=lightning_coord(r,28+4*k);}
  int maxdelta=0;for(unsigned k=0;k<3;k++){int64_t delta=(int64_t)b[k]-a[k];if(delta<0)delta=-delta;if(delta>1048576){maxdelta=-1;break;}if(delta>maxdelta)maxdelta=(int)delta;}if(maxdelta<=0)continue;
  int spread=(maxdelta*256/steps)*2/768;double upper=(double)(int16_t)u16(def+26)*(int32_t)u32(r+76),lower=(u32(def)&1)?upper-u32(def+16):0;
  uint32_t seed=(slot<<16)^u16(r)^word(MVR_TICK)*2654435761u;
  for(unsigned bolt=0;bolt<bolts&&count<PARTICLE_MAX;bolt++){
   int32_t previous[3];memcpy(previous,a,sizeof(a));
   for(unsigned j=1;j<=steps&&count<PARTICLE_MAX;j++){
    int32_t next[3];lightning_curve(r,j,steps,next);if(j<steps)for(unsigned k=0;k<3;k++)next[k]+=lightning_range(&seed,spread)-spread/2;
    double d=lightning_distance(a,next);
    if(d>lower&&d<upper){
     count=lightning_segment(out,count,(slot<<16)|u16(r),hash,col,palette,mainseq,mainframes,scale,u16(def+36),previous,next,&seed);
     if(j<steps&&(lightning_random(&seed)&0xc000)==0&&forkframes){
      int forks=u16(def+30)+lightning_range(&seed,u16(def+32));if(forks>32)forks=32;
      double length=lightning_distance(previous,next)*.5,angle=atan2((double)next[1]-previous[1],(double)next[0]-previous[0]);int32_t tail[3];memcpy(tail,next,sizeof(tail));
      for(int n=0;n<forks&&count<PARTICLE_MAX;n++){
       angle+=(lightning_range(&seed,u16(def+38)*2)-(int)u16(def+38))*(6.283185307179586/65536.0);
       int32_t tip[3]={tail[0]+(int32_t)(cos(angle)*length),tail[1]+(int32_t)(sin(angle)*length),tail[2]};
       count=lightning_segment(out,count,(slot<<16)|u16(r),hash,col,palette,forkseq,forkframes,scale,u16(def+34),tail,tip,&seed);memcpy(tail,tip,sizeof(tip));
      }
     }
    }
    memcpy(previous,next,sizeof(next));
   }
  }
 }
 if(!optional_read(0x529518,after,64)||memcmp(pool,after,64))return start;
 return count;
}
