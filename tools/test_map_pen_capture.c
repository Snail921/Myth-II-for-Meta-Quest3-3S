#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#define MYTH_COOP_471 1
#define MVR_MESH 0x531618
static unsigned mode=1;
static uint32_t u32(const void*p){uint32_t v;memcpy(&v,p,4);return v;}
static uint16_t u16(const void*p){uint16_t v;memcpy(&v,p,2);return v;}
static uint32_t word(uint32_t a){switch(a){case 0x6bee46:return mode;case 0x5332dc:return 1;case 0x5332e0:return 0;case MVR_MESH:return 0x1000000;default:return 0;}}
static int optional_read(uint32_t a,void*out,size_t n){memset(out,0,n);if(a==0x66b974){uint16_t d[4]={149,128,0,0};memcpy(out,d,n);return 1;}if(a==0x66bb5c){uint16_t d[2]={1,1};memcpy(out,d,n);return 1;}if(a==0x66b978){float s=1;memcpy(out,&s,4);return 1;}return 0;}
#include "../src/bridge/map_pen_capture.h"
int main(){unsigned char b[MAP_PEN_BYTES];collect_map_pen(b);assert(u32(b)==1&&u32(b+8)==1);for(mode=0;mode<=2;mode+=2){collect_map_pen(b);assert(!(u32(b)&1));}puts("PASS actual live mode1 enables annotation controls; mode0/2 disable input");}
