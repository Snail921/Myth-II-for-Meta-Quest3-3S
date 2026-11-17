// Read-only roster. Runtime player records: flags +8, type +9, team +10,
// name +24 (32 bytes), primary RGB16 +88. Confirmed against recorded PC session.
static unsigned collect_coop_roster(unsigned char*out){
#ifdef MYTH_COOP_471
 unsigned char raw[32*104],head[12];
 if(!optional_read(0x533294,head,12))return 0;
 unsigned count=u16(head+2);uint32_t base=u32(head+8);
 int local=(int)word(0x5332dc),team=(int)word(0x5332e0);
 if(!count||count>32||local<0||(unsigned)local>=count||team<0||!optional_read(base,raw,count*104))return 0;
 unsigned n=0;
 for(unsigned i=0;i<count;i++){
  const unsigned char*p=raw+i*104;
  if((int)i==local||(int8_t)p[10]!=team||p[9]!=0||(p[8]&0x44))continue;
  unsigned char*q=out+n*44;memcpy(q,&i,4);memcpy(q+4,p,4);
  q[8]=p[89];q[9]=p[91];q[10]=p[93];q[11]=255;memcpy(q+12,p+24,32);q[43]=0;n++;
 }
 return n;
#else
 (void)out;return 0;
#endif
}
