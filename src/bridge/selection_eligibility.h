#pragma once
/* Build 434: 0x466c80 / 0x466c2d / 0x466c48, verified against original x86. */
static int original_selection_eligible(unsigned actor_byte9,unsigned definition_flags,int definition_kind,int active,int game_kind){
 if(!active)return 1;
 if((actor_byte9&0x10)||definition_kind==5){
  return (definition_flags&0x02000000)!=0||(game_kind==11&&(actor_byte9&8))||(game_kind==12&&(actor_byte9&0x20));
 }
 return (definition_flags&0x00080000)==0;
}
