// Verified live multiplayer mode is 1 (Quest co-op log, 2026-11-20).
// v24: 32-byte header, 32 RGBA team colours, 200 packed byte-pairs.
#define MAP_PEN_BYTES 560
static void collect_map_pen(unsigned char*out){
 memset(out,0,MAP_PEN_BYTES);
#ifdef MYTH_COOP_471
 unsigned char raw[400],verify[400],dims[8],steps[4],ph[12],th[12],players[32*104],teams[32*44];
 unsigned n=word(0x66b9ca)&65535;float scale;unsigned char sb[4];
 if(n>200||!optional_read(0x66b974,dims,8)||!optional_read(0x66bb5c,steps,4)||!optional_read(0x66b978,sb,4))return;
 memcpy(&scale,sb,4);float rx=u16(dims)/(u16(steps)*scale),ry=u16(dims+2)/(u16(steps+2)*scale);
 if(!(rx>0&&rx<=256&&ry>0&&ry<=256))return;
 if(n&&(!optional_read(0x66b9cc,raw,n*2)||!optional_read(0x66b9cc,verify,n*2)||memcmp(raw,verify,n*2)||(word(0x66b9ca)&65535)!=n))return;
 unsigned flags=0,team=word(0x5332e0),player=word(0x5332dc);
 if((word(0x6bee46)&65535)==1&&player<32&&team<32)flags|=1;
 if(word(0x66b990))flags|=2;
 memcpy(out,&flags,4);memcpy(out+4,&n,4);unsigned t=team<32?team+1:0;memcpy(out+8,&t,4);
 memcpy(out+16,&rx,4);memcpy(out+20,&ry,4);unsigned mesh=word(MVR_MESH);memcpy(out+24,&mesh,4);
 for(unsigned i=0;i<32;i++){out[32+i*4]=255;out[33+i*4]=64;out[34+i*4]=64;out[35+i*4]=255;}
 if(optional_read(0x533294,ph,12)&&optional_read(0x5332cc,th,12)){
 unsigned pc=u16(ph+2),tc=u16(th+2);
 if(pc&&pc<=32&&tc&&tc<=32&&optional_read(u32(ph+8),players,pc*104)&&optional_read(u32(th+8),teams,tc*44))
 for(unsigned i=0;i<tc;i++){unsigned captain=u16(teams+i*44+42);if(captain<pc){const unsigned char*p=players+captain*104;out[32+i*4]=p[89];out[33+i*4]=p[91];out[34+i*4]=p[93];}}
 }
 if(n)memcpy(out+160,raw,n*2);
#endif
}
