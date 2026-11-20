#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#define MYTH_COOP_471 1
#define MVR_MESH 0x531618
#define MVR_GAME_STATE 0x6bee40
static unsigned char mem[0x800000];
static uint32_t u32(const void*p){uint32_t x;memcpy(&x,p,4);return x;}
static uint16_t u16(const void*p){uint16_t x;memcpy(&x,p,2);return x;}
static uint32_t word(uint32_t a){assert(a+4<=sizeof(mem));return u32(mem+a);}
static int optional_read(uint32_t a,void*p,size_t n){if(a<65536||a+n>sizeof(mem))return 0;memcpy(p,mem+a,n);return 1;}
static void w(uint32_t a,uint32_t x){memcpy(mem+a,&x,4);}
static void sh(uint32_t a,uint16_t x){memcpy(mem+a,&x,2);}
static int info_string(int table,int index,char*out,size_t n){snprintf(out,n,table==1?"Rule %d":table==2?"State %d":"Disconnected",index);return 1;}
#include "../src/bridge/multiplayer_hud.h"
int main(){
 unsigned char out[MULTIPLAYER_HUD_BYTES];
 w(MVR_GAME_STATE,3);sh(0x6bee46,1);sh(0x6bee48,9);sh(0x6bee74,2);w(0x6bee70,5400);sh(0x540a58,65535);sh(0x6b23f2,1);sh(0x6b23f4,2);w(0x532284,1850);
 sh(0x533296,2);w(0x53329c,0x10000);sh(0x5332ce,2);w(0x5332d4,0x11000);
 for(unsigned i=0;i<2;i++){unsigned p=0x10000+i*104,t=0x11000+i*44;mem[p+10]=i;snprintf((char*)mem+p+24,32,"Player %u",i);mem[p+89]=100+i;mem[p+91]=20;mem[p+93]=30;mem[t+38]=1;sh(t+42,i);sh(0x6bd99c+i*40,i);w(0x6bd99c+i*40+36,(i+1)*1800);}
 collect_multiplayer_hud(out);assert(out[0]==1&&u32(out+4)==2);assert(!strcmp((char*)out+16,"Rule 9"));assert(!strcmp((char*)out+208,"3:00"));assert(!strcmp((char*)out+468,"01:00  0%"));assert(!strcmp((char*)out+404,"Player 0"));
 // Original sorted order is kept, not rebuilt by VR.
 sh(0x6bd99c,1);sh(0x6bd99c+40,0);collect_multiplayer_hud(out);assert(!strcmp((char*)out+404,"Player 1"));
 for(unsigned k=0;k<16;k++){sh(0x6bee48,k);collect_multiplayer_hud(out);assert(out[0]==1&&u32(out+4)==2);for(unsigned i=0;i<64;i++)assert(!out[400+i*132+67]&&!out[400+i*132+131]);}
 sh(0x6bee48,0);w(0x6bd99c+36,512);collect_multiplayer_hud(out);assert(!strcmp((char*)out+468,"2  0%"));
 sh(0x6bee48,3);w(0x11000+44+28,0x105);collect_multiplayer_hud(out);assert(!strcmp((char*)out+468,"1 3 9  0%"));
 sh(0x540a58,1);sh(0x541d00,3);collect_multiplayer_hud(out);assert(out[0]==3&&!strcmp((char*)out+272,"Disconnected"));
 sh(0x533296,33);collect_multiplayer_hud(out);assert(out[0]==2&&u32(out+4)==0);sh(0x533296,2);
 sh(0x540a58,65535);mem[0x10000+104+9]=1;sh(0x6bee48,13);sh(0x6bee46,2);collect_multiplayer_hud(out);assert(!out[0]);
 puts("PASS HUD: rule/time, sorted rows, 16 score branches, fixed-point Body Count, flags, original disconnect status, corrupt roster, single-player exclusion");
}
