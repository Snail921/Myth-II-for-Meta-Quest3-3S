/* View-independent original object/particle light masks. No guest function calls. */
#define LIGHT_LIMIT 262144u
#define LIGHT_COUNT 256u
typedef struct {uint32_t key;int valid;uint32_t pixels;int32_t ox,oy;uint16_t w,h,stride,frames;} LightShape;
typedef struct {unsigned count;LightShape shapes[512];uint32_t collections;} LightLookup;
static int32_t light_s32(const void*p){int32_t v;memcpy(&v,p,4);return v;}
static LightShape*light_shape(LightLookup*l,unsigned col,unsigned seq,unsigned frame){
 if(col>=255||seq>=256||frame>=256)return NULL;
 uint32_t key=(col<<16)|(seq<<8)|frame;
 for(unsigned i=0;i<l->count;i++)if(l->shapes[i].key==key)return &l->shapes[i];
 if(l->count>=512)return NULL;
 LightShape*s=&l->shapes[l->count++];memset(s,0,sizeof(*s));s->key=key;
 unsigned char head[160],sequence[12],map[32],bitmap[52],index[2];
 if(!collection_header(l->collections,col,head,sizeof(head))||seq>=u32(head+128)||u32(head+128)>1024)return s;
 uint32_t sp;
 if(!collection_sequence(head,seq,sequence,&sp)||!u16(sequence+8)||u16(sequence+8)>64||frame>=u16(sequence+10))return s;
 s->frames=u16(sequence+10);
 if(!optional_read(sp+64+frame*(46+2*u16(sequence+8)),index,2))return s;
 unsigned mi=u16(index);if(mi>=u32(head+144)||mi==65535||u32(head+144)>8192)return s;
 if(!optional_read(u32(head+156)+mi*32,map,sizeof(map))||!(u32(map)&1))return s;
 unsigned bi=u16(map+12);if(bi>=u32(head+96)||bi==65535||u32(head+96)>20000)return s;
 if(!collection_bitmap(head,bi,bitmap))return s;
 s->w=u16(bitmap);s->h=u16(bitmap+2);s->stride=u16(bitmap+4);s->pixels=u32(bitmap+48);
 if(!s->w||!s->h||s->w>256||s->h>256||s->stride<s->w||s->stride>1024||(u16(bitmap+6)&4))return s;
 s->ox=-(light_s32(map+4)>>10);s->oy=-(light_s32(map+8)>>10);s->valid=1;return s;
}
static unsigned light_fade(unsigned mode,const unsigned char*p,unsigned frames){
 int32_t elapsed=light_s32(p+80),duration=light_s32(p+84);if(mode==2||mode==4){elapsed=p[73]*p[75]+p[74];duration=frames*p[75];}
 if(mode<1||mode>4)return 65535;
 unsigned ratio=elapsed<duration&&duration>0?(unsigned)(((uint32_t)elapsed<<16)/duration):65535;
 return mode==1||mode==2?ratio:65535-ratio;
}
static void light_emit(unsigned char*out,unsigned*bytes,unsigned*count,uint32_t source,unsigned domain,LightShape*s,const unsigned char*pos,unsigned alpha,unsigned transfer,unsigned phase,unsigned height_mode){
 if(!s||!s->valid||!alpha||*count>=LIGHT_COUNT||64u+s->w*s->h>LIGHT_LIMIT-*bytes)return;
 unsigned char*r=out+*bytes;uint32_t values[16]={source,domain,s->key,0,0,0,(uint32_t)s->ox,(uint32_t)s->oy,s->w,s->h,alpha,transfer,phase,height_mode,0,0};memcpy(values+3,pos,12);memcpy(r,values,64);
 for(unsigned y=0;y<s->h;y++)if(!optional_read(s->pixels+y*s->stride,r+64+y*s->w,s->w))return;
 *bytes+=64+s->w*s->h;(*count)++;
}
static unsigned collect_lights(const unsigned char*objects,unsigned oc,unsigned char*out){
 LightLookup lookup={0};unsigned char ptr[4];unsigned bytes=4,count=0;
 if(!optional_read(0x52b4d4,ptr,4)){memset(out,0,4);return 4;}lookup.collections=u32(ptr);
 for(unsigned i=oc;i-->0;){const unsigned char*o=objects+i*80;if(!u16(o)||(u32(o+4)&0x200000))continue;
  LightShape*s=light_shape(&lookup,o[42],o[43],o[46]);if(!s||!s->valid)continue;
  int phase=o[53]?(int)(int8_t)o[52]*65536/(int)(int8_t)o[53]:0;if(phase>65535)phase=65535;
  light_emit(out,&bytes,&count,(i<<16)|u16(o),1,s,o+8,65535,o[51],(uint16_t)phase,0);
 }
 unsigned char ph[64],eh[64],emitter[88],particle[88],definition[128],after[64];
 if(!optional_read(0x529638,ph,64)||!optional_read(0x529678,eh,64)||u16(ph+24)!=88||u16(eh+24)!=88||u16(ph+46)>8192||u16(eh+46)>1024||!optional_read(0x5297cc,ptr,4))goto done;
 uint32_t defs=u32(ptr);unsigned char visited[8192]={0};
 unsigned char effects[64]={0},effect[32];unsigned effectCount=0;
 if(optional_read(0x529cd8,effects,64)&&u16(effects+24)==32&&u16(effects+46)<=4096)effectCount=u16(effects+46);
 for(unsigned i=0;i<u16(eh+46)+effectCount;i++){
  int ground=i>=u16(eh+46);unsigned emitterSlot=i;
  if(ground){if(!optional_read(u32(effects+52)+(i-u16(eh+46))*32,effect,32)||!u16(effect))continue;emitterSlot=u16(effect+2);if(emitterSlot>=u16(eh+46))continue;}
  memset(visited,0,sizeof(visited));
  if(!optional_read(u32(eh+52)+emitterSlot*88,emitter,88)||!u16(emitter)||(!ground&&!emitter[80]))continue;
  unsigned di=u16(emitter+4);if(di>=1024||!optional_read(defs+di*128,definition,128)||(u32(definition)&0x2000000))continue;
  unsigned col=u16(definition+122),slot=u16(emitter+10);
  while(slot<u16(ph+46)&&!visited[slot]){
   visited[slot]=1;if(!optional_read(u32(ph+52)+slot*88,particle,88))break;
   if(u16(particle)&&(particle[77]&8)){
    LightShape*s=light_shape(&lookup,col,particle[72],particle[73]);
    if(s&&s->valid){unsigned alpha=(light_fade(u16(definition+44),particle,s->frames)*(ground?u16(effect+26):u16(emitter+86)))>>16;
     uint32_t pos[3];memcpy(pos,particle+44,12);if(ground)for(unsigned a=0;a<3;a++)pos[a]=u32(particle+32+a*4)+u32(effect+4+a*4);
     light_emit(out,&bytes,&count,ground?((i-u16(eh+46))<<16)|slot:(slot<<16)|u16(particle),ground?3:2,s,(unsigned char*)pos,alpha,0,0,(u32(definition)&0x400000)?1:2);
    }
   }slot=u16(particle+6);
  }
 }
 if(!optional_read(0x529638,after,64)||memcmp(ph,after,64)||!optional_read(0x529678,after,64)||memcmp(eh,after,64)){bytes=4;count=0;}
done:memcpy(out,&count,4);return bytes;
}
