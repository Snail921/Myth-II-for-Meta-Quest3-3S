/* Executes generated production collector headers against emulated read-only game pages. */
typedef signed char int8_t;typedef unsigned int uint32_t;typedef int int32_t;typedef unsigned short uint16_t;typedef short int16_t;typedef unsigned long long uint64_t;typedef unsigned int size_t;typedef int ssize_t;
#define UINT32_MAX 0xffffffffu
#define NULL ((void*)0)
#define offsetof(T,F) __builtin_offsetof(T,F)
#define MVR_COLLECTION_RELATIVE 1
static int fd;
static void*memcpy(void*d,const void*s,size_t n){unsigned char*a=d;const unsigned char*b=s;for(size_t i=0;i<n;i++)a[i]=b[i];return d;}
void*memset(void*d,int v,size_t n){unsigned char*a=d;for(size_t i=0;i<n;i++)a[i]=(unsigned char)v;return d;}
static int memcmp(const void*a,const void*b,size_t n){const unsigned char*x=a,*y=b;for(size_t i=0;i<n;i++)if(x[i]!=y[i])return x[i]-y[i];return 0;}
static uint32_t u32(const void*p){uint32_t v;memcpy(&v,p,4);return v;}
static uint16_t u16(const void*p){uint16_t v;memcpy(&v,p,2);return v;}
static uint32_t word(uint32_t a){return u32((void*)a);}
static int optional_read(uint32_t a,void*p,size_t n){if(a<65536||n>4096)return 0;memcpy(p,(void*)a,n);return 1;}
static ssize_t pread(int f,void*p,size_t n,uint32_t a){(void)f;memcpy(p,(void*)a,n);return n;}
#include "collection_access.h"
#include "surface_capture.h"
#include "light_capture.h"
#include "particle_capture.h"
#include "shockwave_capture.h"
#include "weather_capture.h"
static SurfaceCache surface;
static SurfacePending pending;
static unsigned char cache[SURFACE_TILES*SURFACE_PAIR];
__declspec(dllexport) unsigned collector185_test(unsigned mode,unsigned char*out){
 unsigned count=u16((void*)(0x532e3c+42));const unsigned char*objects=(void*)word(0x532e3c+48);
 if(mode==0)return collect_lights(objects,count,out);
 if(mode==1){unsigned n=collect_particles(out);n=collect_shockwaves(out,n);n=collect_object_fx(objects,count,out,n);return collect_weather(out,n);}
 if(mode==2){surface.cache=cache;surface_collect(&surface,&pending,word(0x6bee64),1);if(!pending.valid)return 0;memcpy(out,pending.data,pending.count*SURFACE_ROW);return pending.count;}
 return 0;
}


