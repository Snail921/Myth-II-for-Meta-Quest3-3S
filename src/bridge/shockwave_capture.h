/* Original mesh-effect ring renderer 0x436a50 / 0x436b42.
 * Separate from LPGR emitters. Wire kind 3: radius/width in 1/512 world units,
 * segment count in word 10; original opacity is inverse transfer transparency. */
static unsigned collect_shockwaves(unsigned char*out,unsigned count){
 unsigned char pool[64],after[64],ptr[4],effect[32],def[36],head[320];
 if(!optional_read(0x529bf0,pool,64)||u16(pool+24)!=32||u16(pool+46)>4096||!optional_read(0x529cbc,ptr,4))return count;
 uint32_t defs=u32(ptr);if(!optional_read(0x52b4d4,ptr,4))return count;uint32_t cols=u32(ptr);unsigned first=count;
 for(unsigned i=0;i<u16(pool+46)&&count<PARTICLE_MAX;i++){
  if(!optional_read(u32(pool+52)+i*32,effect,32)||!u16(effect)||u16(effect+4)>=1024||!optional_read(defs+u16(effect+4)*36,def,36)||!(u32(def)&2))continue;
  int age=(int32_t)u32(effect+16),rate=(int16_t)u16(def+10),start=(int16_t)u16(def+12);
  if(age<0||age>65535)continue;
  int radius=(int32_t)((uint32_t)age*(uint32_t)rate)+start;
  int limit=(int16_t)u16(def+6),width=(int16_t)u16(def+24),segments=(int16_t)u16(def+22);
  int ratio=limit?(int16_t)(((int16_t)radius*256)/limit):0;
  int transparency=32768+ratio*128;if(transparency<0)transparency=0;if(transparency>65535)transparency=65535;
  unsigned col=u16(def+30),seq=u16(def+20),frame=u16(effect+20),palette=u16(def+32);
  if(radius<=0||radius>32767||width<=0||width>32767||segments<3||segments>128||col>=255||seq>255||frame>255||palette>=128||transparency==65535||!collection_header(cols,col,head,320))continue;
  uint32_t hash=2166136261u;unsigned offsets[]={64,96,112,128,252};for(unsigned k=0;k<5;k++)for(unsigned j=0;j<4;j++)hash=(hash^head[offsets[k]+j])*16777619u;
  uint32_t row[16]={(i<<16)|u16(effect),hash,seq,frame,(int16_t)u16(effect+8)*512,(int16_t)u16(effect+10)*512,u32(effect+12)+(int16_t)u16(def+26),65535-transparency,radius,width,segments,0,0,col,palette,3};
  memcpy(out+count*PARTICLE_ROW,row,sizeof(row));count++;
 }
 if(!optional_read(0x529bf0,after,64)||memcmp(pool,after,64))return first;
 return count;
}
