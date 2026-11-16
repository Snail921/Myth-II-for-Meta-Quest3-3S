#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
static unsigned char memory[0x700000];
static uint16_t u16(const void*p){uint16_t v;memcpy(&v,p,2);return v;}
static uint32_t u32(const void*p){uint32_t v;memcpy(&v,p,4);return v;}
static uint32_t word(uint32_t a){assert(a+4<sizeof(memory));return u32(memory+a);}
static int optional_read(uint32_t a,void*p,size_t n){if(a<0x10000||a+n>sizeof(memory)||n>4096)return 0;memcpy(p,memory+a,n);return 1;}
#include "../src/bridge/selection_info.h"
static void w16(unsigned char*p,uint16_t v){memcpy(p,&v,2);}
static void w32(unsigned char*p,uint32_t v){memcpy(p,&v,4);}
static void table(int id,const char*first,const char*second){
 unsigned at=0x20000+id*1024;unsigned char*d=memory+0x10000+id*16;w16(d,second?2:1);w32(d+12,at);w32(memory+at,at+16);strcpy((char*)memory+at+16,first);
 if(second){w32(memory+at+4,at+256);strcpy((char*)memory+at+256,second);}
}
int main(){
 w32(memory+0x528b14,0x10000);w32(memory+0x52a6f4,0x30000);w32(memory+0x528f9c,0x40000);
 table(0,"Warrior","Warriors");table(1,"Alric","Balor");table(2,"First biography","Second biography");
 unsigned char*d=memory+0x30000;w16(d+0x3e0,0);w16(d+0x3e2,1);w16(d+0x3e4,2);w16(d+0x18,8);w32(d+4,123);w16(memory+0x40000+0x174,7);
 unsigned char actors[432]={0},selection[256]={0},out[SELECTION_INFO_BYTES];w16(actors,100);w16(actors+216,101);actors[216+0x90]=1;w16(selection,1);
 read_selection_info(actors,2,selection,out);assert(!strcmp((char*)out+4,"Warrior"));assert(!strcmp((char*)out+260,"Alric"));assert(!strcmp((char*)out+772,"First biography"));assert(u32(out+1796)==100&&u32(out+1800)==7&&u32(out+1804)==8);
 w16(selection,2);w16(selection+6,1);read_selection_info(actors,2,selection,out);assert(!strcmp((char*)out+4,"2 Warriors"));assert(!strcmp((char*)out+260,"Alric, Balor"));assert(!out[772]);
 strcpy((char*)actors+0xc6,"Renamed");read_selection_info(actors,2,selection,out);assert(!strcmp((char*)out+260,"Renamed, Balor"));
 w16(selection,1);w16(selection+4,1);collect_selection_info(actors,2,selection,out);assert(!strcmp((char*)out+260,"Balor"));w16(selection+4,0);collect_selection_info(actors,2,selection,out);assert(!strcmp((char*)out+260,"Renamed"));
 // UI working buffer may contain ellipsis. Reconstruct from authoritative tables.
 strcpy((char*)memory+0x675820,"...");table(3,"Salvation",0);table(4,"Legendary","Normal");
 w16(memory+0x67ec08,13);w32(memory+0x529bd4,0x50000);w16(memory+0x503e4,3);
 w16(memory+0x528ad0+0x54,5);w32(memory+0x528ad0+0x48,0x51000);w32(memory+0x51010,0x696e646c);
 w16(selection,0);collect_selection_info(actors,2,selection,out);assert(!u32(out)&&!strcmp((char*)out+4,"Salvation  (Legendary)")&&!out[260]);
 w16(memory+0x67ec04,1);read_selection_info(actors,2,selection,out);assert(!strcmp((char*)out+4,"Salvation  (Normal)"));
 table(3,"Down a Broken Path",0);read_selection_info(actors,2,selection,out);assert(!strcmp((char*)out+4,"Down a Broken Path  (Normal)"));
 w16(memory+0x67ec08,0);w32(memory+0x67ebe4,0x52000);w16(memory+0x52012,3);read_selection_info(actors,2,selection,out);assert(!strcmp((char*)out+4,"Down a Broken Path  (Normal)"));
 w16(memory+0x67ec04,99);read_selection_info(actors,2,selection,out);assert(!strcmp((char*)out+4,"Down a Broken Path"));
 w16(memory+0x52012,65535);read_selection_info(actors,2,selection,out);assert(!out[4]);
 puts("PASS level title ignores UI ellipsis, difficulty/level changes, both name paths and missing data");
 char buf[8];assert(!info_string(-1,0,buf,8));assert(!info_string(1,99,buf,8));assert(info_string(2,0,buf,8)&&buf[7]==0);
 // Shared ammunition path: icon from current unit collection, definition sequence.
 w32(memory+0x52a9f4,0x60000);w32(memory+0x52ae6c,2);w16(memory+0x67ec06,1);
 w16(selection,1);w16(selection+4,0);actors[0x16]=2;actors[0x8c]=5;w16(d+0x24,14);memory[0x60000+42]=7;memory[0x60000+44]=255;
 read_selection_info(actors,2,selection,out);assert(u32(out+1812)==5&&u32(out+1816)==7&&u32(out+1820)==255&&u32(out+1824)==14);
 actors[0x8c]=0;read_selection_info(actors,2,selection,out);assert(u32(out+1812)==0);
 actors[0x8c]=1;actors[0x16]=3;read_selection_info(actors,2,selection,out);assert(u32(out+1812)==0);
 w16(memory+0x67ec06,0);read_selection_info(actors,2,selection,out);assert(u32(out+1812)==1);
 w16(selection,2);read_selection_info(actors,2,selection,out);assert(u32(out+1812)==0);
 puts("PASS ammunition icon/count, palette255, zero hides, enemy hides, replay mode and multiple selection");
 puts("PASS original-table names, plurals, biography, renamed actors, representative icon, selection cache invalidation and bounds");
}
