/* Original particle_systems.c: 0x452bcb updates 36-byte weather records;
 * 0x452e95 repeats them in XY tiles from definition+0x5c. No invented rain RNG.
 * FX row kind 2: [7] near|far, [8] scale, [9] tile width in cells,
 * [10,12,14] signed velocity XYZ, [11] definition flags (low 8), original random view (bits 8..13), vertical period (high 16). */
static unsigned collect_weather(unsigned char*out,unsigned count){
 unsigned char ptr[4],def[132],pool[64],after[64],head[320],p[36];
 if(!optional_read(0x54f928,ptr,4)||u32(ptr)<65536||!optional_read(u32(ptr),def,sizeof(def)))return count;
 if(!optional_read(0x52ac38,pool,64)||u16(pool+24)!=36||u16(pool+46)>1024||!optional_read(0x52b4d4,ptr,4))return count;
 unsigned col=u16(def+120),seq=u16(def+20),period=u16(def+92),scale=u16(def+26);
 unsigned near=(u32(def)&4)?u32(def+12):0,far=(u32(def)&4)?u32(def+16):18432;
 if(col>=255||seq>255||!period||period>512||!scale||scale>32767||near>=far||far>65535||!collection_header(u32(ptr),col,head,320))return count;
 uint32_t hash=2166136261u;unsigned offsets[]={64,96,112,128,252};for(unsigned k=0;k<5;k++)for(unsigned j=0;j<4;j++)hash=(hash^head[offsets[k]+j])*16777619u;
 unsigned char wind[12]={0};if(optional_read(0x544348,ptr,4)&&u32(ptr)>=65536)optional_read(u32(ptr)+44,wind,12);
 unsigned start=count;
 for(unsigned i=0;i<u16(pool+46)&&count<PARTICLE_MAX;i++){
  if(!optional_read(u32(pool+52)+i*36,p,36)||!u16(p)||u16(p+4)>255||u16(p+2)>63)continue;
  uint32_t v[16]={(i<<16)|u16(p),hash,seq,u16(p+4),u32(p+8),u32(p+12),u32(p+16),near|(far<<16),scale,period,u32(p+24),(u32(def)&255)|((uint32_t)u16(p+2)<<8)|((uint32_t)(uint16_t)((int16_t)u16(def+94)-(int16_t)u16(def+96))<<16),u32(p+28),col,u32(p+32),2};
  v[10]+=u32(wind);v[12]+=u32(wind+4);v[14]+=u32(wind+8);
  memcpy(out+count*PARTICLE_ROW,v,sizeof(v));count++;
 }
 if(!optional_read(0x52ac38,after,64)||memcmp(pool,after,64))return start;
 return count;
}
