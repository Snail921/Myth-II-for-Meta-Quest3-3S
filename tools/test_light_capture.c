#define main capture_entry
#include "../src/bridge/capture.c"
#undef main
#include <assert.h>
static void put(uint32_t a,const void*p,size_t n){assert(pwrite(fd,p,n,a)==(ssize_t)n);}
static void put32(uint32_t a,uint32_t v){put(a,&v,4);}
static void put16(uint32_t a,uint16_t v){put(a,&v,2);}
int main(int argc,char**argv){
 char name[]="/data/local/tmp/myth-light-test-XXXXXX";fd=mkstemp(name);assert(fd>=0);unlink(name);unsigned char object[80]={0},z[512]={0};
 uint32_t col=0x10000,refs=0x11000,seqref=0x12000,seq=0x13000,maps=0x14000,bitmap=0x15000,pixels=0x16000;
 put(col,z,320);put(refs,z,128);put(seqref,z,128);put(seq,z,128);put(maps,z,32);put(bitmap,z,52);
 put32(0x52b4d4,col);put32(col+96,1);put32(col+108,refs);put32(col+128,1);put32(col+140,seqref);put32(col+144,1);put32(col+156,maps);
 put32(seqref+72,seq);put16(seq+8,1);put16(seq+10,1);put16(seq+64,0);put32(maps,1);put32(maps+4,1024);put32(maps+8,2048);put16(maps+12,0);
 put32(refs+72,bitmap);put16(bitmap,2);put16(bitmap+2,2);put16(bitmap+4,3);put32(bitmap+48,pixels);unsigned char raw[6]={50,100,99,150,200,99};put(pixels,raw,6);
 object[0]=1;int32_t xyz[3]={10000,20000,512};memcpy(object+8,xyz,12);object[53]=1;
 put(0x529638,z,64);put(0x529678,z,64);
 unsigned char*out=malloc(LIGHT_LIMIT);unsigned bytes=collect_lights(object,1,out);assert(bytes==72&&u32(out)==1&&u32(out+4+4)==1);assert(light_s32(out+4+24)==-1&&light_s32(out+4+28)==-2);assert(out[68]==50&&out[69]==100&&out[70]==150&&out[71]==200);
 put32(maps,0);assert(collect_lights(object,1,out)==4);put32(maps,1);
 uint32_t pp=0x20000,ep=0x21000,defs=0x22000;put(pp,z,88);put(ep,z,88);put(defs,z,128);put32(0x5297cc,defs);
 put16(0x529638+24,88);put16(0x529638+46,1);put32(0x529638+52,pp);put16(0x529678+24,88);put16(0x529678+46,1);put32(0x529678+52,ep);
 put16(ep,2);put16(ep+10,0);put16(ep+80,1);put16(ep+86,65535);put16(pp,3);put16(pp+6,65535);put(pp+44,xyz,12);unsigned char flag=8;put(pp+77,&flag,1);
 bytes=collect_lights(object,1,out);assert(u32(out)==2&&bytes==140&&u32(out+72+4)==2);assert(u32(out+72+40)==65534);
 unsigned char*fx=malloc(PARTICLE_MAX*PARTICLE_ROW);put16(ep+82,65535);put16(ep+84,65535);put16(pp+70,256);
 assert(collect_particles(fx)==1);assert(u32(fx+28)==65534&&u32(fx+32)==256&&u32(fx+36)==256);assert(light_s32(fx+16)==xyz[0]);
 put32(0x528f9c,0x30000);put16(0x30176,7);assert(collect_particles(fx)==1&&u32(fx+56)==7);
 put32(defs,0x8000000);put16(ep+52,9);assert(collect_particles(fx)==1&&u32(fx+56)==9);put32(defs,0);
 put16(ep+82,512);assert(collect_particles(fx)==1&&u32(fx+32)==512);
 put32(defs,0x2000000);assert(collect_particles(fx)==0); // invalid media grid is rejected
 put16(0x67ec62,64);put16(0x67ec64,64);put32(0x67ec6c,0x60000);
 put16(0x60000+((xyz[1]>>9)*64+(xyz[0]>>9))*12+8,(uint16_t)-2560);
 assert(collect_particles(fx)==1&&light_s32(fx+24)==-2560);
 assert(light_s32(fx+16)==xyz[0]&&light_s32(fx+20)==xyz[1]);
 unsigned char grid[14]={0};uint32_t pos[3]={0xffffffff,0xffffffff,100};
 grid[0]=64;grid[2]=64;uint32_t cells=0x60000;memcpy(grid+10,&cells,4);put16(cells+8,123);
 assert(particle_media_height(grid,pos)&&pos[2]==123);
 pos[0]=pos[1]=999999;put16(cells+(64*64-1)*12+8,(uint16_t)-512);
 assert(particle_media_height(grid,pos)&&(int32_t)pos[2]==-512);
 grid[0]=0;assert(!particle_media_height(grid,pos));put32(defs,0);
 put16(pp+6,0);assert(collect_particles(fx)==1);put16(pp+6,65535);put(0x529cd8,z,64);put16(0x529cd8+24,32);put16(0x529cd8+46,2);put32(0x529cd8+52,0x50000);put(0x50000,z,64);
 put16(0x50000,1);put16(0x50020,2);put16(0x50000+26,65535);put16(0x50020+26,32768);put32(0x50000+4,1000);put32(0x50020+4,2000);put32(pp+32,100);put16(ep+80,0);
 assert(collect_particles(fx)==2);assert(u32(fx+16)==1100&&u32(fx+64+16)==2100);assert(u32(fx+32)==256&&u32(fx+28)==65534&&u32(fx+64+28)==32767);bytes=collect_lights(object,1,out);assert(u32(out)==3&&u32(out+72+4)==3&&u32(out+72+12)==1100&&u32(out+140+12)==2100);put16(0x529cd8+46,0);put16(ep+80,1);
 object[2]=0;object[3]=1;assert(collect_object_fx(object,1,fx,0)==1&&u32(fx+60)==1&&u32(fx+32)==256);
 put32(0x561860,0x40000);put(0x40000,z,20);put16(0x40002,3);put32(col+192,1);put32(col+196,0x41000);put32(0x41000,0x42000);unsigned char palette[1024]={0};uint32_t rgb=0x00203040;memcpy(palette+4,&rgb,4);put(0x42000+148,palette,1024);unsigned pc=0;unsigned char palettes[1028];append_palette(0,0,palettes,&pc);assert(pc==1&&palettes[8]==0x20&&palettes[9]==0x30&&palettes[10]==0x40);append_palette(0,0,palettes,&pc);assert(pc==1);free(fx);
 flag=0;put(pp+77,&flag,1);assert(collect_lights(object,1,out)==72);
 if(argc==2){FILE*f=fopen(argv[1],"rb");assert(f);unsigned n;assert(fread(&n,4,1,f)==1);for(unsigned i=0;i<n;i++){uint32_t v[8];assert(fread(v,32,1,f)==1);unsigned char particle[88]={0};memcpy(particle+80,v+1,4);memcpy(particle+84,v+2,4);particle[73]=v[3];particle[74]=v[4];particle[75]=v[5];assert(light_fade(v[0],particle,v[6])==v[7]);}fclose(f);printf("PASS %u original particle fade cases\n",n);}
 free(out);close(fd);puts("PASS runtime object/particle light masks, registration, padded rows, eligibility, emitter alpha");
}
