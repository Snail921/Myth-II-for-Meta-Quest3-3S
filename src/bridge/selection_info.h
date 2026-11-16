/* Build 434: read-only equivalents of 43e529/43e55f/46535c.
 * No call to 46530c: it consumes the game's UI dirty flags. */
#define SELECTION_INFO_BYTES 1828
static int info_string(int table,int index,char*out,size_t capacity){
 unsigned char descriptor[16],pointer[4];out[0]=0;
 uint32_t base=word(0x528b14);
 if(table<0||table>4095||index<0||!optional_read(base+table*16,descriptor,16)||index>=(int16_t)u16(descriptor))return 0;
 if(!optional_read(u32(descriptor+12)+index*4,pointer,4))return 0;
 uint32_t address=u32(pointer);
 for(size_t n=0;n+1<capacity;){
  unsigned char chunk[32];size_t bytes=capacity-1-n;if(bytes>32)bytes=32;size_t page=4096-((address+n)&4095);if(bytes>page)bytes=page;
  if(!optional_read(address+(uint32_t)n,chunk,bytes)){out[0]=0;return 0;}
  for(size_t j=0;j<bytes;j++){out[n++]=(char)chunk[j];if(!chunk[j])return 1;}
 }
 out[capacity-1]=0;return 1;
}
static void info_append(char*out,size_t size,const char*s){
 size_t used=strlen(out);if(!s[0]||used+2>=size)return;
 snprintf(out+used,size-used,"%s%s",used?", ":"",s);
}
/* Read-only 428bdf lookup in the loaded stli registry; never load tags remotely. */
static int info_string_tag(uint32_t tag){
 unsigned char header[0x58];if(!optional_read(0x528ad0,header,sizeof(header)))return -1;
 int count=(int16_t)u16(header+0x54);uint32_t base=u32(header+0x48);
 if(count<0||count>4096)return -1;
 for(int end=count;end>0;){unsigned char ids[1024];int start=end>256?end-256:0;
  if(!optional_read(base+start*4,ids,(size_t)(end-start)*4))return -1;
  for(int i=end-1;i>=start;i--)if(u32(ids+(i-start)*4)==tag)return i;
  end=start;
 }return -1;
}
static void info_level_title(char*out,size_t capacity){
 /* 458919: scenario name for kind 13, otherwise localized game-kind name.
  * 675820 is UI working storage and must not be used as authoritative text. */
 unsigned char field[2];char name[192]={0},difficulty[64]={0};int kind=(int16_t)(word(0x67ec08)&65535);
 uint32_t base=word(kind==13?0x529bd4:0x67ebe4);
 if(base&&optional_read(base+(kind==13?0x3e4:0x12),field,2))
  info_string((int16_t)u16(field),kind==13?0:kind,name,sizeof(name));
 info_string(info_string_tag(0x696e646c),(int16_t)(word(0x67ec04)&65535),difficulty,sizeof(difficulty));
 if(name[0]&&difficulty[0])snprintf(out,capacity,"%s  (%s)",name,difficulty);
 else snprintf(out,capacity,"%s",name);
}
static void read_selection_info(const unsigned char*actors,unsigned ac,const unsigned char*selection,unsigned char*out){
 memset(out,0,SELECTION_INFO_BYTES);unsigned count=u16(selection);if(count>126)return;
 memcpy(out,&count,4);char*title=(char*)out+4,*names=title+256,*flavor=names+512;
 if(!count){info_level_title(title,256);return;}

 unsigned groups[1024]={0};uint32_t definitions=word(0x52a6f4);
 for(unsigned i=0;i<count;i++){
  unsigned slot=u16(selection+4+i*2);if(slot>=ac)continue;
  const unsigned char*a=actors+slot*216;unsigned definition=u16(a+2);if(definition>=1024)continue;
  groups[definition]++;unsigned char d[1024];if(!optional_read(definitions+definition*1024,d,1024))continue;
  if(count==1){int table=(int16_t)u16(d+0x3e4),index=(int8_t)a[0x90];unsigned char desc[16];
   if(table>=0&&table<4096&&index>=0&&optional_read(word(0x528b14)+table*16,desc,16)&&u16(desc)>0)info_string(table,index%u16(desc),flavor,1024);
  }
  unsigned original=u16(a+0xb0);if(original!=definition&&original<1024){unsigned char old[1024];if(optional_read(definitions+original*1024,old,1024)&&u32(d+4)!=u32(old+4))memcpy(d,old,1024);}
  char name[128]={0};if(a[0xc6]){memcpy(name,a+0xc6,18);name[18]=0;}else info_string((int16_t)u16(d+0x3e2),(int8_t)a[0x90],name,sizeof(name));
  info_append(names,512,name);
 }
 /* Original 465906: ammunition icon/count, only a single friendly selection.
  * Byte 8c is signed; zero/negative hides the icon. No class-name assumptions. */
 if(count==1){unsigned slot=u16(selection+4);if(slot<ac){const unsigned char*a=actors+slot*216;
  if((int8_t)a[0x8c]>0&&((int8_t)a[0x16]==(int32_t)word(0x52ae6c)||(word(0x67ec06)&65535)==0)){
   unsigned definition=u16(a+2),object=u16(a+12);unsigned char d[1024],o[80];
   if(definition<1024&&object<8192&&optional_read(definitions+definition*1024,d,1024)&&optional_read(word(0x52a9f4)+object*80,o,80)&&o[0x2a]<255&&u16(d+0x24)<256){
    uint32_t inventory[4]={a[0x8c],o[0x2a],o[0x2c],u16(d+0x24)};memcpy(out+1812,inventory,16);
   }
  }
 }}
 /* Match 46544a: representative maximizes family + definition + appearance frequency. */
 unsigned best=0,representative=0;
 for(unsigned i=0;i<count;i++){unsigned slot=u16(selection+4+i*2);if(slot>=ac)continue;const unsigned char*a=actors+slot*216;unsigned score=0;
  for(unsigned j=0;j<count;j++){unsigned other=u16(selection+4+j*2);if(other>=ac)continue;const unsigned char*b=actors+other*216;score+=(u16(a+20)==u16(b+20))+(u16(a+2)==u16(b+2))+(u16(a+0x94)==u16(b+0x94));}
  if(score>best){best=score;representative=slot;}
 }
 if(best){const unsigned char*a=actors+representative*216;unsigned char d[1024],appearance[384];unsigned def=u16(a+2),app=u16(a+0x94);
  if(def<1024&&app<1024&&optional_read(definitions+def*1024,d,1024)&&optional_read(word(0x528f9c)+app*384,appearance,384)){
   uint32_t values[4]={u16(a),u16(appearance+0x174),u16(d+0x18),0};memcpy(out+1796,values,16);
  }
 }
 for(unsigned i=0;i<1024;i++)if(groups[i]){
  unsigned char d[1024];char type[128]={0},group[160];if(!optional_read(definitions+i*1024,d,1024))continue;
  int table=(int16_t)u16(d+0x3e0);if(!info_string(table,groups[i]>1?1:0,type,sizeof(type))||!type[0])info_string(table,0,type,sizeof(type));
  if(!type[0])snprintf(type,sizeof(type),"Unit");
  if(count==1)snprintf(group,sizeof(group),"%s",type);else snprintf(group,sizeof(group),"%u %s",groups[i],type);
  info_append(title,256,group);
 }
}

static void collect_selection_info(const unsigned char*actors,unsigned ac,const unsigned char*selection,unsigned char*out){
 static unsigned char previous[256],cached[SELECTION_INFO_BYTES];static uint64_t last=0;static uint32_t previousBase=0;
 struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);uint64_t now=(uint64_t)t.tv_sec*1000000000ull+t.tv_nsec;uint32_t base=word(0x52a6f4);
 if(!last||now-last>=200000000ull||base!=previousBase||memcmp(previous,selection,256)){
  read_selection_info(actors,ac,selection,cached);memcpy(previous,selection,256);previousBase=base;last=now;
 }
 memcpy(out,cached,SELECTION_INFO_BYTES);
}
