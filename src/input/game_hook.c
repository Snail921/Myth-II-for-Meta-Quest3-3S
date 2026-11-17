#include "win_min.h"
int _fltused=0;
#include "sound_range.h"
#include "camera_bridge.h"
static I map_limit(I axis){B*m=*(B**)0x529bd4;if(!m)return 0;U blocks=*(US*)(m+8+axis*2);return blocks&&blocks<=16?(I)(blocks*32-1)*512:0;}
struct Command { U magic,seq,op,id,flags;I x,y;U reserved; };
struct Ack { U magic,seq,status,count; };
struct CW { I lparam;U wparam,message;H window; };
extern I can_control(I slot);extern void select_slot(I slot,I mode);extern void move_selected(I x,I y);
extern void route_selected(I count,const I*points);
static I routeXY[8];static US routeIDs[126];static U routeCount,routeUnits,routeTick,routeFacing;
extern void ground_selected(const void* options);
extern void voice_selected(I kind);
static I controlled_selection(void){US n=*(US*)0x553fd8;if(!n||n>126||(*(US*)0x553fda&1))return 0;for(U i=0;i<n;i++){I slot=((short*)0x553fdc)[i];if(slot<0||slot>=*(US*)0x529f4e||!can_control(slot))return 0;}return 1;}
static void cancel_mode(void){((void(*)(I))0x465f3c)(-1);}
static U lastSeq;
static US selectionStage[126];static U staged,expected,additive;static I staging;
extern I current_formation(void);
extern void movement_options(const void*,I,const I*);
static void move_with_facing(I count,const I*points,U facing){
 I form=(short)current_formation();US opts[4]={(US)((form<0?1:8)|16),(US)form,(US)facing,0};
 movement_options(opts,count,points);
}
// Use ordinary engine orders per actor, at its current position, without changing selection.
extern void move_actor(const US*,const void*,const I*);
static U orient_selected(const struct Command*c){
 if(c->id<0x10000||c->id>0x1ffff||c->flags||c->x||c->y||c->reserved)return 16;
 if(!controlled_selection())return 18;
 U n=*(US*)0x553fd8;US slots[126];I points[252];
 B*actors=*(B**)0x529f54;B*objects=*(B**)0x52a9f4;U count=*(US*)0x52a9ee;
 for(U i=0;i<n;i++){slots[i]=((US*)0x553fdc)[i];I object=*(short*)(actors+slots[i]*216+12);if(object<0||(U)object>=count)return 19;
  points[i*2]=*(I*)(objects+object*80+8);points[i*2+1]=*(I*)(objects+object*80+12);
  if(points[i*2]<0||points[i*2+1]<0||points[i*2]>=map_limit(0)||points[i*2+1]>=map_limit(1))return 19;
 }
 // One waypoint exactly at the actor position is discarded by the original engine.
 // Two identical points retain facing; no patrol bit, no displacement.
 US opts[4]={16,0,(US)c->id,0};routeCount=routeFacing=0;
 for(U i=0;i<n;i++){I path[4]={points[i*2],points[i*2+1],points[i*2],points[i*2+1]};move_actor(slots+i,opts,path);}
 cancel_mode();return 1;
}
extern void calculate_formation(const void*,I,const US*,I,const I*,I*,US*,I);
// Fixed bounded reply: header, selected IDs, waypoint-major XY slots, trailing sequence.
extern U terrain_kind(const I*);
struct FormationReply {U magic,seq,units,points;US ids[126];I xy[1008];B blocked[504];U endseq;};
static U preview_formation(const struct Command*c){
 if(!controlled_selection())return 18;
 if(c->x<0||c->y<0||c->x>=map_limit(0)||c->y>=map_limit(1))return 16;
 U n=*(US*)0x553fd8,prefix=(c->flags&1)?routeCount:0;
 if(c->id>1)return 16;if(c->id==1){if(prefix!=4)return 16;prefix--;}if(prefix>=4)return 16;
 B*actors=*(B**)0x529f54;US*slots=(US*)0x553fdc;
 if(prefix){if(n!=routeUnits||*(U*)0x67ec24<routeTick)return 19;for(U i=0;i<n;i++)if(routeIDs[i]!=*(US*)(actors+slots[i]*216))return 19;}
 static struct FormationReply reply;I points[8];US facings[126];
 I form=(short)current_formation();if(form < -1||form>9)return 16;
 US opts[4]={(US)((form<0?1:8)|((c->reserved&0x10000)?16:0)),(US)form,(US)c->reserved,0};
 for(U i=0;i<8;i++)points[i]=0;
 for(U i=0;i<prefix*2;i++)points[i]=routeXY[i];points[prefix*2]=c->x;points[prefix*2+1]=c->y;
 reply.magic=0x32565046;reply.seq=c->seq;reply.units=n;reply.points=prefix+1;
 for(U i=0;i<126;i++){reply.ids[i]=i<n?*(US*)(actors+slots[i]*216):0;facings[i]=0;}
 for(U i=0;i<1008;i++)reply.xy[i]=0;
 // Uses the same player formation tag and angle calculation as MOVEMENT.
 calculate_formation(opts,*(I*)0x52ae68,slots,n,points,reply.xy,facings,prefix+1);
 for(U i=0;i<504;i++)reply.blocked[i]=0;
 // Original preview 0x467eec uses selected slot i's terrain-prohibition mask.
 for(U j=0;j<=prefix;j++)for(U i=0;i<n;i++){U k=j*126+i;U kind=terrain_kind(reply.xy+k*2);reply.blocked[k]=(*(US*)(actors+slots[i]*216+0x1a)&(1u<<(kind&31)))!=0;}
 reply.endseq=c->seq;write_packet("C:\\mythvr\\formation.bin",&reply,sizeof(reply));return 1;
}
extern I can_pickup(I,I,I);
extern void pickup_selected(const void*);
// Query status 100/101 is intentionally distinct from mutation ACK success.
static I pickup_allowed(U key){
 U slot=key>>16,id=key&65535,count=*(US*)0x52a9ee;
 if(!id||count>8192||slot>=count||!controlled_selection())return 0;
 if(*(short*)0x553f22!=-1&&*(short*)0x553f22!=9)return 0;
 B*o=*(B**)0x52a9f4+slot*80;
 if(*(US*)o!=id||(*(U*)(o+4)&0x200)||!(o[40]==3||o[40]==4||(*(U*)(o+4)&0x100000)))return 0;
 US n=*(US*)0x553fd8;
 for(U i=0;i<n;i++)if(!can_pickup(((US*)0x553fdc)[i],slot,(short)id))return 0;
 return 1;
}
extern void register_group(I slot);extern void recall_group(I slot);extern I group_selectable(I slot);
static U group_command(const struct Command*c){
 if(c->id>9||c->flags>1||c->x||c->y||c->reserved||(c->op==19&&c->flags))return 16;
 US n=*(US*)0x553fd8;if(n>126)return 14;
 routeCount=routeFacing=0;staging=0;cancel_mode();
 if(c->op==19){if(n&&!controlled_selection())return 18;register_group(c->id);return 1;}
 US*g=(US*)(0x554410+c->id*256);if(g[0]>126)return 14;
 U ac=*(US*)0x529f4e;if(ac>400)return 12;
 for(U i=0;i<g[0];i++)if(g[2+i]>=ac)return 13;
 if(!c->flags)recall_group(c->id);
 else{B*actors=*(B**)0x529f54;
  for(U i=0;i<g[0];i++){U slot=g[2+i];if(!group_selectable(slot)||(actors[slot*216+7]&0x40))continue;
   I present=0;n=*(US*)0x553fd8;if(n>126)return 14;for(U j=0;j<n;j++)if(((US*)0x553fdc)[j]==slot)present=1;
   if(!present&&n<126)select_slot(slot,0);
  }
 }
 if(controlled_selection())voice_selected(5);return 1;
}
// Original tutorial SNIF consumes a 44-byte input snapshot, independently of camera movement.
extern void sniff_camera_input(void*);
static U tutorial_camera(const struct Command*c,U mesh){
 if(!c->id||c->id>1023||c->x||c->y||c->reserved)return 16;
 // Training Map landscape 00tm, 3x3 blocks; never notify another scenario.
 if(*(U*)mesh!=0x3030746d||*(US*)(mesh+8)!=3||*(US*)(mesh+10)!=3)return 1;
 if(*(U*)0x67ec10&0x32)return 1;
 US which=*(US*)0x561872;if(which<51||which>55)return 1;
 if(c->flags!=((U)*(US*)0x561870<<16|which))return 1;
 U expected=3u<<((which==51?0:which==52?1:which==53?2:which==54?3:4)*2);
 if(!(c->id&expected))return 1;
 U snapshot[11]={0};snapshot[2]=c->id;snapshot[6]=0xffffffff;
 sniff_camera_input(snapshot);return 1;
}
#include "coop_transfer.h"
static U execute(const struct Command*c){
 if(*(US*)0x400000!=0x5a4d||*(U*)0x46bf56!=0x3ca3c085)return 10;
 if(c->op==18){if(c->id<100||c->id>200||c->flags||c->x||c->y||c->reserved)return 16;return set_sound_range(c->id);}
 // Reject menus, invalid map dimensions, unloaded pools and invalid commands.
 if(*(US*)0x67ec00!=3)return 20;
 U mesh=*(U*)0x529bd4;
 if(!mesh||!map_limit(0)||!map_limit(1)||!*(U*)0x529f54||!*(U*)0x52a9f4)return 11;
 if((c->op==3||c->op==13||c->op==15)&&c->reserved&&(!(c->reserved&0x10000)||c->reserved>0x1ffff))return 16;
 if(c->op==16||c->op==17){
  if(c->flags||c->x||c->y||c->reserved)return 16;
  I allowed=pickup_allowed(c->id);
  if(c->op==16)return allowed?101:100;
  if(!allowed)return 18;
  US opts[4]={0,0,(US)(c->id>>16),(US)c->id};
  routeCount=0;pickup_selected(opts);voice_selected(3);cancel_mode();return 1;
 }
 if(c->op==24){if(c->id>1||c->flags||c->x||c->y||c->reserved)return 16;((void(*)(I,I))0x44c33a)((I)c->id,0);return 1;}
 if(c->op==23)return transfer_selected(c);
 if(c->op==22)return orient_selected(c);
 if(c->op==21)return tutorial_camera(c,mesh);
 if(c->op==19||c->op==20)return group_command(c);
 if(c->op==15)return preview_formation(c);
 if(c->op==12){
  if(c->id>65535||c->flags>1||c->x<0||c->x>=map_limit(0)||c->y<0||c->y>=map_limit(1)||(I)c->reserved < -32768||(I)c->reserved>65536)return 16;
  U hooks=install_camera_hooks();if(hooks!=1)return hooks;
  cameraHeartbeat=*(U*)0x67ec24;cameraHeartbeatMap=mesh;
  cameraInputHeld=c->flags;
  if((*(U*)0x67ec10&0x32)||script_camera_active()||cameraInputHeld)return 1; // Script camera owns presentation while OBMO is active.
  ((void(*)(float,float,float,float,float,float,U,I))0x44fda8)(c->x/512.f,c->y/512.f,(I)c->reserved/512.f,c->id*(6.283185307179586f/65536.f),0.f,0.f,15,0);
  return 1;
 }
 if(c->op==14){if(c->id>3||c->flags||c->x||c->y||c->reserved)return 16;if(c->id==3){routeCount=routeFacing=0;return 1;}}
 if(c->op==13||c->op==14){
  if(!controlled_selection()){routeCount=0;return 18;}US n=*(US*)0x553fd8;B*actors=*(B**)0x529f54;
  if(c->op==13&&(c->flags&1)){routeCount=0;routeFacing=0;routeUnits=n;for(U i=0;i<n;i++)routeIDs[i]=*(US*)(actors+((US*)0x553fdc)[i]*216);routeTick=*(U*)0x67ec24;}
  if(routeUnits!=n||*(U*)0x67ec24<routeTick){routeCount=0;return 19;}
  for(U i=0;i<n;i++)if(routeIDs[i]!=*(US*)(actors+((US*)0x553fdc)[i]*216)){routeCount=0;return 19;}
  if(c->op==13){if(c->id>1)return 16;if(c->id==1){if(routeCount!=4||c->flags)return 16;routeCount--;}if(routeCount>=4||c->x<0||c->y<0||c->x>=map_limit(0)||c->y>=map_limit(1)){routeCount=0;return 16;}routeXY[routeCount*2]=c->x;routeXY[routeCount*2+1]=c->y;routeCount++;routeFacing=c->reserved;return 1;}
  if(!routeCount)return 19;if(c->id){if(routeCount<2)return 16;I form=(short)current_formation();US opts[4]={(US)((form<0?0:8)|(c->id==1?1:3)),(US)(form<0?0:form),0,0};movement_options(opts,routeCount,routeXY);}else if(routeFacing)move_with_facing(routeCount,routeXY,routeFacing);else route_selected(routeCount,routeXY);routeCount=0;voice_selected(3);cancel_mode();return 1;
 }
 routeCount=0;
 if(c->op==9){if(c->id>126||c->flags>1)return 16;staged=0;expected=c->id;additive=c->flags;staging=1;return 1;}
 if(c->op==10){if(!staging)return 19;const US*ids=(const US*)&c->id;for(U i=0;i<10;i++){if(!ids[i])continue;if(staged>=expected){staging=0;return 19;}selectionStage[staged++]=ids[i];}return 1;}
 if(c->op==11){
  if(!staging||staged!=expected||c->id!=expected){staging=0;return 19;}staging=0;
  US slots[126];U count=*(US*)0x529f4e;if(count>400)return 12;B*actors=*(B**)0x529f54;
  for(U i=0;i<staged;i++){I found=-1;for(U j=0;j<count;j++)if(*(US*)(actors+j*216)==selectionStage[i]){found=j;break;}if(found<0||!can_control(found))return 18;slots[i]=(US)found;}
  cancel_mode();if(!additive)((void(*)(void))0x465107)();
  for(U i=0;i<staged;i++){I present=0;US n=*(US*)0x553fd8;if(n>126)return 14;for(U j=0;j<n;j++)if(((US*)0x553fdc)[j]==slots[i])present=1;if(!present)select_slot(slots[i],0);}
  if(controlled_selection())voice_selected(5);return 1;
 }
 if(c->op==2){cancel_mode();((void(*)(void))0x465107)();return 1;}
 if(c->op==1||c->op==4){
  U count=*(US*)0x529f4e;B*actors=*(B**)0x529f54;if(count>400)return 12;
  I slot=-1;for(U i=0;i<count;i++)if(*(US*)(actors+i*216)==c->id){slot=i;break;}if(slot<0||!c->id)return 13;
  if(c->op==4&&controlled_selection()&&(!can_control(slot)||*(short*)0x553f22==3)){
   U flags=*(short*)0x553f22==3?2:0;
   ((void(*)(void*,I,U,I,I))0x467c3b)((void*)0x553fd8,slot,flags,0,0);
   voice_selected(0);cancel_mode();return 1;
  }
  // Original selection routine enforces eligibility; do not write selection globals directly.
  cancel_mode();if(!(c->flags&1))((void(*)(void))0x465107)();select_slot(slot,0);
  US n=*(US*)0x553fd8;if(n>126)return 14;for(U i=0;i<n;i++)if(((short*)0x553fdc)[i]==slot){if(can_control(slot))voice_selected(5);return 1;}return 15;
 }
 if(c->op==3){
  US n=*(US*)0x553fd8;if(!n||n>126||c->x<0||c->y<0||c->x>=map_limit(0)||c->y>=map_limit(1))return 16;
  if(*(US*)0x553fda&1)return 18;
  for(U i=0;i<n;i++){I slot=((short*)0x553fdc)[i];if(slot<0||slot>=*(US*)0x529f4e||!can_control(slot))return 18;}
  if(c->reserved){I point[2]={c->x,c->y};move_with_facing(1,point,c->reserved);}else move_selected(c->x,c->y);voice_selected(3);cancel_mode();return 1;
 }
 if(c->op>=5&&c->op<=8){
  if(!controlled_selection())return 18;
  if(c->op==5){
   if(c->x<0||c->y<0||c->x>=map_limit(0)||c->y>=map_limit(1)||(I)c->reserved < -32768||(I)c->reserved>32767)return 16;
   struct {US flags,pad;I x,y,z;} opts={*(short*)0x553f22==3?1:0,0,c->x,c->y,(I)c->reserved};
   ground_selected(&opts);voice_selected(0);cancel_mode();return 1;
  }
  if(c->op==6){if(c->id>5||c->id==3)return 16;((void(*)(I))0x466ec4)((I)c->id);return 1;}
  if(c->op==7){((void(*)(I))0x465f3c)(3);return 1;}
  if(c->op==8){if(c->id>9)return 16;((void(*)(I,I))0x468ae7)((I)c->id,1);return 1;}
 }
 return 17;
}
__declspec(dllexport) I WIN MythInputHook(I code,U wp,I lp){
 if(code>=0){struct CW*c=(struct CW*)lp;if(c->message==0x8439&&c->wparam==0x4d595448){struct Command command;if(read_packet("C:\\mythvr\\command.bin",&command,sizeof(command))&&command.magic==0x31444d43&&command.seq!=lastSeq){lastSeq=command.seq;struct Ack previous;if(read_packet("C:\\mythvr\\ack.bin",&previous,sizeof(previous))&&previous.magic==0x314b4341&&previous.seq==command.seq)return 0;U status=execute(&command);struct Ack ack={0x314b4341,command.seq,status,*(US*)0x553fd8};write_packet("C:\\mythvr\\ack.bin",&ack,sizeof(ack));}return 0;}}
 return CallNextHookEx(0,code,wp,lp);
}
I WIN DllMain(H module,U reason,H reserved){(void)module;(void)reason;(void)reserved;return 1;}
