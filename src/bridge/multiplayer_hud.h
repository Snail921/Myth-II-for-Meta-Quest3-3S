// Read-only Build 471b HUD adapter. See multiplayer-hud.md for original entrypoints.
// Wire layout: flags/count/kind/reserved, title[192], time[64], notice[128],
// 64 rows of RGBA[4], name[64], score[64]. No engine calls or UI dirty-flag writes.
#define MULTIPLAYER_HUD_BYTES (400+64*132)
static unsigned chatOwnRow;
static void collect_multiplayer_hud(unsigned char*out){
 chatOwnRow=0;memset(out,0,MULTIPLAYER_HUD_BYTES);
#ifdef MYTH_COOP_471
 unsigned char state[80],ph[12],th[12],players[32*104],teams[32*44],rank[32*40],playerRank[32*36];
 if(!optional_read(0x6bee40,state,sizeof(state)))return;
 // Original central status, including Disconnected. Do not infer disconnection
 // from a silent socket, a paused film, or an ended simulation.
 int notice=(int16_t)(word(0x540a58)&65535);
 if(notice>=0&&notice<16){info_string((int16_t)word(0x541d00),notice,(char*)out+272,128);if(out[272])out[0]|=2;}
 if(!optional_read(0x533294,ph,12)||!optional_read(0x5332cc,th,12))return;
 unsigned pc=u16(ph+2),tc=u16(th+2);
 if(!pc||pc>32||!tc||tc>32||!optional_read(u32(ph+8),players,pc*104)||!optional_read(u32(th+8),teams,tc*44)||!optional_read(0x6bd99c,rank,tc*40))return;
 int havePlayerRank=optional_read(0x6bc7c0,playerRank,pc*36);
 unsigned humans=0;for(unsigned i=0;i<pc;i++)if(players[i*104+9]==0&&!(players[i*104+8]&4))humans++;
 int kind=(int16_t)u16(state+8),mode=(int16_t)u16(state+6);
 if(kind<0||kind>15||(mode!=1&&kind==13&&humans<2))return;
 out[0]|=1;memcpy(out+8,&kind,4);
 int table=(int16_t)word(0x6b23f2);
 if(kind==13){unsigned char index[2];uint32_t mesh=word(MVR_MESH);if(optional_read(mesh+0x28a,index,2))table=(int16_t)u16(index);}
 info_string(table,kind==13?0:kind,(char*)out+16,192);
 int phase=(int16_t)u16(state+0x34);int32_t ticks=0;int label=-1;
 if(phase==3)label=1;else if(phase==4)label=2;
 else if(phase==5){if(state[0x10]&0x20){if(state[0x14]&0x80)ticks=(int32_t)u32(state+0x28);else label=3;}else label=3;}
 else if(phase>=0&&phase<=2)ticks=(int32_t)u32(state+(phase==1?0x24:0x30));else label=0;
 if(label>=0)info_string((int16_t)word(0x6b23f4),label,(char*)out+208,64);
 else {if(ticks<0)ticks=0;unsigned sec=(unsigned)ticks/30;if(sec>=3600)snprintf((char*)out+208,64,"%u:%02u:%02u",sec/3600,(sec/60)%60,sec%60);else snprintf((char*)out+208,64,"%u:%02u",sec/60,sec%60);}
 int64_t total=0;for(unsigned i=0;i<tc;i++)total+=(int32_t)u32(rank+i*40+36);
 unsigned n=0,seen=0;
 for(unsigned i=0;i<tc&&n<64;i++){
  const unsigned char*r=rank+i*40;unsigned t=u16(r);if(t>=tc||(seen&(1u<<t)))continue;seen|=1u<<t;
  const unsigned char*team=teams+t*44;unsigned captain=u16(team+42);if(!team[38]||captain>=pc)continue;
  const unsigned char*p=players+captain*104;if(p[9]!=0||(p[8]&4))continue;
  if(t==word(0x5332e0))chatOwnRow=n+1;
  unsigned char*row=out+400+n++*132;row[0]=p[89];row[1]=p[91];row[2]=p[93];row[3]=160;
  memcpy(row+4,p+(team[38]>1?56:24),32);row[67]=0;
  char*score=(char*)row+68;int32_t value=(int32_t)u32(r+36);
  // The original 0x4a5119 switch: possession, numbered flags, time,
  // King of the Map (time + territories), and numeric scores.
  if(kind==0){if(word(0x6bee54)&0x200000){int percent=total?(int)((int64_t)value*100/total):0;if(value<0)percent=-percent;snprintf(score,64,"%d%%",percent);}else snprintf(score,64,"%d",word(0x532284)>0x6b8?(value+128)>>8:value/100);}
  else if(kind==1||kind==2||kind==5||kind==6){if(value)snprintf(score,64,"*");}
  else if(kind==3||kind==4){unsigned bits=u32(team+28);size_t at=0;for(unsigned f=0;f<9;f++)if(bits&(1u<<f))at+=(size_t)snprintf(score+at,64-at,"%s%u",at?" ":"",f+1);}
  else if(kind==9||kind==14||kind==15){if(value>0){unsigned sec=(unsigned)value/30;if(kind==15)snprintf(score,64,"%03u:%02u (x%u)",sec/60,sec%60,u16(team+34));else snprintf(score,64,"%02u:%02u",sec/60,sec%60);}}
  else if(value)snprintf(score,64,"%d",value);
  int64_t army=(int32_t)u32(r+8)+(int64_t)(int32_t)u32(r+16);
  int percent=army?(int)((int64_t)(int32_t)u32(r+16)*100/army):0;
  size_t used=strlen(score);snprintf(score+used,64-used,"%s%d%%",used?"  ":"",percent);
  if(team[38]>1)for(unsigned j=0;j<pc&&n<64;j++){
   const unsigned char*member=players+j*104;if(member[10]!=t||member[9]!=0||(member[8]&4))continue;
   unsigned char*q=out+400+n++*132;q[0]=member[97];q[1]=member[99];q[2]=member[101];q[3]=128;memcpy(q+4,member+24,32);
   if(havePlayerRank)for(unsigned k=0;k<pc;k++)if(u16(playerRank+k*36)==j){int pct=army?(int)((int64_t)(int32_t)u32(playerRank+k*36+16)*100/army):0;snprintf((char*)q+68,64,"%d%%",pct);break;}
  }
 }
 memcpy(out+4,&n,4);
 // Discard a torn roster or a level/state switch; optional HUD must never
 // make an otherwise valid scene snapshot fail.
 unsigned char check[12];if(!optional_read(0x533294,check,12)||memcmp(check,ph,12)||!optional_read(0x5332cc,check,12)||memcmp(check,th,12)||word(MVR_GAME_STATE)!=(u32(state)))memset(out,0,MULTIPLAYER_HUD_BYTES);
#endif
}
