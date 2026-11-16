#ifndef MYTH_SOUND_RANGE_H
#define MYTH_SOUND_RANGE_H
static U sound_percent=100;
static I sound_installed;
extern I original_sound_distance(const I*,const I*);
extern void sound_distance_hook(void);
I scaled_sound_distance(const I*a,const I*b){
 if(sound_percent==100)return original_sound_distance(a,b);
 I delta[2],zero[2]={0,0};
 for(U i=0;i<2;i++){
  double d=((double)a[i]-(double)b[i])*100.0/(double)sound_percent;
  if(d>=32767.0||d<=-32767.0)return 32767;
  delta[i]=(I)d;
 }
 return original_sound_distance(delta,zero);
}
static U set_sound_range(U percent){
 B*call=(B*)0x4cc603;
 if(!sound_installed){
  // Only this Build 434 audio call site; never patch the shared distance helper.
  if(call[0]!=0xe8||*(U*)(call+1)!=(U)(0x46bfa2-0x4cc608)||*(U*)0x4cc608!=0x010e43f6)return 21;
  if(percent==100){sound_percent=100;return 1;}
  U old,ignored;if(!VirtualProtect(call,5,0x40,&old))return 22;
  *(U*)(call+1)=(U)sound_distance_hook-0x4cc608;
  FlushInstructionCache((H)-1,call,5);
  VirtualProtect(call,5,old,&ignored);sound_installed=1;
 }
 sound_percent=percent;return 1;
}
#endif
