/* Freestanding emulator harness, not injected into the live game. */
typedef unsigned int uint32_t;typedef unsigned short uint16_t;typedef unsigned int size_t;
#define UINT32_MAX 0xffffffffu
#define MVR_COLLECTION_RELATIVE 1
static void*memcpy(void*d,const void*s,size_t n){unsigned char*a=d;const unsigned char*b=s;for(size_t i=0;i<n;i++)a[i]=b[i];return d;}
static uint32_t u32(const void*p){uint32_t v;memcpy(&v,p,4);return v;}
static int optional_read(uint32_t a,void*p,size_t n){if(a<65536||n>4096)return 0;memcpy(p,(void*)a,n);return 1;}
#include "../src/bridge/collection_access.h"
__declspec(dllexport) uint32_t collection185_test(unsigned mode,unsigned col,unsigned index,unsigned char*out){
 unsigned char head[320];if(!collection_header(*(uint32_t*)0x533e20,col,head,320))return 0;
 if(mode==0){memcpy(out,head,320);return 1;}
 if(mode==1){uint32_t a=0;if(!collection_sequence(head,index,out,&a))return 0;return a;}
 if(mode==2)return collection_bitmap(head,index,out);
 if(mode==3)return collection_palette_offset();
 return 0;
}
