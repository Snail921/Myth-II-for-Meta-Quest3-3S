/* Original emitter/particle render state, independent of the flat viewport. */
#define PARTICLE_MAX 2048u
#define PARTICLE_ROW 64u
/* Build 434 renderer 0x42fd63..0x42fd9a: flag 0x02000000 snaps Z
 * to cell+8. It is a media-surface effect, NOT a reflection-only particle. */
static int particle_media_height(const unsigned char*grid,uint32_t*position){
 unsigned h=u16(grid),w=u16(grid+2);uint32_t cells=u32(grid+10);
 if(!w||!h||w>1024||h>1024||cells<0x10000)return 0;
 int x=(int32_t)position[0]>>9,y=(int32_t)position[1]>>9;
 if(x<0)x=0;if(y<0)y=0;if(x>=(int)w)x=w-1;if(y>=(int)h)y=h-1;
 uint64_t address=(uint64_t)cells+((uint64_t)y*w+x)*12+8;
 unsigned char value[2];if(address>UINT32_MAX||!optional_read((uint32_t)address,value,2))return 0;
 position[2]=(uint32_t)(int32_t)(int16_t)u16(value);return 1;
}
static unsigned collect_particles(unsigned char*out){
 unsigned char ph[64],eh[64],em[88],p[88],def[128],ptr[4],head[320],seq[12],after[64],visited[8192]={0};
 uint32_t sequenceAddress=0;uint32_t sigs[256]={0};unsigned count=0;unsigned char mediaGrid[14]={0};optional_read(0x67ec62,mediaGrid,sizeof(mediaGrid));
 if(!optional_read(0x529638,ph,64)||!optional_read(0x529678,eh,64)||u16(ph+24)!=88||u16(eh+24)!=88||u16(ph+46)>8192||u16(eh+46)>1024||!optional_read(0x5297cc,ptr,4))return 0;
 uint32_t defs=u32(ptr);if(!optional_read(0x52b4d4,ptr,4))return 0;uint32_t cols=u32(ptr);
 unsigned char effects[64]={0},effect[32];unsigned effectCount=0;
 if(optional_read(0x529cd8,effects,64)&&u16(effects+24)==32&&u16(effects+46)<=4096)effectCount=u16(effects+46);
 for(unsigned i=0;i<u16(eh+46)+effectCount&&count<PARTICLE_MAX;i++){
  int ground=i>=u16(eh+46);unsigned emitterSlot=i;
  if(ground){if(!optional_read(u32(effects+52)+(i-u16(eh+46))*32,effect,32)||!u16(effect))continue;emitterSlot=u16(effect+2);if(emitterSlot>=u16(eh+46))continue;}
  memset(visited,0,sizeof(visited));
  if(!optional_read(u32(eh+52)+emitterSlot*88,em,88)||!u16(em)||(!ground&&!em[80])||u16(em+4)>=1024||!optional_read(defs+u16(em+4)*128,def,128))continue;
  unsigned col=u16(def+122);if(col>=255||!collection_header(cols,col,head,320))continue;
  if(!sigs[col]){uint32_t hash=2166136261u;unsigned offsets[]={64,96,112,128,252};for(unsigned k=0;k<5;k++)for(unsigned j=0;j<4;j++)hash=(hash^head[offsets[k]+j])*16777619u;sigs[col]=hash;}
  unsigned slot=u16(em+10);
  while(slot<u16(ph+46)&&!visited[slot]&&count<PARTICLE_MAX){
   visited[slot]=1;if(!optional_read(u32(ph+52)+slot*88,p,88))break;
   if(u16(p)&&(p[77]&8)&&p[72]<u32(head+128)&&u32(head+128)<=1024&&collection_sequence(head,p[72],seq,&sequenceAddress)&&p[73]<u16(seq+10)){
    unsigned alpha=light_fade(u16(def+44),p,u16(seq+10))*(ground?u16(effect+26):u16(em+86))>>16;
    int16_t sx=(int16_t)u16(em+82),sy=(int16_t)u16(em+84);if(sx==-1)sx=(int16_t)u16(p+70);if(sy==-1)sy=(int16_t)u16(p+70);if(ground)sx=sy=(int16_t)u16(p+70);
    unsigned palette=0;unsigned char palettePtr[4],paletteValue[2];int16_t group=(int16_t)u16(def+120),entry=(int16_t)u16(p+78);
    if(group>=0&&group<256&&entry>=0&&entry<192&&optional_read(0x528f9c,palettePtr,4)&&optional_read(u32(palettePtr)+0x176+(group*192+entry)*2,paletteValue,2))palette=u16(paletteValue)&127;
    if((u32(def)&0x8000000)&&u16(em+52)!=65535)palette=u16(em+52)&127;
    if(alpha&&sx>0&&sy>0){uint32_t v[16]={ (slot<<16)|u16(p),sigs[col],p[72],p[73],0,0,0,alpha,(unsigned)sx,(unsigned)sy,p[77]&3,u32(def),u16(p+68),col,palette,0};memcpy(v+4,p+44,12);if(ground)for(unsigned axis=0;axis<3;axis++)v[4+axis]=u32(p+32+axis*4)+u32(effect+4+axis*4);if((u32(def)&0x2000000)&&!particle_media_height(mediaGrid,v+4)){slot=u16(p+6);continue;}memcpy(out+count*PARTICLE_ROW,v,sizeof(v));count++;}
   }slot=u16(p+6);
  }
 }
 if(!optional_read(0x529638,after,64)||memcmp(ph,after,64)||!optional_read(0x529678,after,64)||memcmp(eh,after,64))return 0;
 if(effectCount&&(!optional_read(0x529cd8,after,64)||memcmp(effects,after,64)))return 0;
 return count;
}

/* Also expose ordinary FX sprites absent from the initially exported scene. */
static unsigned collect_object_fx(const unsigned char*objects,unsigned oc,unsigned char*out,unsigned count){
 unsigned char ptr[4],head[320];uint32_t signatures[256]={0};if(!optional_read(0x52b4d4,ptr,4))return count;uint32_t cols=u32(ptr);
 for(unsigned i=0;i<oc&&count<PARTICLE_MAX;i++){const unsigned char*o=objects+i*80;unsigned col=o[42];if(!u16(o)||(u32(o+4)&0x20000200u)||col>=255||!u16(o+2)||u16(o+2)>32767)continue;
  if(!signatures[col]){if(!collection_header(cols,col,head,320))continue;uint32_t hash=2166136261u;unsigned offsets[]={64,96,112,128,252};for(unsigned k=0;k<5;k++)for(unsigned j=0;j<4;j++)hash=(hash^head[offsets[k]+j])*16777619u;signatures[col]=hash;}
  unsigned alpha=65535;int phase=o[53]?(int)(int8_t)o[52]*65536/(int)(int8_t)o[53]:0;if(phase>65535)phase=65535;if(o[51]==1)alpha=65535-(unsigned)(uint16_t)phase;else if(o[51]==2)alpha=(uint16_t)phase;
  uint32_t v[16]={(i<<16)|u16(o),signatures[col],o[43],o[46],0,0,0,alpha,u16(o+2),u16(o+2),(u32(o+4)>>17)&3,0x400000,u16(o+28),col,o[44],1};memcpy(v+4,o+8,12);memcpy(out+count*PARTICLE_ROW,v,sizeof(v));count++;
 }return count;
}
