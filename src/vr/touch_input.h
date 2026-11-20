#include "orbit_camera.h"
#include "follow_camera.h"
#include "tutorial_camera.h"
#include "script_camera_gate.h"
static ScriptCameraGate scriptCameraGate;static bool scriptCameraBlocked=false;
static void restartSceneFade();
// Included after scene/IPC/geometry declarations. Logical actions remain separate from bindings.
#include "interaction_math.h"
#include "radial_math.h"
#include "selection_math.h"
#include "facing_math.h"
#include <chrono>
#include <deque>
#include "camera_settings.h"
#include "controller_tap.h"
#include "group_cycle.h"
struct TouchInput {
 bool physical=false;
 int routedKey(int i)const{bool flip=(i<2?cameraSpeeds.swapTriggers:i<4?cameraSpeeds.swapAX:cameraSpeeds.swapYB);return flip?(i^1):i;}
 XrAction routed(XrAction a)const{
  if(physical)return a;
  auto pair=[&](XrAction l,XrAction r,bool flip){return flip?(a==l?r:a==r?l:a):a;};
  if(a==select||a==extend)return pair(select,extend,cameraSpeeds.swapTriggers);
  if(a==special||a==groundAttack)return pair(special,groundAttack,cameraSpeeds.swapGrips);
  if(a==orders||a==formations)return pair(orders,formations,cameraSpeeds.swapAX);
  if(a==clear||a==nextGroup)return pair(clear,nextGroup,cameraSpeeds.swapYB);
  if(a==aim||a==leftAim)return pair(aim,leftAim,cameraSpeeds.swapHands);
  if(a==grip||a==leftGrip)return pair(grip,leftGrip,cameraSpeeds.swapHands);
  if(a==move||a==elevateTurn)return pair(move,elevateTurn,cameraSpeeds.swapSticks);
  if(a==followClick||a==warpClick)return pair(followClick,warpClick,cameraSpeeds.swapSticks);
  return a;
 }
 XrInstance xrInstance{};XrSpaceLocationFlags aimFlags[2]={0,0};
 void logAimSource(XrSession session,bool left){
  XrInteractionProfileState profile{XR_TYPE_INTERACTION_PROFILE_STATE};
  if(XR_SUCCEEDED(xrGetCurrentInteractionProfile(session,path(xrInstance,left?"/user/hand/left":"/user/hand/right"),&profile))&&profile.interactionProfile){char text[256]={};uint32_t count=0;xrPathToString(xrInstance,profile.interactionProfile,sizeof(text),&count,text);LOG("ChatAimSource hand=%d profile=%s flags=%llu",left?1:0,text,(unsigned long long)aimFlags[left?1:0]);}
 }
 XrSession hapticSession{};XrAction leftHaptic{},rightHaptic{};bool hapticFocused=false;ControllerTap tap;GroupCycle groupCycle;
 std::map<unsigned,int> hapticRequests;std::map<unsigned,GroupCycle::Request> groupRequests;
 void pulse(bool right){if(!hapticFocused)return;XrHapticActionInfo info{XR_TYPE_HAPTIC_ACTION_INFO};info.action=(right^cameraSpeeds.swapHands)?rightHaptic:leftHaptic;XrHapticVibration vibration{XR_TYPE_HAPTIC_VIBRATION};vibration.duration=25000000;vibration.frequency=XR_FREQUENCY_UNSPECIFIED;vibration.amplitude=.25f;xrApplyHapticFeedback(hapticSession,&info,(const XrHapticBaseHeader*)&vibration);}
 XrActionSet set{};XrAction select{},clear{},nextGroup{},extend{},move{},elevateTurn{},aim{},grip{},leftGrip{},special{},groundAttack{},orders{},formations{},minimap{},followClick{},warpClick{};
 bool lastNextGroup=true;bool lastFollowClick=true,lastWarpClick=true;
 XrSpace leftGripSpace{};bool lastMinimap=true;bool groundAttackHeld=false,specialHeld=false,lastSpecial=false,lastOrders=false,lastFormations=false,lastExtend=false;bool swallowRight=false,swallowLeft=false;
 XrAction leftAim{};XrSpace leftAimSpace{};XrSpace viewSpace{},aimSpace{},gripSpace{};bool lastSelect=false,lastClear=false,selectHeld=false,extendHeld=false;
 XrPath path(XrInstance i,const char*s){XrPath p;ck(xrStringToPath(i,s,&p));return p;}
 void init(XrInstance instance,XrSession session){
  xrInstance=instance;
  XrReferenceSpaceCreateInfo viewInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};viewInfo.referenceSpaceType=XR_REFERENCE_SPACE_TYPE_VIEW;viewInfo.poseInReferenceSpace.orientation.w=1;ck(xrCreateReferenceSpace(session,&viewInfo,&viewSpace));
  XrActionSetCreateInfo si{XR_TYPE_ACTION_SET_CREATE_INFO};strcpy(si.actionSetName,"myth_controls");strcpy(si.localizedActionSetName,"Myth controls");ck(xrCreateActionSet(instance,&si,&set));
  auto create=[&](const char*name,XrActionType type){XrActionCreateInfo c{XR_TYPE_ACTION_CREATE_INFO};strcpy(c.actionName,name);strcpy(c.localizedActionName,name);c.actionType=type;XrAction a;ck(xrCreateAction(set,&c,&a));return a;};
  select=create("select",XR_ACTION_TYPE_FLOAT_INPUT);clear=create("clear_selection",XR_ACTION_TYPE_BOOLEAN_INPUT);extend=create("extend_selection",XR_ACTION_TYPE_FLOAT_INPUT);move=create("camera_move",XR_ACTION_TYPE_VECTOR2F_INPUT);elevateTurn=create("camera_height_turn",XR_ACTION_TYPE_VECTOR2F_INPUT);aim=create("right_pointer",XR_ACTION_TYPE_POSE_INPUT);grip=create("right_grip",XR_ACTION_TYPE_POSE_INPUT);
  leftAim=create("left_keyboard_pointer",XR_ACTION_TYPE_POSE_INPUT);
  leftGrip=create("left_grip_pose",XR_ACTION_TYPE_POSE_INPUT);special=create("special_ability",XR_ACTION_TYPE_FLOAT_INPUT);groundAttack=create("ground_attack",XR_ACTION_TYPE_FLOAT_INPUT);orders=create("orders_menu",XR_ACTION_TYPE_BOOLEAN_INPUT);formations=create("formation_menu",XR_ACTION_TYPE_BOOLEAN_INPUT);
  nextGroup=create("next_group",XR_ACTION_TYPE_BOOLEAN_INPUT);
  followClick=create("follow_selected",XR_ACTION_TYPE_BOOLEAN_INPUT);warpClick=create("warp_selected",XR_ACTION_TYPE_BOOLEAN_INPUT);
  hapticSession=session;leftHaptic=create("left_feedback",XR_ACTION_TYPE_VIBRATION_OUTPUT);rightHaptic=create("right_feedback",XR_ACTION_TYPE_VIBRATION_OUTPUT);
  minimap=create("wrist_minimap",XR_ACTION_TYPE_BOOLEAN_INPUT);
  XrActionSuggestedBinding bindings[]={
   {leftAim,path(instance,"/user/hand/left/input/aim/pose")},
   {leftHaptic,path(instance,"/user/hand/left/output/haptic")},{rightHaptic,path(instance,"/user/hand/right/output/haptic")},
   {followClick,path(instance,"/user/hand/right/input/thumbstick/click")},{warpClick,path(instance,"/user/hand/left/input/thumbstick/click")},
   {minimap,path(instance,"/user/hand/left/input/menu/click")},
   {select,path(instance,"/user/hand/left/input/trigger/value")},{clear,path(instance,"/user/hand/left/input/y/click")},
   {nextGroup,path(instance,"/user/hand/right/input/b/click")},
   {extend,path(instance,"/user/hand/right/input/trigger/value")},
   {move,path(instance,"/user/hand/left/input/thumbstick")},{elevateTurn,path(instance,"/user/hand/right/input/thumbstick")},{aim,path(instance,"/user/hand/right/input/aim/pose")},{grip,path(instance,"/user/hand/right/input/grip/pose")},
   {leftGrip,path(instance,"/user/hand/left/input/grip/pose")},{special,path(instance,"/user/hand/left/input/squeeze/value")},{groundAttack,path(instance,"/user/hand/right/input/squeeze/value")},{orders,path(instance,"/user/hand/right/input/a/click")},{formations,path(instance,"/user/hand/left/input/x/click")}};
  XrInteractionProfileSuggestedBinding profile{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};profile.interactionProfile=path(instance,"/interaction_profiles/oculus/touch_controller");profile.countSuggestedBindings=sizeof(bindings)/sizeof(bindings[0]);profile.suggestedBindings=bindings;ck(xrSuggestInteractionProfileBindings(instance,&profile));
  XrSessionActionSetsAttachInfo attach{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};attach.countActionSets=1;attach.actionSets=&set;ck(xrAttachSessionActionSets(session,&attach));
  XrActionSpaceCreateInfo sp{XR_TYPE_ACTION_SPACE_CREATE_INFO};sp.poseInActionSpace.orientation.w=1;sp.action=aim;ck(xrCreateActionSpace(session,&sp,&aimSpace));sp.action=grip;ck(xrCreateActionSpace(session,&sp,&gripSpace));
  sp.action=leftGrip;ck(xrCreateActionSpace(session,&sp,&leftGripSpace));sp.action=leftAim;ck(xrCreateActionSpace(session,&sp,&leftAimSpace));
 }
 bool boolean(XrSession s,XrAction a){XrActionStateGetInfo gi{XR_TYPE_ACTION_STATE_GET_INFO};gi.action=routed(a);XrActionStateBoolean v{XR_TYPE_ACTION_STATE_BOOLEAN};ck(xrGetActionStateBoolean(s,&gi,&v));return v.isActive&&v.currentState;}
 bool trigger(XrSession s,XrAction a,bool&held){XrActionStateGetInfo gi{XR_TYPE_ACTION_STATE_GET_INFO};gi.action=routed(a);XrActionStateFloat v{XR_TYPE_ACTION_STATE_FLOAT};ck(xrGetActionStateFloat(s,&gi,&v));held=v.isActive&&v.currentState>=(held?.40f:.55f);return held;}
 bool pose(XrSession s,XrAction a,XrSpace hand,XrSpace base,XrTime time,XrPosef&out){a=routed(a);if(a==aim)hand=aimSpace;else if(a==leftAim)hand=leftAimSpace;else if(a==grip)hand=gripSpace;else if(a==leftGrip)hand=leftGripSpace;if(benchmarkPose.enabled){out=benchmarkPose.hand(a==leftGrip||a==leftAim);return true;}XrActionStateGetInfo gi{XR_TYPE_ACTION_STATE_GET_INFO};gi.action=a;XrActionStatePose state{XR_TYPE_ACTION_STATE_POSE};ck(xrGetActionStatePose(s,&gi,&state));if(!state.isActive)return false;XrSpaceLocation loc{XR_TYPE_SPACE_LOCATION};ck(xrLocateSpace(hand,base,time,&loc));if(a==aim||a==leftAim)aimFlags[a==leftAim?1:0]=loc.locationFlags;if((loc.locationFlags&3)!=3)return false;out=loc.pose;return true;}
};
static TouchInput input;static Terrain terrain;
static int infoPanelButton=0,originalDialogMode=0;
static double infoButtonFlash[2]={-1,-1};
static bool originalDialogHovered=false;
static void pollOriginalDialog();
static void originalDialogRequest(int,int=0,int=0);
static bool hitOriginalDialog(Vec3,Vec3,Vec3&,float&,float&);
static bool infoPanelShown=true,infoPanelHovered=false,infoPanelGesture=false;
static bool wristMapShown=true,wristMapPoseValid=false;static XrPosef wristMapPose{};static Vec3 wristMapViewer{};static XrPosef multiplayerHead{};static Vec3 wristMapForward{0,0,-1};
static RadialMenu radial;
static bool mapPenEnabled=false,mapPenStroke=false,mapPenNeedsEnd=false;static Vec3 mapPenForward{0,0,-1};static double mapPenLastPoint=0,mapPenChangedAt=0;static int mapPenX=-1,mapPenY=-1;static unsigned mapPenMesh=0;
static bool mapHovered=false,mapGesture=false,mapWarpRejected=false;
static double mapPressTime=0;
static Vec3 mapPointerHit{};static float mapPointerU=0,mapPointerV=0;
static bool hitWristMap(Vec3 origin,Vec3 direction,Vec3&hit,float&u,float&v,bool tracking=false);
static bool hitSelectionPanel(Vec3 origin,Vec3 direction,Vec3&hit);
static bool warpFromWristMap(float u,float v,const XrPosef&head);

