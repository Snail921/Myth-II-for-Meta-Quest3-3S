#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
static uint32_t u32(const void*p){uint32_t n;memcpy(&n,p,4);return n;}
static uint16_t u16(const void*p){uint16_t n;memcpy(&n,p,2);return n;}
static int16_t s16(const void*p){int16_t n;memcpy(&n,p,2);return n;}
static int32_t s32(const void*p){int32_t n;memcpy(&n,p,4);return n;}
int main(void){
 unsigned char h[64];unsigned frame=0;
 while(1){
  size_t got=fread(h,1,64,stdin);if(!got && feof(stdin))break;
  if(got!=64||memcmp(h,"MYTHBR01",8)||(u32(h+8)<1||u32(h+8)>18)||u32(h+12)!=u32(h+16)||u32(h+32)!=216||u32(h+36)!=80)return 2;
  uint32_t ac=u32(h+24),oc=u32(h+28);if(ac>400||oc>8192)return 2;
  size_t n=ac*216+oc*80;unsigned char*b=malloc(n);if(!b||fread(b,1,n,stdin)!=n)return 2;
  if(u32(h+8)>=2){unsigned char meta[272];if(fread(meta,1,272,stdin)!=272||u32(meta+16)>126)return 2;}
  if(u32(h+8)>=3){uint32_t n;if(fread(&n,4,1,stdin)!=1||n>128)return 2;unsigned char row[1028];for(unsigned i=0;i<n;i++)if(fread(row,1,1028,stdin)!=1028)return 2;}
  if(u32(h+8)>=4){uint32_t n;if(fread(&n,4,1,stdin)!=1)return 2;if(n!=0xffffffff){if(n>512)return 2;unsigned char row[36];for(unsigned i=0;i<n;i++)if(fread(row,1,36,stdin)!=36)return 2;}}
  if(u32(h+8)>=5){uint64_t epoch,seen=0;uint32_t n,id;if(fread(&epoch,8,1,stdin)!=1||fread(&n,4,1,stdin)!=1||n>4||(!epoch&&n))return 2;unsigned char row[131072];for(unsigned i=0;i<n;i++){if(fread(&id,4,1,stdin)!=1||id>=42||(seen&(1ULL<<id))||fread(row,1,sizeof(row),stdin)!=sizeof(row))return 2;seen|=1ULL<<id;}}
  if(u32(h+8)>=6){uint32_t len;if(fread(&len,4,1,stdin)!=1||len<4||len>262144)return 2;unsigned char*l=malloc(len);if(!l||fread(l,1,len,stdin)!=len)return 2;uint32_t count=u32(l);size_t pos=4;if(count>256)return 2;for(unsigned i=0;i<count;i++){if(len-pos<64)return 2;uint32_t w=u32(l+pos+32),ht=u32(l+pos+36);if(!w||!ht||w>256||ht>256)return 2;pos+=64+(size_t)w*ht;if(pos>len)return 2;}if(pos!=len)return 2;free(l);}
  if(u32(h+8)>=7){uint32_t count;if(fread(&count,4,1,stdin)!=1||count>2048)return 2;unsigned char row[64];for(unsigned i=0;i<count;i++)if(fread(row,1,64,stdin)!=64)return 2;}
  if(u32(h+8)>=9){uint32_t len;if(fread(&len,4,1,stdin)!=1||(len&&len!=(u32(h+8)>=10?4808:4304)))return 2;unsigned char reply[4808];if(len&&(fread(reply,1,len,stdin)!=len||u32(reply)!=(u32(h+8)>=10?0x32565046:0x31565046)||u32(reply+4)!=u32(reply+len-4)||u32(reply+8)>126||u32(reply+12)>4))return 2;}
  if(u32(h+8)>=11){unsigned char eligibility[400];unsigned ac=u32(h+24);if(fread(eligibility,1,ac,stdin)!=ac)return 2;for(unsigned i=0;i<ac;i++)if(eligibility[i]>1)return 2;}
  if(u32(h+8)>=12){unsigned char info[1812];if(fread(info,1,sizeof(info),stdin)!=sizeof(info)||u32(info)>126||info[259]||info[771]||info[1795])return 2;}
  if(u32(h+8)>=13){unsigned char inv[16];if(fread(inv,1,16,stdin)!=16||u32(inv)>127||(u32(inv)&&(u32(inv+4)>254||u32(inv+8)>255||u32(inv+12)>255)))return 2;}
  if(u32(h+8)>=14){uint32_t groups[10];if(fread(groups,4,10,stdin)!=10)return 2;for(unsigned i=0;i<10;i++)if(groups[i]>126)return 2;}
  if(u32(h+8)>=15){uint32_t camera[11];if(fread(camera,4,11,stdin)!=11)return 2;}
  if(u32(h+8)>=16){uint32_t mana[400];if(fread(mana,4,ac,stdin)!=ac)return 2;}
  if(u32(h+8)>=17){uint32_t n;int32_t e[7];unsigned stride=u32(h+8)>=18?7:4;if(fread(&n,4,1,stdin)!=1||n>4096)return 2;for(unsigned i=0;i<n;i++)if(fread(e,4,stride,stdin)!=stride)return 2;}
  unsigned live=0;for(unsigned i=0;i<ac;i++){
   const unsigned char*a=b+i*216;if(!u16(a))continue;live++;
   int obj=s16(a+12);if(obj<0||(unsigned)obj>=oc)return 3;
   const unsigned char*o=b+ac*216+obj*80;if(!u16(o))return 3;
   if(u16(a)==10359)printf("{\"frame\":%u,\"consumer_uid\":%u,\"tick\":%u,\"unit_id\":%u,\"position_raw\":[%d,%d,%d],\"position_game\":[%.6f,%.6f,%.6f],\"yaw_raw\":%u,\"collection\":%u,\"sequence\":%u,\"animation_frame\":%u,\"frame_tick\":%u}\n",frame,(unsigned)getuid(),u32(h+12),u16(a),s32(o+8),s32(o+12),s32(o+16),s32(o+8)/512.0,s32(o+12)/512.0,s32(o+16)/512.0,u16(o+28),o[42],o[43],o[46],o[47]);
  }
  if(live!=u32(h+48))return 3;
  fflush(stdout);free(b);frame++;
 }
 fprintf(stderr,"validated_frames=%u\n",frame);return frame?0:4;
}
