#define _POSIX_C_SOURCE 200809L
#include "../engine/profile_434.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <stddef.h>
#include <signal.h>
/* Build-434-specific, read-only producer. No ptrace stops or game writes. */
static int fd;
static void read_exact(uint32_t address,void *dst,size_t n){
 if(address<0x10000 || n>2097152 || pread(fd,dst,n,(off_t)address)!=(ssize_t)n){perror("bounded pread");exit(2);}
}
static uint32_t u32(const void *p){uint32_t v;memcpy(&v,p,4);return v;}
static uint16_t u16(const void *p){uint16_t v;memcpy(&v,p,2);return v;}
static uint32_t word(uint32_t a){uint32_t v;read_exact(a,&v,4);return v;}
/* Original neutral sprite palette: collection+c4[object+2c], table+94.
 * Never change guest memory. Invalid optional palette data falls back to assets. */
static int optional_read(uint32_t a,void *p,size_t n){return a>=0x10000&&n<=4096&&pread(fd,p,n,(off_t)a)==(ssize_t)n;}
#include "coop_roster.h"
#include "collection_access.h"
#include "surface_capture.h"
#include "light_capture.h"
#include "particle_capture.h"
#include "shockwave_capture.h"
#include "weather_capture.h"
#include "lightning_capture.h"
// Original mana renderer 0x4bfc7b: actor+0x7a, definition+0x214; display flags+0x198.
static void collect_mana_definitions(const unsigned char*actors,unsigned count,uint16_t*out){
 uint16_t values[1024][2]={{0}};unsigned char seen[1024]={0};uint32_t base=word(MVR_MONSTER_DEFINITIONS);
 for(unsigned i=0;i<count;i++){
  out[i*2]=out[i*2+1]=0;int16_t def;memcpy(&def,actors+i*216+2,2);
  if(!u16(actors+i*216)||def<0||def>=1024||base<65536||base>0xffefffff)continue;
  if(!seen[def]){seen[def]=1;if(!optional_read(base+def*1024+0x214,&values[def][0],2)||!optional_read(base+def*1024+0x198,&values[def][1],2)){values[def][0]=values[def][1]=0;}}
  out[i*2]=values[def][0];out[i*2+1]=values[def][1];
 }
}
static uint32_t collect_models(unsigned char*out){
 unsigned char header[64],after[64],model[132],definition[64];
 if(!optional_read(0x529d18,header,64)||u16(header+24)!=132||u16(header+46)>1024)return 0xffffffff;
 uint32_t definitions=word(0x529ec4),n=0;
 for(unsigned i=0;i<u16(header+46);i++){
  if(!optional_read(u32(header+52)+i*132,model,132))return 0xffffffff;
  if(!u16(model)||u16(model+70)>=0x8000)continue;
  unsigned index=u16(model+4);if(index>=1024||n>=512)return 0xffffffff;
  if(!optional_read(definitions+index*64,definition,64))return 0xffffffff;
  unsigned char*r=out+n*36;memcpy(r,definition+4,4);memcpy(r+4,model+8,12);memcpy(r+16,model+96,18);memcpy(r+34,model+70,2);n++;
 }
 if(!optional_read(0x529d18,after,64)||memcmp(header,after,64))return 0xffffffff;
 return n;
}
static void append_palette(unsigned col,unsigned variant,unsigned char*out,unsigned*count){
 if(col>=255||variant>=128||*count>=128)return;uint32_t key=col|(variant<<8);unsigned j;for(j=0;j<*count;j++)if(u32(out+j*1028)==key)return;
 uint32_t base=word(MVR_COLLECTIONS),descriptor=word(0x561860);unsigned char desc[20];if(!optional_read(descriptor,desc,sizeof(desc)))return;unsigned format=u16(desc+2);if(format<1||format>3)return;
  unsigned char head[200],ptr[4],pixels[1024];
  if(!collection_header(base,col,head,sizeof(head))||variant>=u32(head+192)||u32(head+192)>128)return;
  if(!optional_read(u32(head+196)+variant*4,ptr,4))return;
  unsigned bytes=format==3?4:2;if(!optional_read(u32(ptr)+collection_palette_offset(),pixels,256*bytes))return;
  unsigned char*r=out+(*count)*1028;memcpy(r,&key,4);
  for(j=0;j<256;j++){unsigned v=bytes==4?u32(pixels+j*4):u16(pixels+j*2);unsigned char*p=r+4+j*4;
   if(format==3){p[0]=v>>16;p[1]=v>>8;p[2]=v;}else if(format==2){p[0]=((v>>11)&31)*255/31;p[1]=((v>>5)&63)*255/63;p[2]=(v&31)*255/31;}else{p[0]=((v>>10)&31)*255/31;p[1]=((v>>5)&31)*255/31;p[2]=(v&31)*255/31;}p[3]=255;
  }(*count)++;

}
/* 1.8.5 neutral table hue parameters consumed by original 0x4500a0. */
static unsigned collect_hd_hues(const unsigned char*palettes,unsigned count,unsigned char*out){
 unsigned n=0;
#if MVR_COLLECTION_RELATIVE
 for(unsigned i=0;i<count;i++){unsigned key=u32(palettes+i*1028),col=key&255,variant=key>>8;unsigned char head[200],ptr[4],hues[84];
  if(!collection_header(word(MVR_COLLECTIONS),col,head,200)||variant>=u32(head+192)||!optional_read(u32(head+196)+variant*4,ptr,4)||!optional_read(u32(ptr)+64,hues,84)||u32(hues+80)>8)continue;
  memcpy(out+n*88,&key,4);memcpy(out+n*88+4,hues,84);n++;
 }
#endif
 return n;
}
#include "selection_eligibility.h"
#include "selection_info.h"
static void collect_selection_eligibility(const unsigned char*actors,unsigned count,unsigned char*out){
 unsigned char cached[1024]={0},valid[1024]={0};uint32_t flags[1024]={0};int16_t kinds[1024]={0};
 uint32_t base=word(MVR_MONSTER_DEFINITIONS);int active=word(0x67ec06)&65535,game_kind=(int16_t)word(0x67ec08);
 for(unsigned i=0;i<count;i++){
  const unsigned char*a=actors+i*216;int16_t definition;memcpy(&definition,a+2,2);out[i]=0;
  if(!u16(a)||definition<0||definition>=1024||base<65536||base>0xffefffff)continue;
  if(!cached[definition]){cached[definition]=1;valid[definition]=optional_read(base+definition*1024,&flags[definition],4)&&optional_read(base+definition*1024+0x1b6,&kinds[definition],2);}
  if(valid[definition])out[i]=(unsigned char)!!original_selection_eligible(a[9],flags[definition],kinds[definition],active,game_kind);
 }
}
static unsigned collect_palettes(const unsigned char*actors,unsigned ac,const unsigned char*objects,unsigned oc,unsigned char*out){
 unsigned count=0;for(unsigned i=0;i<ac;i++){const unsigned char*a=actors+i*216;int16_t slot;memcpy(&slot,a+12,2);if(!u16(a)||slot<0||(unsigned)slot>=oc)continue;const unsigned char*o=objects+slot*80;if(u16(o)&&o[44])append_palette(o[42],o[44],out,&count);}return count;
}
static uint64_t ns(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return (uint64_t)t.tv_sec*1000000000+t.tv_nsec;}
static const char*mailbox="files/rootfs/home/xuser-1/.wine/drive_c/mythvr/command.bin";
static const char*temporary="files/rootfs/home/xuser-1/.wine/drive_c/mythvr/command.tmp";
static int32_t map_limit(unsigned axis){unsigned char ptr[4],dims[4];if(!optional_read(MVR_MESH,ptr,4)||!optional_read(u32(ptr)+8,dims,4))return 0;unsigned blocks=u16(dims+axis*2);return blocks&&blocks<=16?(int32_t)(blocks*32-1)*512:0;}
static const char*ackpath="files/rootfs/home/xuser-1/.wine/drive_c/mythvr/ack.bin";
static void receive_command(void){
 static unsigned char data[32];static size_t have;ssize_t n=recv(STDOUT_FILENO,data+have,32-have,MSG_DONTWAIT);
 if(n<=0)return;have+=(size_t)n;if(have<32)return;have=0;
 uint32_t op=u32(data+8),flags=u32(data+16);int32_t x,y;memcpy(&x,data+20,4);memcpy(&y,data+24,4);
 if(u32(data)!=0x31444d43||!u32(data+4)||op<1||op>24)return;
 if(op!=10&&op!=23&&((op!=21&&flags>1)||(op!=5&&op!=12&&op!=3&&op!=13&&op!=15&&u32(data+28))))return;
 if((op==1||op==4)&&(u32(data+12)==0||u32(data+12)>65535))return;
 if((op==3||op==5||op==12||op==13||op==15)&&(x<0||y<0||x>=map_limit(0)||y>=map_limit(1)))return;
 if((op==3||op==13||op==15)&&u32(data+28)&&(!(u32(data+28)&0x10000)||u32(data+28)>0x1ffff))return;
 if(op==5&&((int32_t)u32(data+28)<-32768||(int32_t)u32(data+28)>32767))return;
 if((op==19||op==20)&&(u32(data+12)>9||(op==19&&flags)||x||y||u32(data+28)))return;
 if(op==24&&(u32(data+12)>1||flags||x||y||u32(data+28)))return;
 if(op==23&&(u32(data+12)>=32||x||y))return;
 if(op==22&&(u32(data+12)<0x10000||u32(data+12)>0x1ffff||flags||x||y||u32(data+28)))return;
 if(op==21&&(!u32(data+12)||u32(data+12)>1023||(flags&65535)<51||(flags&65535)>55||x||y||u32(data+28)))return;
 if(op==18&&(u32(data+12)<100||u32(data+12)>200||flags||x||y||u32(data+28)))return;
 if(op==12&&(u32(data+12)>65535||flags>1||(int32_t)u32(data+28)<-32768||(int32_t)u32(data+28)>65536))return;
 if((op==16||op==17)&&(!(u32(data+12)&65535)||(u32(data+12)>>16)>=8192||flags||x||y||u32(data+28)))return;
 if(op==6&&(u32(data+12)>5||u32(data+12)==3))return;
 if(op==8&&u32(data+12)>9)return;
 if((op==9||op==11)&&u32(data+12)>126)return;
 FILE*f=fopen(temporary,"wb");if(!f)return;int ok=fwrite(data,1,32,f)==32;int closed=fclose(f);if(ok&&!closed)rename(temporary,mailbox);
 fprintf(stderr,"INPUT seq=%u op=%u id=%u x=%d y=%d\n",u32(data+4),op,u32(data+12),x,y);
}
static unsigned connector_edges(int32_t*out,const unsigned char*objects,unsigned oc){
 unsigned char h[64],r[512];unsigned n=0;
#if MYTH_ENGINE_BUILD == 471
 uint32_t pool=0x52ecbc,defs=word(0x52eca0);unsigned co=42,po=48;
#else
 uint32_t pool=0x527a18,defs=word(0x527ad4);unsigned co=46,po=52;
#endif
 if(!optional_read(pool,h,64)||u16(h+24)!=512||u16(h+co)>512)return 0;
 for(unsigned i=0;i<u16(h+co);i++){
 if(!optional_read(u32(h+po)+i*512,r,512))return 0;
 unsigned count=u16(r+6);if(!u16(r)||count>31)continue;
 unsigned char definition[32];if(!optional_read(defs+u16(r+2)*32,definition,32))continue;
 int fraction=(int16_t)u16(definition+10),spacing=(int16_t)u16(definition+12);if(spacing<=0)continue;
 for(unsigned j=0;j<count;j++)if(u32(r+8+j*16)&1){unsigned k=(j+1)%count,sa=u16(r+12+j*16),sb=u16(r+12+k*16);if(sa>=oc||sb>=oc)continue;const unsigned char*a=objects+sa*80,*b=objects+sb*80;if(u16(a)!=u16(r+14+j*16)||u16(b)!=u16(r+14+k*16))continue;if(n>=4096)return 0;int32_t*e=out+n*7;memcpy(e,a+8,8);memcpy(e+2,b+8,8);e[4]=(int32_t)u32(a+16)+(((int16_t)u16(a+34)+(int16_t)u16(a+36))*fraction>>8);e[5]=(int32_t)u32(b+16)+(((int16_t)u16(b+34)+(int16_t)u16(b+36))*fraction>>8);e[6]=spacing;n++;}

 }return n;
}
int main(int argc,char **argv){
 if(argc!=5&&argc!=7){fprintf(stderr,"usage: capture PID OUTPUT_PREFIX FRAME_COUNT INTERVAL_MS\n");return 2;}
 int pid=atoi(argv[1]),frames=atoi(argv[3]),interval=atoi(argv[4]);
 if(pid<1||frames<0||frames>100000000||interval<0||interval>10000)return 2;
 char path[512];snprintf(path,sizeof(path),"/proc/%d/mem",pid);fd=open(path,O_RDONLY|O_CLOEXEC);if(fd<0){perror("open");return 2;}
 if((word(0x400000)&0xffff)!=0x5a4d)return 3;
 unsigned char anchor[8];read_exact(0x46bf56,anchor,8);
 const unsigned char expected[8]={0x85,0xc0,0xa3,0x3c,0x38,0x67,0x00,0x75};
 if(memcmp(anchor,expected,8)){fprintf(stderr,"Build anchor mismatch\n");return 3;}
 unsigned char *actors=malloc(400*216),*objects=malloc(8192*80),*palettes=malloc(128*1028),*model_poses=malloc(512*36);
 if(!actors||!objects||!palettes||!model_poses)return 2;
 SurfaceCache surface={0};surface.cache=malloc(SURFACE_TILES*SURFACE_PAIR);SurfacePending*surface_pending=malloc(sizeof(SurfacePending));if(!surface.cache||!surface_pending)return 2;
 unsigned char*light_data=malloc(LIGHT_LIMIT);if(!light_data)return 2;
 unsigned char*particle_data=malloc(PARTICLE_MAX*PARTICLE_ROW);if(!particle_data)return 2;
 int socketfd=-1;
 if(!strcmp(argv[2],"@mythvr")){
  signal(SIGPIPE,SIG_IGN);socketfd=socket(AF_INET,SOCK_STREAM|SOCK_CLOEXEC,0);
  struct sockaddr_in addr={0};addr.sin_family=AF_INET;addr.sin_port=htons(38434);addr.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
  unsigned char token[32];FILE*tf=fopen("files/myth-bridge/bridge.token","rb");if(!tf||fread(token,1,32,tf)!=32){fprintf(stderr,"Missing IPC capability\n");return 5;}fclose(tf);
  if(socketfd<0||connect(socketfd,(struct sockaddr*)&addr,sizeof(addr))){perror("scene IPC");return 5;}
  if(write(socketfd,token,32)!=32)return 5;
  if(dup2(socketfd,STDOUT_FILENO)<0)return 5;close(socketfd);
 }
 uint32_t expected_landscape=argc==7?(uint32_t)strtoul(argv[5],NULL,10):0,expected_mesh=argc==7?(uint32_t)strtoul(argv[6],NULL,10):0;
 int accepted=0;
 for(int f=0;frames==0||f<frames;f++){
  if(expected_mesh&&(word(MVR_MESH)!=expected_mesh||word(expected_mesh)!=expected_landscape||(word(MVR_GAME_STATE)&65535)!=3))break;
  if(!strcmp(argv[2],"@mythvr"))receive_command();
  for(int attempt=0;attempt<20;attempt++){
   unsigned char ap[64],op[64],ap2[64],op2[64],selection[256],selection2[256];uint64_t start=ns();
   uint32_t tick=word(MVR_TICK),rng=word(MVR_RNG);
   read_exact(MVR_ACTOR_POOL,ap,64);read_exact(MVR_OBJECT_POOL,op,64);
   uint32_t ac=u16(ap+46),oc=u16(op+46);
   if(memcmp(ap,"monster",7)||memcmp(op,"object",6)||u16(ap+24)!=216||u16(op+24)!=80||ac>400||oc>8192)return 3;
   read_exact(u32(ap+52),actors,ac*216);read_exact(u32(op+52),objects,oc*80);
   read_exact(MVR_SELECTION,selection,256);
   uint32_t meta[5]={word(MVR_PLAYER),word(MVR_TEAM),word(0x553f20)>>16,u16(selection+2),u16(selection)};
   if(meta[4]>126)continue;
   unsigned char info[SELECTION_INFO_BYTES];collect_selection_info(actors,ac,selection,info);
   unsigned char eligibility[400];collect_selection_eligibility(actors,ac,eligibility);
   uint16_t mana_definitions[800];collect_mana_definitions(actors,ac,mana_definitions);
   uint32_t groups[10]={0};unsigned char groupData[2560];
   if(optional_read(0x554410,groupData,sizeof(groupData)))for(unsigned g=0;g<10;g++){unsigned n=u16(groupData+g*256);if(n<=126)groups[g]=n;}
   uint32_t palette_count=collect_palettes(actors,ac,objects,oc,palettes);
   uint32_t model_count=collect_models(model_poses);
   surface_collect(&surface,surface_pending,tick,ns());
   unsigned light_bytes=collect_lights(objects,oc,light_data);
   unsigned particle_count=collect_particles(particle_data);
   particle_count=collect_shockwaves(particle_data,particle_count);
   particle_count=collect_object_fx(objects,oc,particle_data,particle_count);
   particle_count=collect_lightning(particle_data,particle_count);
   for(unsigned i=0;i<particle_count;i++)append_palette(u32(particle_data+i*64+52),u32(particle_data+i*64+56),palettes,&palette_count);
   particle_count=collect_weather(particle_data,particle_count);
   int32_t edges[4096*7];unsigned edgeCount=connector_edges(edges,objects,oc);
   read_exact(MVR_ACTOR_POOL,ap2,64);read_exact(MVR_OBJECT_POOL,op2,64);
   read_exact(MVR_SELECTION,selection2,256);
   uint32_t endtick=word(MVR_TICK),endrng=word(MVR_RNG);uint64_t elapsed=ns()-start;
   if(expected_mesh&&(word(MVR_MESH)!=expected_mesh||word(expected_mesh)!=expected_landscape||(word(MVR_GAME_STATE)&65535)!=3))return 0;
   if(tick!=endtick||rng!=endrng||memcmp(ap,ap2,64)||memcmp(op,op2,64)||memcmp(selection,selection2,256))continue;
   /* tick bracket reduces tears but does not exclude an in-progress same tick. */
   uint32_t ack[4]={0};FILE*af=fopen(ackpath,"rb");if(af){if(fread(ack,1,16,af)!=16||ack[0]!=0x314b4341)memset(ack,0,sizeof(ack));fclose(af);}
   unsigned char formation[4808];uint32_t formation_bytes=0;
   FILE*fp=fopen("files/rootfs/home/xuser-1/.wine/drive_c/mythvr/formation.bin","rb");
   if(fp){if(fread(formation,1,sizeof(formation),fp)==sizeof(formation)&&u32(formation)==0x32565046&&u32(formation+4)==u32(formation+4804)&&u32(formation+8)<=126&&u32(formation+12)<=4)formation_bytes=sizeof(formation);fclose(fp);}
   uint32_t h[14]={21,tick,endtick,rng,ac,oc,216,80,(uint32_t)elapsed,(uint32_t)(elapsed>>32),u16(ap+48),u16(op+48),ack[1],ack[2]};
   char final[512];snprintf(final,sizeof(final),"%s-%04d.bin",argv[2],f);snprintf(path,sizeof(path),"%s-%04d.tmp",argv[2],f);
   int streaming=!strcmp(argv[2],"-")||!strcmp(argv[2],"@mythvr");
   FILE *out=streaming?stdout:fopen(path,"wb");if(!out){perror("output");return 2;}
   int ok=fwrite("MYTHBR01",1,8,out)==8 && fwrite(h,1,56,out)==56 && fwrite(actors,216,ac,out)==ac && fwrite(objects,80,oc,out)==oc && fwrite(meta,1,20,out)==20 && fwrite(selection+4,1,252,out)==252;
   ok=ok&&fwrite(&palette_count,4,1,out)==1&&fwrite(palettes,1028,palette_count,out)==palette_count;
   ok=ok&&fwrite(&model_count,4,1,out)==1;
   if(model_count!=0xffffffff)ok=ok&&fwrite(model_poses,36,model_count,out)==model_count;
   uint64_t surface_epoch=surface_pending->valid?surface_pending->epoch:0;uint32_t surface_count=surface_pending->valid?surface_pending->count:0;
   ok=ok&&fwrite(&surface_epoch,8,1,out)==1&&fwrite(&surface_count,4,1,out)==1&&fwrite(surface_pending->data,SURFACE_ROW,surface_count,out)==surface_count;
   ok=ok&&fwrite(&light_bytes,4,1,out)==1&&fwrite(light_data,1,light_bytes,out)==light_bytes;
   ok=ok&&fwrite(&particle_count,4,1,out)==1&&fwrite(particle_data,PARTICLE_ROW,particle_count,out)==particle_count;
   ok=ok&&fwrite(&formation_bytes,4,1,out)==1&&fwrite(formation,1,formation_bytes,out)==formation_bytes;
   ok=ok&&fwrite(eligibility,1,ac,out)==ac;
   ok=ok&&fwrite(info,1,SELECTION_INFO_BYTES,out)==SELECTION_INFO_BYTES;
   ok=ok&&fwrite(groups,4,10,out)==10;
   uint32_t camera_info[11]={0};
   FILE*cf=fopen("files/rootfs/home/xuser-1/.wine/drive_c/mythvr/camera.bin","rb");
   if(cf){if(fread(camera_info,4,9,cf)!=9||camera_info[0]!=0x314d4143||camera_info[1]!=camera_info[8]||camera_info[7]!=word(MVR_MESH)||camera_info[2]!=word(camera_info[7])||camera_info[3]>tick||tick-camera_info[3]>60)memset(camera_info,0,36);fclose(cf);}
   uint32_t sniff=word(0x561870);camera_info[9]=(sniff<<16)|(sniff>>16);camera_info[10]=word(0x544768)&65535;uint32_t progress=word(0x657c28);
   if((camera_info[10]&2)&&(int16_t)progress>0&&(int16_t)(progress>>16)<=(int16_t)progress)camera_info[10]|=0x10000;
   ok=ok&&fwrite(camera_info,4,11,out)==11;
   ok=ok&&fwrite(mana_definitions,4,ac,out)==ac;
   ok=ok&&fwrite(&edgeCount,4,1,out)==1&&fwrite(edges,28,edgeCount,out)==edgeCount;
   unsigned char hd_hues[128*88];unsigned hd_count=collect_hd_hues(palettes,palette_count,hd_hues);
   ok=ok&&fwrite(&hd_count,4,1,out)==1&&fwrite(hd_hues,88,hd_count,out)==hd_count;
   unsigned char roster[32*44];unsigned roster_count=collect_coop_roster(roster);
   ok=ok&&fwrite(&roster_count,4,1,out)==1&&fwrite(roster,44,roster_count,out)==roster_count;
   uint32_t game_speed=word(0x67ec0e)&65535;
   ok=ok&&fwrite(&game_speed,4,1,out)==1;
   if(!ok || (streaming ? fflush(out) : (fclose(out)||rename(path,final)))){perror("write");return 2;}
   surface_commit(&surface,surface_pending);
   if(surface_count)fprintf(stderr,"SURFACE tick=%u tiles=%u epoch=%llu reset=%d\n",tick,surface_count,(unsigned long long)surface_epoch,surface_pending->reset);
   fprintf(stderr,"{\"frame\":%d,\"tick\":%u,\"capture_us\":%llu,\"actor_slots\":%u,\"object_slots\":%u}\n",f,tick,(unsigned long long)(elapsed/1000),ac,oc);fflush(stderr);accepted++;break;
  }
  struct timespec delay={interval/1000,(interval%1000)*1000000L};nanosleep(&delay,NULL);
 }
 free(actors);free(objects);free(palettes);free(model_poses);free(surface.cache);free(surface_pending);free(light_data);free(particle_data);close(fd);return accepted==frames?0:4;
}