static bool pressFacingCandidate=false;static unsigned pressFacingSelection=0;
static SelectionView selectionView,selectionDragView;static float selectionDragDepth=2;static SelectionGesture selectionGesture;
static std::vector<unsigned> selectionPreview;
static float movePressDiameter=1;static bool movePressValid=false;static Vec3 movePressPoint{};
static unsigned patrolRequest=0;static double patrolFlashUntil=0;static Vec3 patrolFlashPoint{};static float patrolFlashDiameter=1;
static Vec3 waypointHitPoint{};static int pressWaypoint=-1;
static unsigned routePoints=0;static bool pressGroundAttack=false;
struct MoveMarkerUnit { Vec3 offset; float diameter; unsigned id; };
static std::vector<MoveMarkerUnit> moveMarkerUnits;
static std::vector<Vec3> moveMarkerPoints;
static uint64_t moveMarkerRevision=0;
static unsigned moveFacing=0,previewFacing=0;static double lastFacingPreview=0;
static uint32_t acceptedFormation=0;
static uint32_t formationRequest=0;static unsigned formationMarkerUnits=0,formationMarkerPoints=0;
static std::vector<Vec3> formationMarkers;
static std::vector<unsigned char> formationBlocked;
static void clearMoveMarkers(){formationRequest=0;formationMarkerUnits=formationMarkerPoints=0;formationMarkers.clear();formationBlocked.clear();if(!moveMarkerPoints.empty()||!moveMarkerUnits.empty())++moveMarkerRevision;moveMarkerPoints.clear();moveMarkerUnits.clear();}
static unsigned pressUnit=0;static bool pressSelectedImmediately=false,pressGroundValid=false;static Vec3 pressGroundHit{};
static unsigned lastTapID=0;static double lastTapTime=0;
static OrbitCamera orbitCamera;static FollowCamera followCamera;
struct OrbitTarget{unsigned id,key;Vec3 position;bool alive=true;};
static std::vector<OrbitTarget> orbitTargets;
static Vec3 benchmarkCamera{};static float benchmarkYaw=0,benchmarkHeight=0;
static Vec3 camera{0,1.6f,4};static float cameraYaw=0,cameraGroundOffset=1.6f;static bool cameraReady=false;
static bool pointerValid=false,groundValid=false;
static Vec3 pointerStart{},pointerEnd{},groundHit{},lastMove{};static unsigned hoverID=0;static bool hoverFriendly=false;static double lastMoveTime=0;static float lastMoveDiameter=1.f;static bool lastMoveGroundAttack=false;
static unsigned hoverItem=0,pressItem=0,pickupKey=0,pickupSelection=0,pickupRequest=0;
static bool pickupEligible=false;static double pickupQueryTime=0;
static unsigned selectionSignature(){unsigned h=2166136261u;for(const auto&o:objects)if(o.unitID&&o.selected){h=(h^o.unitID)*16777619u;}return h;}
static bool pickupGreen(){return hoverItem&&hoverItem==pickupKey&&pickupEligible&&pickupSelection==selectionSignature();}
static bool pickupCandidate(const Object&o){return !o.unitID&&objectVisible(o)&&(o.objectType==3||o.objectType==4||(o.rawFlags&0x100000));}
static uint32_t requestSeq=0,requestPending=0,pendingOp=0;static double requestTime=0;static bool commandFailed=false;
static std::deque<std::array<uint32_t,8>> commandQueue;
static double lastCameraSync=0;
static char soundRangePath[1024]{};static int soundRangeApplied=0,soundRangeSent=100;static double soundRangePoll=0;
static double seconds(){using namespace std::chrono;return duration<double>(steady_clock::now().time_since_epoch()).count();}
static Vec3 rotated(XrQuaternionf q,Vec3 v){Vec3 u{q.x,q.y,q.z};Vec3 cross{u.y*v.z-u.z*v.y,u.z*v.x-u.x*v.z,u.x*v.y-u.y*v.x};Vec3 cross2{u.y*cross.z-u.z*cross.y,u.z*cross.x-u.x*cross.z,u.x*cross.y-u.y*cross.x};return add(v,add(times(cross,2*q.w),times(cross2,2)));}
static XrPosef worldPose(XrPosef p){Vec3 pos=add(camera,turn({p.position.x,p.position.y,p.position.z},cameraYaw));p.position={pos.x,pos.y,pos.z};float s=sinf(cameraYaw/2),c=cosf(cameraYaw/2);auto q=p.orientation;p.orientation={c*q.x+s*q.z,c*q.y+s*q.w,c*q.z-s*q.x,c*q.w-s*q.y};return p;}
static bool flushCommand(){
 if(requestPending||commandQueue.empty())return true;
 if(peer<0||!authenticated){commandQueue.clear();input.hapticRequests.clear();input.groupRequests.clear();commandFailed=true;return false;}
 auto data=commandQueue.front();commandQueue.pop_front();
 // A single bounded write on the already authenticated duplex stream; no retry on partial transmission.
 if(send(peer,data.data(),sizeof(data),MSG_NOSIGNAL|MSG_DONTWAIT)!=(ssize_t)sizeof(data)){commandFailed=true;commandQueue.clear();close(peer);peer=-1;return false;}
 requestPending=data[1];pendingOp=data[2];requestTime=seconds();commandFailed=false;if(data[2]>=26)LOG("Chat request seq=%u op=%u",data[1],data[2]);else LOG("Input request seq=%u op=%u id=%u flags=%u x=%d y=%d",data[1],data[2],data[3],data[4],(int)data[5],(int)data[6]);return true;
}
static bool command(unsigned op,unsigned id=0,unsigned flags=0,int x=0,int y=0,int z=0,bool cycling=false){
 if(peer<0||!authenticated||commandQueue.size()>=32){commandFailed=true;return false;}
 if(op==1||op==4){for(const auto&o:objects)if(o.unitID==id)LOG("Actor target op=%u tick=%u id=%u key=%08x family=%d col=%u team=%d owner=%d pos=%.3f,%.3f,%.3f objectFlags=%08x actorFlags=%08x",op,tick,id,o.objectKey,o.family,o.col,o.team,o.owner,o.x,o.y,o.z,o.rawFlags,o.actorFlags);}
 commandQueue.push_back({0x31444d43,++requestSeq,op,id,flags,(uint32_t)x,(uint32_t)y,(uint32_t)z});
 if(op==20){input.groupRequests[requestSeq]=input.groupCycle.request(id,cycling);if(cameraSpeeds.selectionHaptics)input.hapticRequests[requestSeq]=0;}
 bool pointerSelection=op==4;
 if(pointerSelection){bool controlled=std::any_of(objects.begin(),objects.end(),[](const Object&o){return o.selected&&o.controllable;});auto target=std::find_if(objects.begin(),objects.end(),[id](const Object&o){return o.unitID==id;});if(controlled&&(originalMode==3||target==objects.end()||!target->controllable))pointerSelection=false;}
 if(op==1||op==2||pointerSelection||op==23)input.groupCycle.interrupt();
 if((op==1||op==4)&&cameraSpeeds.selectionHaptics){for(auto&o:objects)if(o.unitID==id&&o.controllable){bool changed=!o.selected;for(auto&other:objects)if(!flags&&other.selected&&other.unitID!=id)changed=true;if(changed)input.hapticRequests[requestSeq]=0;break;}}
 return flushCommand();
}
#include "chat_input.h"
static void endMapPenStroke(){if(mapPenStroke){LOG("Map pen stroke end x=%d y=%d",mapPenX,mapPenY);if(!command(25,3))mapPenNeedsEnd=true;mapPenStroke=false;mapPenX=mapPenY=-1;}}
static void mapPenButton(int button){
 endMapPenStroke();if(!mapPen.available(seconds()))return;
 if(button==3){bool next=!mapPenEnabled;if(command(25,next?1:0)){mapPenEnabled=next;mapPenChangedAt=seconds();if(next)mapPenForward=cameraSpeeds.mapRotation?turn(wristMapForward,cameraYaw):Vec3{0,0,-1};}}
 else if(button==4)command(25,4);
}
static void mapPenPoint(float u,float v){
 if(!mapPenEnabled||!mapPen.available(seconds())||mapPen.rangeX()<=0||mapPen.rangeY()<=0)return;
 int x=std::clamp(int(std::lround((1-u)*mapPen.rangeX())),1,255),y=std::clamp(int(std::lround(v*mapPen.rangeY())),0,255);
 // Heartbeats also flush a stationary batch; the engine drops duplicate points.
 if((mapPenStroke&&seconds()-mapPenLastPoint<.05)||commandQueue.size()>3)return;
 if(command(25,2,0,x,y)){if(!mapPenStroke)LOG("Map pen stroke begin uv=%.3f,%.3f point=%d,%d",u,v,x,y);mapPenStroke=true;mapPenX=x;mapPenY=y;mapPenLastPoint=seconds();}
}
static void acknowledge(uint32_t seq,uint32_t status){if(requestPending&&seq==requestPending){LOG("Input ACK seq=%u status=%u",seq,status);auto feedback=input.hapticRequests.find(seq);if(feedback!=input.hapticRequests.end()){if(status==1)input.pulse(feedback->second==1);input.hapticRequests.erase(feedback);}auto group=input.groupRequests.find(seq);if(group!=input.groupRequests.end()){input.groupCycle.acknowledge(group->second,status==1);input.groupRequests.erase(group);}if(pendingOp==18){soundRangeApplied=status==1?soundRangeSent:0;requestPending=0;flushCommand();return;}if(pendingOp==16){if(seq==pickupRequest){pickupEligible=status==101;LOG("Pickup query key=%08x status=%u selection=%08x",pickupKey,status,pickupSelection);}requestPending=0;flushCommand();return;}if(seq==patrolRequest){patrolFlashUntil=status==1?seconds()+.5:0;patrolRequest=0;}commandFailed=status!=1;requestPending=0;if(commandFailed){commandQueue.clear();input.hapticRequests.clear();input.groupRequests.clear();routePoints=0;clearMoveMarkers();}else flushCommand();}}
static bool selectGroup(const std::vector<unsigned>&ids,bool additive){
 size_t needed=2+(ids.size()+9)/10;
 if(ids.size()>126||peer<0||!authenticated||commandQueue.size()+needed>32){commandFailed=true;return false;}
 input.groupCycle.interrupt();
 // Build the whole transaction before sending its BEGIN. Failure never commits a partial set.
 commandQueue.push_back({0x31444d43,++requestSeq,9,(uint32_t)ids.size(),additive?1u:0u,0,0,0});
 for(size_t offset=0;offset<ids.size();offset+=10){std::array<uint32_t,8> p{0x31444d43,++requestSeq,10,0,0,0,0,0};for(size_t j=0;j<10&&offset+j<ids.size();j++)p[3+j/2]|=(ids[offset+j]&65535)<<((j%2)*16);commandQueue.push_back(p);}
 commandQueue.push_back({0x31444d43,++requestSeq,11,(uint32_t)ids.size(),0,0,0,0});
 bool changed=false;for(auto&o:objects)if(o.unitID){bool wanted=std::find(ids.begin(),ids.end(),o.unitID)!=ids.end()||(additive&&o.selected);if(wanted!=o.selected)changed=true;}
 if(changed&&!ids.empty()&&cameraSpeeds.selectionHaptics)input.hapticRequests[requestSeq]=0;return flushCommand();
}
static Vec3 unitCenter(const Object&o){return {mythWorldX(o.x,originX,scale),(o.z-originZ)*scale+.65f,(o.y-originY)*scale};}
static bool visibleUnit(const Object&o,ScreenPoint&screen){
 if(!objectVisible(o)||!o.unitID||quads.find(key(o.col,o.seq,o.frame,0))==quads.end())return false;
 auto p=unitCenter(o);if(!selectionView.project(p,screen)||!selectionView.contains(screen))return false;
 auto d=sub(p,selectionView.origin);float distance=sqrtf(dot(d,d));Vec3 hit{};float groundDistance=0;
 return !terrain.ray(selectionView.origin,normalized(d),hit,groundDistance)||groundDistance>=distance-.35f;
}
static float selectedUnitDiameter(){
 float widths=0;unsigned count=0;
 for(const auto&o:objects){if(!objectVisible(o)||!o.selected||!o.unitID)continue;auto it=quads.find(key(o.col,o.seq,o.frame,0));if(it==quads.end())continue;auto bounds=fixedUnitFrame(o,it->second);float width=bounds.right-bounds.left;if(width>0){widths+=width;count++;}}
 return count?widths/count:1.f;
}
// Snapshot the selected group once, so markers never follow moving units.
// Unit sizes/IDs are retained; positions come from the original formation calculator.
static void captureMoveMarkerUnits(){
 ++moveMarkerRevision;formationRequest=0;formationMarkerUnits=formationMarkerPoints=0;formationMarkers.clear();formationBlocked.clear();moveMarkerUnits.clear();Vec3 center{};
 for(const auto&o:objects){if(!objectVisible(o)||!o.selected||!o.unitID||!o.controllable)continue;
  float diameter=1;auto it=quads.find(key(o.col,o.seq,o.frame,0));
  if(it!=quads.end()){auto b=fixedUnitFrame(o,it->second);diameter=std::max(.1f,b.right-b.left);}
  Vec3 p{mythWorldX(o.x,originX,scale),0,(o.y-originY)*scale};center=add(center,p);moveMarkerUnits.push_back({p,diameter,o.unitID});
 }
 if(!moveMarkerUnits.empty()){center=times(center,1.f/moveMarkerUnits.size());for(auto&u:moveMarkerUnits)u.offset=sub(u.offset,center);}
}
static void acceptFormation(const unsigned char*p,size_t size){
 if(size!=4808||!formationRequest||u32(p)!=0x32565046||u32(p+4)!=formationRequest||u32(p+4804)!=formationRequest)return;
 unsigned n=u32(p+8),count=u32(p+12);if(n!=moveMarkerUnits.size()||!n||n>126||!count||count>4)return;
 for(const auto&u:moveMarkerUnits){bool found=false;for(unsigned i=0;i<n;i++)found|=u16(p+16+i*2)==u.id;if(!found)return;}
 std::vector<Vec3> next;next.reserve(n*count);std::vector<unsigned char> blocked;blocked.reserve(n*count);
 for(unsigned j=0;j<count;j++)for(unsigned i=0;i<n;i++){int x=i32(p+268+(j*126+i)*8),y=i32(p+272+(j*126+i)*8);if(x<0||y<0||x>=(int(terrain.width)-1)*512||y>=(int(terrain.height)-1)*512)return;if(p[4300+j*126+i]>1)return;blocked.push_back(p[4300+j*126+i]);next.push_back({mythWorldX(x/512.f,originX,scale),0,(y/512.f-originY)*scale});}
 if(acceptedFormation==formationRequest)return; // one immutable reply per request
 acceptedFormation=formationRequest;formationMarkers.swap(next);formationBlocked.swap(blocked);formationMarkerUnits=n;formationMarkerPoints=count;++moveMarkerRevision;
 LOG("Formation preview seq=%u units=%u points=%u blocked=%zu",formationRequest,n,count,std::count(formationBlocked.begin(),formationBlocked.end(),1));
}
static void requestFormation(Vec3 target,bool append,unsigned facing=0,bool keep=false){

 if(!keep){formationMarkerUnits=formationMarkerPoints=0;formationMarkers.clear();formationBlocked.clear();++moveMarkerRevision;}
 formationRequest=0;acceptedFormation=0;previewFacing=facing;lastFacingPreview=seconds();
 int x=lroundf(mythGameX(target.x,originX,scale)*512),y=lroundf((target.z/scale+originY)*512);
 if(command(15,(append&&routePoints==4)?1:0,(append&&routePoints)?1:0,x,y,facing))formationRequest=requestSeq;
}
// Hit only the first/last staged point; retain the press target during trigger motion.
static int routeWaypointHit(){
 if(hoverID||hoverItem||routePoints<2||moveMarkerPoints.size()<routePoints)return -1;
 Vec3 direction=normalized(sub(pointerEnd,pointerStart));float best=1e9f;int hit=-1;
 for(unsigned j=0;j<routePoints;j++){if(j&&j+1!=routePoints)continue;
  auto test=[&](Vec3 center,float radius){center.y=terrain.elevation(center.x,center.z)+.04f;Vec3 d=sub(center,pointerStart);float t=dot(d,direction);if(t<0)return;Vec3 delta=sub(d,times(direction,t));if(dot(delta,delta)<=radius*radius&&t<best){best=t;hit=j;waypointHitPoint=center;}};
  if(formationMarkerPoints==routePoints&&formationMarkerUnits==moveMarkerUnits.size())for(unsigned i=0;i<formationMarkerUnits;i++)test(formationMarkers[j*formationMarkerUnits+i],std::max(.08f,moveMarkerUnits[i].diameter*.3f));
  test(moveMarkerPoints[j],std::max(.08f,lastMoveDiameter*.3f));
 }
 return hit;
}
static void clickPointer(bool extend,bool groundAttack,bool fixedMove,Vec3 destination,unsigned hoverID,unsigned hoverItem,bool groundValid,Vec3 groundHit){
 if(hoverItem&&!fixedMove&&!groundAttack)return;
 if(hoverID&&!fixedMove&&!groundAttack){
  routePoints=0;clearMoveMarkers();
  auto it=std::find_if(objects.begin(),objects.end(),[hoverID](const Object&o){return o.unitID==hoverID;});
  bool own=it!=objects.end()&&it->controllable;
  if(own&&originalMode!=3&&lastTapID==hoverID&&seconds()-lastTapTime<=.4){
   std::vector<unsigned> ids;for(const auto&o:objects){ScreenPoint p;if(o.controllable&&o.family==it->family&&visibleUnit(o,p))ids.push_back(o.unitID);}
   selectGroup(ids,extend);lastTapID=0;
  }else{command(4,hoverID,extend?1:0);lastTapID=own?hoverID:0;lastTapTime=seconds();}
  return;
 }
 lastTapID=0;
 if(fixedMove||groundValid){auto target=fixedMove?destination:groundHit;bool selected=false;for(auto&o:objects)selected|=o.selected;
  if(selected){if(!groundAttack&&routePoints>=4){if(!extend){auto p=moveMarkerPoints.back();int x=lroundf(mythGameX(p.x,originX,scale)*512),y=lroundf((p.z/scale+originY)*512);if(command(13,1,0,x,y,moveFacing)&&command(14)){routePoints=0;lastMoveTime=moveFacing?0:seconds();++moveMarkerRevision;}}return;}int gx=lroundf(mythGameX(target.x,originX,scale)*512),gy=lroundf((target.z/scale+originY)*512),gz=lroundf((target.y/scale+originZ)*512);
   bool staged=!groundAttack&&(extend||routePoints);
   if(command(groundAttack?5:(staged?13:3),0,(!groundAttack&&extend&&!routePoints)?1:0,gx,gy,groundAttack?gz:(fixedMove?moveFacing:0))){if(!groundAttack){if(!routePoints)moveMarkerPoints.clear();if(moveMarkerUnits.empty())captureMoveMarkerUnits();moveMarkerPoints.push_back(target);++moveMarkerRevision;}else clearMoveMarkers();if(staged){routePoints++;if(!extend){if(command(14))routePoints=0;}}else routePoints=0;lastMove=target;lastMoveGroundAttack=groundAttack;lastMoveTime=(!groundAttack&&fixedMove&&moveFacing&&!routePoints)?0:seconds();lastMoveDiameter=fixedMove?movePressDiameter:selectedUnitDiameter();}
  }else {routePoints=0;clearMoveMarkers();command(2);}
 }else {routePoints=0;clearMoveMarkers();command(2);}
}
#include "coop_transfer_input.h"
static void updateInput(XrSession session,XrSpace space,XrTime time,const XrView*views,float dt,bool focused){
 if(!cameraReady){followCamera.active=false;orbitCamera.active=false;if(levelInitialCamera){camera.x=mythWorldX(levelCamera[0],originX,scale);camera.z=(levelCamera[1]-originY)*scale;cameraYaw=levelCamera[3]+1.5707963268f;cameraGroundOffset=std::clamp((levelCamera[2]-originZ)*scale-terrain.elevation(camera.x,camera.z),1.6f,20.f);}camera.y=terrain.elevation(camera.x,camera.z)+cameraGroundOffset;cameraReady=true;if(benchmarkPose.enabled){benchmarkCamera=camera;benchmarkYaw=cameraYaw;benchmarkHeight=cameraGroundOffset;LOG("Benchmark initial camera=%.3f,%.3f,%.3f yaw=%.3f",camera.x,camera.y,camera.z,cameraYaw);}requestSeq=(uint32_t)(seconds()*1000);}
 if(requestPending&&seconds()-requestTime>2){LOG("Input ACK timeout seq=%u",requestPending);patrolRequest=0;routePoints=0;clearMoveMarkers();requestPending=0;commandQueue.clear();input.hapticRequests.clear();input.groupRequests.clear();commandFailed=true;}
 pollOriginalDialog();
 wristMapPoseValid=false;mapHovered=false;infoPanelHovered=false;originalDialogHovered=false;
 input.hapticFocused=focused;if(!focused)input.tap.reset();
 if(!focused){if(mapPenStroke)mapPenNeedsEnd=true;mapPenStroke=false;if(sunAngleAdjustment.moving)sunAngleAdjustment.step(0,0,0,seconds());coopMenuOpen=false;followCamera.active=false;input.lastNextGroup=true;input.lastFollowClick=input.lastWarpClick=true;orbitCamera.active=false;infoPanelGesture=false;mapGesture=false;input.lastMinimap=true;routePoints=0;clearMoveMarkers();pointerValid=false;radial.close();selectionGesture.cancel();selectionPreview.clear();lastTapID=0;commandQueue.clear();input.swallowRight=input.swallowLeft=true;input.lastSelect=input.lastClear=input.selectHeld=input.extendHeld=false;input.lastSpecial=input.lastOrders=input.lastFormations=true;return;}
 XrActiveActionSet active{input.set,XR_NULL_PATH};XrActionsSyncInfo sync{XR_TYPE_ACTIONS_SYNC_INFO};sync.countActiveActionSets=1;sync.activeActionSets=&active;XrResult synced=xrSyncActions(session,&sync);if(synced==XR_SESSION_NOT_FOCUSED){if(sunAngleAdjustment.moving)sunAngleAdjustment.step(0,0,0,seconds());followCamera.active=false;input.lastNextGroup=true;input.lastFollowClick=input.lastWarpClick=true;orbitCamera.active=false;pointerValid=false;return;}ck(synced);
 bool select=input.trigger(session,input.select,input.selectHeld),clear=input.boolean(session,input.clear),extend=input.trigger(session,input.extend,input.extendHeld);
 bool followPressed=input.boolean(session,input.followClick),warpPressed=input.boolean(session,input.warpClick),cameraJump=false;
 bool mapPressed=input.boolean(session,input.minimap);
 bool groundAttack=input.trigger(session,input.groundAttack,input.groundAttackHeld);
 auto clicks=speedClick(warpPressed,followPressed,input.lastWarpClick,input.lastFollowClick,groundAttack);
 if(shadowDebugEnabled&&!originalDialogMode&&!radial.kind&&!coopMenuOpen){
  if(followPressed&&!input.lastFollowClick&&!groundAttack)sunAngleAdjustment.toggle(seconds());
  if(sunAngleAdjustment.active){XrActionStateGetInfo sunGi{XR_TYPE_ACTION_STATE_GET_INFO};sunGi.action=input.routed(input.elevateTurn);XrActionStateVector2f sunStick{XR_TYPE_ACTION_STATE_VECTOR2F};ck(xrGetActionStateVector2f(session,&sunGi,&sunStick));sunAngleAdjustment.step(sunStick.isActive?sunStick.currentState.x:0,sunStick.isActive?sunStick.currentState.y:0,dt,seconds());}
 }

 bool special=input.trigger(session,input.special,input.specialHeld),orders=input.boolean(session,input.orders),formations=input.boolean(session,input.formations);
 bool speedChord=groundAttack&&(warpPressed||followPressed)&&!originalDialogMode&&!radial.kind&&!coopMenuOpen;
 if(!speedChord&&!originalDialogMode&&!coopMenuOpen&&mapPressed&&!input.lastMinimap){if(extend){infoPanelShown=!infoPanelShown;if(infoPanelShown)wristMapShown=true;LOG("Selection panel visible=%d",infoPanelShown);}else wristMapShown=!wristMapShown;}input.lastMinimap=mapPressed;
 if(originalDialogMode){special=orders=formations=false;if(clear&&!input.lastClear)originalDialogRequest(4);}
 if(!select)input.swallowRight=false;if(!extend)input.swallowLeft=false;
 XrPosef head=views[0].pose;head.position={(views[0].pose.position.x+views[1].pose.position.x)/2,(views[0].pose.position.y+views[1].pose.position.y)/2,(views[0].pose.position.z+views[1].pose.position.z)/2};
 XrPosef grip;bool gripOK=input.pose(session,input.grip,input.gripSpace,space,time,grip);
 XrPosef leftGrip;bool leftOK=input.pose(session,input.leftGrip,input.leftGripSpace,space,time,leftGrip);
 chatTrackingEyes[0]=views[0];chatTrackingEyes[1]=views[1];
 chatGripValid[0]=gripOK;chatGripValid[1]=leftOK;if(gripOK)chatTrackingGrip[0]=grip;if(leftOK)chatTrackingGrip[1]=leftGrip;
 multiplayerHead=head;wristMapPoseValid=leftOK;if(leftOK)wristMapPose=leftGrip;wristMapViewer={head.position.x,head.position.y,head.position.z};Vec3 mapForward=rotated(head.orientation,{0,0,-1});mapForward.y=0;if(dot(mapForward,mapForward)>.0001f)wristMapForward=normalized(mapForward);
 chatKeyboardInput(session,space,time,dt,!coopMenuOpen,select,extend,clear,orders,formations);
 // Right grip + stick click owns this chord before menus, abilities or
 // pointer commands. Grip alone remains the ground-attack/camera modifier.
 if(speedChord){
  if(clicks.speed>=0&&!select&&!selectionGesture.pending)command(24,(unsigned)clicks.speed);
  input.lastFollowClick=followPressed;input.lastWarpClick=warpPressed;
  input.lastClear=clear;input.lastNextGroup=input.boolean(session,input.nextGroup);input.lastMinimap=mapPressed;
  input.lastOrders=orders;input.lastFormations=formations;input.lastSpecial=special;input.lastExtend=extend;
  input.lastSelect=select;input.swallowRight=select;selectionGesture.cancel();selectionPreview.clear();pointerValid=false;hoverID=hoverItem=0;
  input.tap.reset();return;
 }
 bool menuEvent=false;bool nextGroup=input.boolean(session,input.nextGroup)&&!chatBlockB;
 bool transferChord=groundAttack&&clear&&!input.lastClear&&!originalDialogMode&&!radial.kind;
 bool transferModal=coopMenuOpen;
 if(transferChord&&!transferModal){
  routePoints=0;clearMoveMarkers();selectionGesture.cancel();lastTapID=0;mapGesture=infoPanelGesture=false;
  XrSpaceLocation view{XR_TYPE_SPACE_LOCATION};
  auto valid=XR_SPACE_LOCATION_POSITION_VALID_BIT|XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;
  if(XR_SUCCEEDED(xrLocateSpace(input.viewSpace,space,time,&view))&&(view.locationFlags&valid)==valid)openCoopTransfer(benchmarkPose.enabled?head:view.pose);
  else commandFailed=true;menuEvent=true;input.swallowRight=true;input.swallowLeft=true;
 }
 if(transferModal||coopMenuOpen||transferChord){
  if(coopMenuOpen&&(coopFrozenSelection!=coopSelectionHash||std::any_of(coopChoices.begin(),coopChoices.end(),[](const CoopRecipient&p){return !coopCurrent(p);})))coopMenuOpen=false;
  if(transferModal&&clear&&!input.lastClear)coopMenuOpen=false;
  coopHover=-1;XrPosef aim{};pointerValid=input.pose(session,input.aim,input.aimSpace,space,time,aim);hoverID=hoverItem=0;groundValid=false;
  if(pointerValid){auto world=worldPose(aim);pointerStart={world.position.x,world.position.y,world.position.z};pointerEnd=add(pointerStart,times(rotated(world.orientation,{0,0,-1}),1.2f));
   if(coopMenuOpen){pointCoopTransfer({aim.position.x,aim.position.y,aim.position.z},rotated(aim.orientation,{0,0,-1}));
    if(select&&!input.lastSelect&&!input.swallowRight&&coopHover>=0){if(coopHover<(int)coopChoices.size())sendCoopTransfer(coopChoices[coopHover],coopFrozenSelection);coopMenuOpen=false;}
   }
  }
  followCamera.active=orbitCamera.active=false;input.tap.reset();radial.close();selectionGesture.cancel();selectionPreview.clear();
  input.lastSelect=select;input.lastClear=clear;input.lastNextGroup=nextGroup;input.lastOrders=orders;input.lastFormations=formations;input.lastSpecial=special;input.lastExtend=extend;
  input.lastFollowClick=input.boolean(session,input.followClick);input.lastWarpClick=input.boolean(session,input.warpClick);
  if(!coopMenuOpen){input.swallowRight=true;input.swallowLeft=true;}return;
 }

 // Right grip + B owns B before group cycling and consumes held triggers.
 // Keep modal UI behavior unchanged; deselect is a gameplay command.
 if(groundAttack&&nextGroup&&!originalDialogMode&&!radial.kind){
  if(!input.lastNextGroup){if(routePoints)command(14,3);command(2);LOG("Right grip+B deselect");}
  routePoints=0;clearMoveMarkers();selectionGesture.cancel();selectionPreview.clear();
  movePressValid=false;moveFacing=0;pressFacingCandidate=false;lastMoveTime=0;lastTapID=0;
  mapGesture=infoPanelGesture=false;input.tap.reset();
  input.swallowRight=select;input.swallowLeft=extend;
  input.lastSelect=select;input.lastClear=clear;input.lastNextGroup=nextGroup;
  input.lastOrders=orders;input.lastFormations=formations;input.lastSpecial=special;input.lastExtend=extend;
  input.lastFollowClick=followPressed;input.lastWarpClick=warpPressed;
  return;
 }

 bool tapAllowed=!(chatRightHit||chatLeftHit)&&cameraSpeeds.tapGroups&&gripOK&&leftOK&&!originalDialogMode&&!radial.kind&&!orders&&!formations&&!select&&!extend&&!special&&!groundAttack&&!selectionGesture.pending&&(!routePoints||cameraSpeeds.buttonGroups);
 int tapDirection=input.tap.update(leftOK?Vec3{leftGrip.position.x,leftGrip.position.y,leftGrip.position.z}:Vec3{},gripOK?Vec3{grip.position.x,grip.position.y,grip.position.z}:Vec3{},seconds(),tapAllowed,cameraSpeeds.tapDistance);
 if(tapDirection&&cameraSpeeds.buttonGroups){if(routePoints)command(14,3);routePoints=0;clearMoveMarkers();if(command(2)){input.hapticRequests[requestSeq]=tapDirection>0?1:0;menuEvent=true;lastTapID=0;}}
 else if(tapDirection&&input.groupRequests.empty()){int next=input.groupCycle.next(unitGroups,tapDirection,std::any_of(objects.begin(),objects.end(),[](const Object&o){return o.unitID&&o.selected;}));if(next>=0&&command(20,next,0,0,0,0,true)){input.hapticRequests[requestSeq]=tapDirection>0?1:0;menuEvent=true;lastTapID=0;LOG("Controller tap direction=%d group=%d",tapDirection,next);}}

 bool groupButtonsAllowed=cameraSpeeds.buttonGroups&&input.groupRequests.empty()&&!originalDialogMode&&!radial.kind&&!select&&!orders&&!formations&&!selectionGesture.pending&&!routePoints;
 if(groupButtonsAllowed&&((clear&&!input.lastClear)||(nextGroup&&!input.lastNextGroup))){int direction=clear?-1:1;int next=input.groupCycle.next(unitGroups,direction,std::any_of(objects.begin(),objects.end(),[](const Object&o){return o.unitID&&o.selected;}));if(next>=0)command(20,next,extend?1:0,0,0,0,true);menuEvent=true;lastTapID=0;}
 input.lastNextGroup=nextGroup;
 auto openMenu=[&](int kind,const XrPosef&p,bool valid){menuEvent=true;input.swallowRight=select;input.swallowLeft=extend;if(valid)radial.open(kind,{p.position.x,p.position.y,p.position.z},rotated(head.orientation,{1,0,0}),rotated(head.orientation,{0,1,0}),rotated(head.orientation,{0,0,1}),cameraSpeeds.radialSize);};
 if(!radial.kind&&orders&&!input.lastOrders)openMenu(1,grip,gripOK);
 else if(!radial.kind&&formations&&!input.lastFormations)openMenu(formationMenuKind(groundAttack,extend,u32(selectionInfo.data())!=0),leftGrip,leftOK);
 if(radial.kind){auto&p=radial.kind==1?grip:leftGrip;bool valid=radial.kind==1?gripOK:leftOK;if(valid)radial.update({p.position.x,p.position.y,p.position.z});else{radial.close();menuEvent=true;input.swallowRight=select;input.swallowLeft=extend;}}
 // OBMO events are explicitly published by the original scenario callback, not inferred from listener updates.
 static unsigned lastScriptSerial=0;static bool scriptBaseline=false;
 if(!scriptBaseline){lastScriptSerial=cameraInfo[1];scriptBaseline=true;}
 if(!cameraInfo[0])lastScriptSerial=0;
 bool newScriptCamera=cameraInfo[0]==0x314d4143&&cameraInfo[1]&&cameraInfo[1]!=lastScriptSerial;
 scriptCameraBlocked=scriptCameraGate.update((cameraInfo[10]&0x10000)!=0,newScriptCamera,seconds());
 if(scriptCameraBlocked){followCamera.active=false;orbitCamera.active=false;}
 if(newScriptCamera&&!benchmarkPose.enabled){
  lastScriptSerial=cameraInfo[1];
  float gx=int32_t(cameraInfo[4])/512.f,gy=int32_t(cameraInfo[5])/512.f;
  if(gx>=0&&gy>=0&&gx<terrain.width&&gy<terrain.height&&cameraInfo[6]<=65535){
   followCamera.active=false;orbitCamera.active=false;
   Vec3 headForward=rotated(head.orientation,{0,0,-1});float localYaw=atan2f(-headForward.x,-headForward.z);
   cameraYaw=cameraInfo[6]*(6.283185307f/65536.f)+1.5707963268f-localYaw;
   Vec3 local=turn({head.position.x,0,head.position.z},cameraYaw);
   float x=mythWorldX(gx,originX,scale),z=(gy-originY)*scale;
   camera.x=x-local.x;camera.z=z-local.z;camera.y=terrain.elevation(x,z)+std::max(cameraGroundOffset,1.6f);
   selectionGesture.cancel();selectionPreview.clear();movePressValid=false;input.swallowRight=select;
   restartSceneFade();
   LOG("Script camera event=%u tick=%u xy=%.2f,%.2f yaw=%u",cameraInfo[1],cameraInfo[3],gx,gy,cameraInfo[6]);
  }
 }
 unsigned tutorialCameraBits=0;
 if(!scriptCameraBlocked&&!benchmarkPose.enabled){
 if(clicks.follow||clicks.warp){
  std::vector<Vec3> selected;std::vector<OrbitTarget> targets;
  for(const auto&o:objects)if(o.unitID&&o.selected&&o.health>0&&objectVisible(o)){
   Vec3 p{mythWorldX(o.x,originX,scale),(o.z-originZ)*scale,(o.y-originY)*scale};selected.push_back(p);targets.push_back({o.unitID,o.objectKey,p,true});
  }
  if(!selected.empty()){
   auto eye=worldPose(head);Vec3 eyePos{eye.position.x,eye.position.y,eye.position.z},forward=rotated(eye.orientation,{0,0,-1});
   orbitTargets=std::move(targets);orbitCamera.begin(selected,eyePos,forward,turn({0,0,-1},cameraYaw),terrain);
   followCamera.begin(eyePos,orbitCamera.pivot,forward,followCamera.startingDistance(cameraSpeeds.orbitDistance,cameraSpeeds.reuseFollowDistance),!clicks.warp,cameraSpeeds.aimOnJump,turn({0,0,-1},cameraYaw));
   followCamera.place(camera,cameraYaw,{head.position.x,head.position.y,head.position.z},rotated(head.orientation,{0,0,-1}),orbitCamera.pivot,terrain,cameraSpeeds.aimOnJump);
   cameraJump=true;orbitCamera.active=followCamera.active;
   LOG("Camera %s targets=%zu distance=%.2f",followCamera.active?"follow":"warp",orbitTargets.size(),followCamera.distance);
  }
 }

 XrActionStateGetInfo gi{XR_TYPE_ACTION_STATE_GET_INFO};gi.action=input.routed(input.move);XrActionStateVector2f stick{XR_TYPE_ACTION_STATE_VECTOR2F};ck(xrGetActionStateVector2f(session,&gi,&stick));
 if(stick.isActive)tutorialCameraBits|=tutorialStickBits(stick.currentState.x,stick.currentState.y,6,followCamera.active?8:2);
 if(stick.isActive&&followCamera.active){
  if(FollowCamera::lateralBreak(stick.currentState.x)){followCamera.active=false;orbitCamera.active=false;LOG("Camera follow stopped: lateral stick");}
  else followCamera.adjust(stick.currentState.y,3.f*cameraSpeeds.move,dt,cameraSpeeds.reuseFollowDistance);
 }
 if(stick.isActive&&!followCamera.active){float sx=stick.currentState.x,sy=stick.currentState.y,m=sqrtf(sx*sx+sy*sy);if(m>.18f){float gain=std::min(1.f,(m-.18f)/.82f);Vec3 forward=rotated(head.orientation,{0,0,-1});forward.y=0;forward=normalized(turn(forward,cameraYaw));Vec3 right{-forward.z,0,forward.x};camera=add(camera,times(add(times(right,sx/m),times(forward,sy/m)),gain*3.f*cameraSpeeds.move*dt));}}
   if(orbitCamera.active&&(followCamera.active||!groundAttack)&&!orbitTargets.empty()){
    Vec3 center{};unsigned alive=0;bool removed=false;
    for(auto&t:orbitTargets){if(t.alive){bool found=false;for(const auto&o:objects)if(o.unitID==t.id&&o.objectKey==t.key){found=true;t.position={mythWorldX(o.x,originX,scale),(o.z-originZ)*scale,(o.y-originY)*scale};if(o.health<=0)t.alive=false;break;}if(!found)t.alive=false;if(!t.alive)removed=true;}
     if(t.alive){center=add(center,t.position);alive++;}
    }
    if(!alive&&followCamera.active){followCamera.active=false;orbitCamera.active=false;LOG("Camera follow stopped: targets lost");}
    if(alive)center=times(center,1.f/alive);orbitCamera.followSurvivors(camera,center,removed,alive!=0,dt);
   }
 // Separate axis dead zones allow height and yaw to be adjusted independently.
 gi.action=input.routed(input.elevateTurn);XrActionStateVector2f rightStick{XR_TYPE_ACTION_STATE_VECTOR2F};ck(xrGetActionStateVector2f(session,&gi,&rightStick));
 if(rightStick.isActive&&!(shadowDebugEnabled&&sunAngleAdjustment.active)){tutorialCameraBits|=tutorialStickBits(rightStick.currentState.x,-rightStick.currentState.y,groundAttack?4:0,8);auto axis=[](float v){return copysignf(std::clamp((fabsf(v)-.18f)/.82f,0.f,1.f),v);};
  float horizontal=axis(rightStick.currentState.x),angle=-horizontal*1.5707963268f*cameraSpeeds.stick*dt;
  if(followCamera.active){if(!groundAttack)followCamera.rotate(angle);cameraYaw+=angle;}
  else if(!groundAttack&&horizontal!=0){
   if(!orbitCamera.active){
    std::vector<Vec3>selected;orbitTargets.clear();for(const auto&o:objects)if(o.unitID&&o.selected&&o.health>0&&objectVisible(o)){
     Vec3 p{mythWorldX(o.x,originX,scale),(o.z-originZ)*scale,(o.y-originY)*scale};selected.push_back(p);orbitTargets.push_back({o.unitID,o.objectKey,p,true});
    }
    auto eye=worldPose(head);orbitCamera.begin(selected,{eye.position.x,eye.position.y,eye.position.z},normalized(rotated(eye.orientation,{0,0,-1})),turn({0,0,-1},cameraYaw),terrain,cameraSpeeds.orbitDistance);
    LOG("Orbit begin selected=%zu pivot=%.3f,%.3f,%.3f",selected.size(),orbitCamera.pivot.x,orbitCamera.pivot.y,orbitCamera.pivot.z);
   }
   orbitCamera.step(camera,cameraYaw,{head.position.x,head.position.y,head.position.z},angle,terrain);
  }else{orbitCamera.active=false;cameraYaw+=angle;}
  float lift=axis(rightStick.currentState.y)*3.f*cameraSpeeds.move*dt;
  if(orbitCamera.active||cameraJump)camera.y+=lift;else cameraGroundOffset=std::clamp(cameraGroundOffset+lift,.2f,80.f);
 }else if(!followCamera.active)orbitCamera.active=false;
 if(followCamera.active)followCamera.place(camera,cameraYaw,{head.position.x,head.position.y,head.position.z},rotated(head.orientation,{0,0,-1}),orbitCamera.pivot,terrain,false);
 Vec3 hp=turn({head.position.x,0,head.position.z},cameraYaw);float worldX=std::clamp(camera.x+hp.x,mythWorldX(terrain.width-1.5f,originX,scale),mythWorldX(.5f,originX,scale)),worldZ=std::clamp(camera.z+hp.z,(-originY+.5f)*scale,(terrain.height-1.5f-originY)*scale);camera.x=worldX-hp.x;camera.z=worldZ-hp.z;float ground=terrain.elevation(worldX,worldZ);
 if(orbitCamera.active||cameraJump)orbitCamera.constrainHeight(camera,ground,head.position.y,cameraGroundOffset);
 else{float target=ground+cameraGroundOffset;camera.y+=(target-camera.y)*(1-expf(-12*dt));}
 }
 if(benchmarkPose.enabled){camera=benchmarkCamera;cameraYaw=benchmarkYaw;cameraGroundOffset=benchmarkHeight;followCamera.active=false;orbitCamera.active=false;}
 input.lastFollowClick=followPressed;input.lastWarpClick=warpPressed;
 refreshChatRays();
 coopHover=-1;XrPosef aim{};pointerValid=chatTrackingValid[cameraSpeeds.swapHands?1:0];
 if(pointerValid)aim=chatTrackingAim[cameraSpeeds.swapHands?1:0];else pointerValid=input.pose(session,input.aim,input.aimSpace,space,time,aim);
 hoverID=0;hoverItem=0;groundValid=false;
 XrPosef localAim=aim;
 if(pointerValid){aim=worldPose(aim);pointerStart={aim.position.x,aim.position.y,aim.position.z};Vec3 direction=normalized(rotated(aim.orientation,{0,0,-1}));float distance=300;groundValid=terrain.ray(pointerStart,direction,groundHit,distance);pointerEnd=groundValid?groundHit:add(pointerStart,times(direction,20));float best=groundValid?distance:300;auto pointerHead=worldPose(head);
  for(auto&o:objects){if(!objectVisible(o)||(!o.unitID&&!pickupCandidate(o)))continue;auto sprite=quads.find(key(o.col,o.seq,o.frame,0));if(sprite==quads.end())continue;
   Vec3 foot{mythWorldX(o.x,originX,scale),(o.z-originZ)*scale,(o.y-originY)*scale};
   Vec3 normal=normalized({pointerHead.position.x-foot.x,0,pointerHead.position.z-foot.z});
   auto bounds=fixedUnitFrame(o,sprite->second);
   float t=rayUnitFrame(pointerStart,direction,add(foot,times(normal,.018f)),normal,bounds.left-.025f,bounds.right+.025f,bounds.bottom-.025f,bounds.top+.025f);
   if(t>=0&&t<best){best=t;hoverID=o.unitID;hoverItem=o.unitID?0:o.objectKey;hoverFriendly=o.friendly;pointerEnd=add(pointerStart,times(direction,t));}
  }
 }
 if(mapPenNeedsEnd&&mapPen.available(seconds())&&command(25,3))mapPenNeedsEnd=false;
 if(mapPenMesh!=u32(mapPen.data.data()+24)){mapPenMesh=u32(mapPen.data.data()+24);mapPenEnabled=mapPenStroke=false;mapPenX=mapPenY=-1;}
 if(mapPen.available(seconds())&&!requestPending&&commandQueue.empty()&&seconds()-mapPenChangedAt>.5){bool enabled=(mapPen.data[0]&2)!=0;if(enabled&&!mapPenEnabled)mapPenForward=cameraSpeeds.mapRotation?turn(wristMapForward,cameraYaw):Vec3{0,0,-1};mapPenEnabled=enabled;}
 if(!mapPen.available(seconds())||commandFailed){endMapPenStroke();mapPenEnabled=false;}
 if(!chat.canSend(seconds())||originalDialogMode)chat.open=false;
 bool chatHit=chat.open&&(cameraSpeeds.swapHands?chatLeftHit:chatRightHit);
 if(chatHit){pointerEnd=chatRayEnd[cameraSpeeds.swapHands?1:0];hoverID=hoverItem=0;groundValid=false;}
 bool chatConsumed=chatHit;
 bool mapConsumed=mapGesture||infoPanelGesture||originalDialogMode||chatConsumed;
 if(pointerValid&&!chatConsumed&&!radial.kind&&!menuEvent&&(!selectionGesture.pending||mapPenEnabled)){
  Vec3 direction=normalized(rotated(localAim.orientation,{0,0,-1}));
  float du=0,dv=0;Vec3 dialogDirection=normalized(rotated(aim.orientation,{0,0,-1}));
  if(originalDialogMode)pointerEnd=add(pointerStart,times(dialogDirection,2.f));
  bool dialogHit=hitOriginalDialog(pointerStart,dialogDirection,pointerEnd,du,dv);originalDialogHovered=dialogHit;
  if(originalDialogMode){hoverID=hoverItem=0;groundValid=false;if(dialogHit){if(select&&!input.lastSelect&&!input.swallowRight)originalDialogRequest(3,int(du*10000),int(dv*10000));}mapConsumed=true;}
  infoPanelHovered=!originalDialogMode&&hitSelectionPanel({localAim.position.x,localAim.position.y,localAim.position.z},direction,mapPointerHit);
  if(infoPanelHovered){pointerEnd=mapPointerHit;hoverID=hoverItem=0;groundValid=false;if(select&&!input.lastSelect){__android_log_print(ANDROID_LOG_INFO,"MythDialogs","Panel click button=%d",infoPanelButton);infoPanelGesture=true;mapGesture=false;if(infoPanelButton){if(infoPanelButton<=2&&!chat.open){endMapPenStroke();infoButtonFlash[infoPanelButton-1]=seconds();originalDialogRequest(infoPanelButton);}else if(infoPanelButton>=5){if(chat.canSend(seconds())){chat.show(infoPanelButton==6);LOG("VR chat opened mode=%s",chat.mode?"Whisper":"Yell");}}else mapPenButton(infoPanelButton);selectionGesture.cancel();radial.close();}}mapConsumed=true;}
  mapHovered=!originalDialogMode&&!infoPanelHovered&&hitWristMap({localAim.position.x,localAim.position.y,localAim.position.z},direction,mapPointerHit,mapPointerU,mapPointerV,true);
  if(mapHovered){pointerEnd=mapPointerHit;hoverID=hoverItem=0;groundValid=false;
   // Pen can acquire the map after the trigger was pressed outside it.
   // A toolbar press still belongs to that button until release.
   if(select&&!input.swallowRight&&!infoPanelGesture&&(mapPenEnabled||!input.lastSelect)){
    mapGesture=mapConsumed=true;mapPressTime=seconds();
    selectionGesture.cancel();lastTapID=0;
   }
   if(mapGesture&&select){if(mapPenEnabled){mapPenPoint(mapPointerU,mapPointerV);mapWarpRejected=false;}else if(!input.lastSelect||seconds()-mapPressTime>=.25)mapWarpRejected=!warpFromWristMap(mapPointerU,mapPointerV,head);}
  }
 }
 if(!select||!mapHovered||!pointerValid||originalDialogMode||radial.kind)endMapPenStroke();
 if(!select){mapGesture=false;infoPanelGesture=false;}
 bool menuActive=radial.kind!=0;
 if(menuActive&&!menuEvent){bool released=radial.kind==1?(!orders&&input.lastOrders):(!formations&&input.lastFormations);
  if(released){int kind=radial.kind,item=radial.release(kind);
   if(item>=0){
    if(kind==3||kind==4){command(14,3);command(kind==3?19:20,(unsigned)item,kind==4&&extend?1:0);}
    else command(kind==1?6:8,kind==1?commandMenuActions[item]:(unsigned)item);
   }
   menuEvent=true;input.swallowRight=select;input.swallowLeft=extend;
  }
 }
 auto worldHead=worldPose(head);
 selectionView.origin={worldHead.position.x,worldHead.position.y,worldHead.position.z};selectionView.right=rotated(worldHead.orientation,{1,0,0});selectionView.up=rotated(worldHead.orientation,{0,1,0});selectionView.forward=rotated(worldHead.orientation,{0,0,-1});
 selectionView.left=std::min(tanf(views[0].fov.angleLeft),tanf(views[1].fov.angleLeft));selectionView.r=std::max(tanf(views[0].fov.angleRight),tanf(views[1].fov.angleRight));selectionView.bottom=std::min(tanf(views[0].fov.angleDown),tanf(views[1].fov.angleDown));selectionView.top=std::max(tanf(views[0].fov.angleUp),tanf(views[1].fov.angleUp));
 selectionPreview.clear();
 if(menuActive||menuEvent||!pointerValid){routePoints=0;clearMoveMarkers();selectionGesture.cancel();lastTapID=0;}
 else if(!originalDialogMode&&(!cameraSpeeds.buttonGroups||routePoints>0)&&clear&&!input.lastClear){bool cancelRoute=routePoints>0;routePoints=0;clearMoveMarkers();selectionGesture.cancel();lastTapID=0;input.swallowRight=select;command(cancelRoute?14:2,cancelRoute?3:0);lastMoveTime=0;}
 else if(mapHovered||mapConsumed){ /* UI owns the press through release; retain staged route. */ }
 else {
  if(special&&!input.lastSpecial){routePoints=0;clearMoveMarkers();command(7);lastTapID=0;}
  ScreenPoint p;bool projected=selectionView.project(pointerEnd,p);
  if(select&&!input.lastSelect&&!input.swallowRight&&projected){
   selectionDragView=selectionView;
   selectionDragDepth=std::max(.1f,dot(sub(pointerEnd,selectionDragView.origin),selectionDragView.forward));
   bool hasSelection=std::any_of(objects.begin(),objects.end(),[](const Object&o){return o.selected;});
   pressWaypoint=groundAttack?-1:routeWaypointHit();
   pressGroundAttack=groundAttack;pressItem=groundAttack?0:hoverItem;pressUnit=groundAttack?0:hoverID;pressGroundValid=groundValid;pressGroundHit=groundHit;pressSelectedImmediately=false;pressFacingCandidate=false;moveFacing=0;previewFacing=0;
   selectionGesture.press(p,seconds(),extend&&!input.swallowLeft&&!groundAttack,hasSelection||groundAttack,routePoints>0&&hasSelection&&groundValid&&!hoverID&&!hoverItem&&!groundAttack);
   // Ordinary own-unit clicks commit on press. Additive gestures retain box selection.
   auto pressedUnit=std::find_if(objects.begin(),objects.end(),[](const Object&o){return o.unitID==pressUnit&&o.controllable;});
   pressFacingCandidate=pressUnit&&!groundAttack&&!extend&&originalMode!=3&&pressedUnit!=objects.end()&&pressedUnit->selected&&!routePoints;
   if(pressFacingCandidate){pressFacingSelection=selectionSignature();selectionGesture.allowBox=false;}
   if(pressUnit&&!groundAttack&&!extend&&originalMode!=3&&pressedUnit!=objects.end()&&!pressFacingCandidate){
    clickPointer(false,false,false,{},pressUnit,0,pressGroundValid,pressGroundHit);
    pressSelectedImmediately=true;selectionGesture.allowBox=false;
   }
   movePressValid=hasSelection&&groundValid&&((!hoverID&&!hoverItem)||groundAttack);
   movePressPoint=(routePoints==4&&!groundAttack)?moveMarkerPoints.back():groundHit;if(movePressValid){movePressDiameter=selectedUnitDiameter();if(!routePoints){moveMarkerPoints.clear();captureMoveMarkerUnits();}if(!groundAttack&&pressWaypoint<0)requestFormation(movePressPoint,routePoints>0||(extend&&!input.swallowLeft));}
  }
  // Immediate click selection arrives asynchronously from the engine. Promote
  // only once this exact unit is the sole selected, controllable actor.
  if(pressSelectedImmediately&&!pressFacingCandidate&&selectionGesture.pending){
   unsigned selectedCount=0;bool selectedTarget=false;
   for(const auto&o:objects)if(o.selected){selectedCount++;selectedTarget=o.unitID==pressUnit&&o.controllable;}
   if(selectedCount==1&&selectedTarget){pressFacingCandidate=true;pressFacingSelection=selectionSignature();movePressValid=false;}
  }
  if(pressFacingCandidate&&selectionGesture.pending){
   if(pressFacingSelection!=selectionSignature()){selectionGesture.cancel();pressFacingCandidate=false;movePressValid=false;}
   else if(seconds()-selectionGesture.began>=.35){
    if(!movePressValid){auto u=std::find_if(objects.begin(),objects.end(),[](const Object&o){return o.unitID==pressUnit;});if(u!=objects.end()){movePressPoint=unitCenter(*u);movePressPoint.y=terrain.elevation(movePressPoint.x,movePressPoint.z);movePressDiameter=selectedUnitDiameter();moveMarkerPoints.clear();captureMoveMarkerUnits();formationMarkerUnits=moveMarkerUnits.size();formationMarkerPoints=1;for(const auto&marker:moveMarkerUnits){auto obj=std::find_if(objects.begin(),objects.end(),[&](const Object&o){return o.unitID==marker.id;});formationMarkers.push_back(unitCenter(*obj));formationBlocked.push_back(0);}movePressValid=true;}}
    if(groundValid){unsigned facing=destinationFacing(movePressPoint,groundHit);if(facing)moveFacing=facing;}
   }
  }
  // Freeze the projection plane for the entire gesture, including HMD/camera motion.
  projected=selectionDragView.project(pointerEnd,p);
  if(selectionGesture.pending&&projected){selectionGesture.update(p,seconds());
   if(selectionGesture.box)for(const auto&o:objects){ScreenPoint q;if(o.controllable&&visibleUnit(o,q)&&selectionDragView.project(unitCenter(o),q)&&selectionGesture.contains(q))selectionPreview.push_back(o.unitID);}
  }
  if(!pressFacingCandidate&&selectionGesture.pending&&pressWaypoint<0&&movePressValid&&!pressGroundAttack&&!selectionGesture.allowBox&&seconds()-selectionGesture.began>=.35&&groundValid){
   unsigned facing=destinationFacing(movePressPoint,groundHit);if(facing)moveFacing=facing;
   if(moveFacing&&(!previewFacing||facingDistance(moveFacing,previewFacing)>=364)&&!requestPending&&commandQueue.empty()&&seconds()-lastFacingPreview>=.12)
    requestFormation(movePressPoint,routePoints>0||selectionGesture.additive,moveFacing,true);
  }
  if(!select&&input.lastSelect&&selectionGesture.pending){
   if(pressFacingCandidate){if(seconds()-selectionGesture.began>=.35){if(moveFacing&&pressFacingSelection==selectionSignature())command(22,moveFacing);}else if(!pressSelectedImmediately)clickPointer(false,false,false,{},pressUnit,0,pressGroundValid,pressGroundHit);movePressValid=false;moveFacing=0;lastMoveTime=0;pressFacingCandidate=false;}
   else if(pressSelectedImmediately){ /* Already committed on press; release cannot issue a world command. */ }
   else if(pressWaypoint>=0&&routePoints>=2&&!pressGroundAttack){
    patrolFlashPoint=waypointHitPoint;patrolFlashDiameter=lastMoveDiameter;
    if(command(14,pressWaypoint==0?1:2)){patrolRequest=requestSeq;routePoints=0;lastMoveTime=0;}
    pressWaypoint=-1;
   }
   else if(selectionGesture.box){routePoints=0;clearMoveMarkers();selectGroup(selectionPreview,selectionGesture.additive);lastTapID=0;}
   else if(pressItem&&!pressGroundAttack){routePoints=0;clearMoveMarkers();lastTapID=0;command(17,pressItem);pickupEligible=false;}
   else {if(movePressValid&&!pressGroundAttack&&moveFacing!=previewFacing)requestFormation(movePressPoint,routePoints>0||selectionGesture.additive,moveFacing,true);
    clickPointer(selectionGesture.additive,pressGroundAttack,movePressValid,movePressPoint,pressUnit,pressItem,pressGroundValid,pressGroundHit);}

   selectionGesture.cancel();selectionPreview.clear();
  }
 }
 // Releasing the modifier keeps the route staged; the final click commits it.
 // One candidate, at most four queries/sec; no query during held gestures or menus.
 unsigned signature=selectionSignature();
 if(pickupKey!=hoverItem||pickupSelection!=signature){if(hoverItem||pickupKey)LOG("Pickup hover old=%08x new=%08x selection=%08x",pickupKey,hoverItem,signature);pickupEligible=false;pickupRequest=0;pickupKey=hoverItem;pickupSelection=signature;}
 if(hoverItem&&!menuActive&&!menuEvent&&!select&&!groundAttack&&!requestPending&&commandQueue.empty()&&seconds()-pickupQueryTime>=.25){
  if(command(16,hoverItem)){pickupRequest=requestSeq;pickupQueryTime=seconds();}
 }
 if(authenticated&&!requestPending&&commandQueue.empty()&&seconds()-soundRangePoll>=1){
  soundRangePoll=seconds();int wanted=100;FILE*f=fopen(soundRangePath,"rb");if(f){char b[5]{};size_t n=fread(b,1,5,f);fclose(f);if(n==3&&b[0]>='1'&&b[0]<='2'&&b[1]>='0'&&b[1]<='9'&&b[2]>='0'&&b[2]<='9'){int v=atoi(b);if(v>=100&&v<=200)wanted=v;}}
  if(wanted!=soundRangeApplied){soundRangeSent=wanted;command(18,wanted);}
 }
 // Current input only: do not accumulate events across narration/stage changes.
 static double lastTutorialCamera=0;static TutorialCameraGate tutorialGate;
 tutorialCameraBits=tutorialGate.update(cameraInfo[9],tutorialCameraBits,dt);
 if(levelAssetPrefix=="levels/3030746d/"&&!originalDialogMode&&!radial.kind&&tutorialCameraBits&&seconds()-lastTutorialCamera>=.2&&!requestPending&&commandQueue.empty()){
  if(command(21,tutorialCameraBits,cameraInfo[9]))lastTutorialCamera=seconds();
 }
 // Low-rate, coalesced presentation-camera updates never queue ahead of player commands.
 if(authenticated&&!requestPending&&commandQueue.empty()&&seconds()-lastCameraSync>=.2){
  auto position=worldPose(head);auto forward=rotated(position.orientation,{0,0,-1});
  float yaw=atan2f(forward.z,-forward.x);if(yaw<0)yaw+=6.283185307f;
  int x=lroundf(mythGameX(position.position.x,originX,scale)*512),y=lroundf((position.position.z/scale+originY)*512),z=lroundf((position.position.y/scale+originZ)*512);
  if(command(12,((unsigned)lroundf(yaw*(65536.f/6.283185307f)))&65535,scriptCameraBlocked?1:0,std::clamp(x,0,(int(terrain.width)-1)*512-1),std::clamp(y,0,(int(terrain.height)-1)*512-1),std::clamp(z,-32768,65536)))lastCameraSync=seconds();
 }
 if(menuActive||menuEvent)pointerValid=false;
 input.lastSelect=select;input.lastClear=clear;
 input.lastSpecial=special;input.lastOrders=orders;input.lastFormations=formations;input.lastExtend=extend;
}

