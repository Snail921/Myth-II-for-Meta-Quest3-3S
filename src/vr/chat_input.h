// Included after command queue and worldPose declarations.
#include "chat_controls.h"
#include "hand_pointer.h"

static int chatHover=-1,chatLeftHover=-1;
static bool chatRightHit=false,chatLeftHit=false,chatPoseReady=false,chatBlockB=false;
static Vec3 chatRayStart[2],chatRayEnd[2];
static XrPosef chatTrackingAim[2]{};static bool chatTrackingValid[2]={false,false};
static XrView chatTrackingEyes[2]{};
static XrPosef chatTrackingGrip[2]{};static bool chatGripValid[2]={false,false};
struct ChatPlane {Vec3 center,right,up,normal;};
static ChatPlane chatCachedPlane;
static XrQuaternionf chatOrientation{};
static void updateChatPlane(float dt){
 if(!chat.open){chatPoseReady=false;chatRightHit=chatLeftHit=false;return;}
 auto p=worldPose(multiplayerHead);
 if(chatPoseReady&&cameraSpeeds.chatFixed)return;
 float a=!chatPoseReady||cameraSpeeds.chatSmooth<=0?1.f:1.f-expf(-std::max(0.f,dt)/cameraSpeeds.chatSmooth);
 auto target=p.orientation;
 if(chatPoseReady&&chatOrientation.x*target.x+chatOrientation.y*target.y+chatOrientation.z*target.z+chatOrientation.w*target.w<0){target.x=-target.x;target.y=-target.y;target.z=-target.z;target.w=-target.w;}
 if(!chatPoseReady)chatOrientation=target;
 else{
  chatOrientation.x+=(target.x-chatOrientation.x)*a;chatOrientation.y+=(target.y-chatOrientation.y)*a;
  chatOrientation.z+=(target.z-chatOrientation.z)*a;chatOrientation.w+=(target.w-chatOrientation.w)*a;
  float n=sqrtf(chatOrientation.x*chatOrientation.x+chatOrientation.y*chatOrientation.y+chatOrientation.z*chatOrientation.z+chatOrientation.w*chatOrientation.w);
  if(n>0){chatOrientation.x/=n;chatOrientation.y/=n;chatOrientation.z/=n;chatOrientation.w/=n;}
 }
 auto&q=chatOrientation;
 chatCachedPlane.right=rotated(q,{1,0,0});chatCachedPlane.up=rotated(q,{0,1,0});chatCachedPlane.normal=rotated(q,{0,0,1});
 // Translation/depth follows immediately; gaze rotation alone is smoothed.
 chatCachedPlane.center=add(Vec3{p.position.x,p.position.y,p.position.z},rotated(q,{cameraSpeeds.chatX,cameraSpeeds.chatY,-cameraSpeeds.chatDistance}));
 chatPoseReady=true;
}
static ChatPlane chatPlane(){if(!chatPoseReady)updateChatPlane(0);return chatCachedPlane;}
static bool hitChatKeyboard(Vec3 origin,Vec3 direction,Vec3&hit,int&hover){
 hover=-1;if(!chat.open||!chat.canSend(seconds())||originalDialogMode)return false;
 auto p=chatPlane();float den=dot(direction,p.normal);if(fabsf(den)<.00001f)return false;
 float t=dot(sub(p.center,origin),p.normal)/den;if(t<=0)return false;
 Vec3 point=add(origin,times(direction,t)),delta=sub(point,p.center);
 float x=dot(delta,p.right),y=dot(delta,p.up);
 if(x<-.42f||x>.42f||y<-.15f||y>.15f)return false;
 auto keys=chatKeys(chat.shift);
 for(unsigned i=0;i<keys.size();i++){auto&k=keys[i];float l=-.42f+k.x*.06f,top=.15f-k.y*.06f;
  if(x>=l&&x<=l+k.w*.06f&&y<=top&&y>=top-.06f){hover=i;break;}}
 hit=point;return true;
}
// Reuse the sampled poses after camera movement; never locate a second pose
// for a different rendering path in the same frame.
static void refreshChatRays(){
 chatRightHit=chatLeftHit=false;chatHover=chatLeftHover=-1;
 if(!chat.open)return;
 updateChatPlane(0);
 for(int hand=0;hand<2;hand++){
  if(!chatTrackingValid[hand])continue;
  auto ray=worldPose(chatTrackingAim[hand]);chatRayStart[hand]={ray.position.x,ray.position.y,ray.position.z};
  bool hit=hitChatKeyboard(chatRayStart[hand],normalized(rotated(ray.orientation,{0,0,-1})),chatRayEnd[hand],hand?chatLeftHover:chatHover);
  if(hand)chatLeftHit=hit;else chatRightHit=hit;
 }
}
static bool sendChat(){
 if(!chat.canSend(seconds())||peer<0||!authenticated||chat.draft.empty()||chat.draft.size()>191)return false;
 unsigned chunks=(chat.draft.size()+19)/20;
 if(commandQueue.size()+chunks+2>32)return false;
 // Enqueue the whole bounded transaction before flushing. No partially queued draft.
 commandQueue.push_back({0x31444d43,++requestSeq,26,chat.mode?1u:0u,(unsigned)chat.draft.size(),0,0,0});
 for(size_t i=0;i<chat.draft.size();i+=20){
  std::array<uint32_t,8> packet{0x31444d43,++requestSeq,27,0,0,0,0,0};
  memcpy(packet.data()+3,chat.draft.data()+i,std::min(size_t(20),chat.draft.size()-i));commandQueue.push_back(packet);
 }
 commandQueue.push_back({0x31444d43,++requestSeq,28,0,0,0,0,0});return flushCommand();
}
static void pressChatCode(int code){
 if(code==261){chat.open=false;chat.draft.clear();return;}
 if(code==260){if(chat.draft.empty()||sendChat()){chat.open=false;chat.draft.clear();}else commandFailed=true;return;}
 chat.edit(code);
}

static void chatKeyboardInput(XrSession session,XrSpace space,XrTime time,float dt,bool allow,bool&leftTrigger,bool&rightTrigger,bool&y,bool&a,bool&x){
 updateChatPlane(dt);chatRightHit=chatLeftHit=false;chatHover=chatLeftHover=-1;
 chatTrackingValid[0]=chatTrackingValid[1]=false;
 input.physical=true;static bool physicalHeld[2]={false,false};
 bool raw[6]={input.trigger(session,input.select,physicalHeld[0]),input.trigger(session,input.extend,physicalHeld[1]),input.boolean(session,input.orders),input.boolean(session,input.formations),input.boolean(session,input.clear),input.boolean(session,input.nextGroup)};
 static ChatButtons buttons;
 bool eligible=chat.open&&chat.canSend(seconds())&&!originalDialogMode&&!radial.kind&&allow&&!selectionGesture.pending;
 if(eligible){
  for(int hand=0;hand<2;hand++){
   XrPosef ray;bool valid=input.pose(session,hand?input.leftAim:input.aim,hand?input.leftAimSpace:input.aimSpace,space,time,ray);
   if(!valid)continue;
   static double lastPoseLog[2]={0,0};
   if(seconds()-lastPoseLog[hand]>2){
    lastPoseLog[hand]=seconds();input.logAimSource(session,hand!=0);
    LOG("ChatAimPosition hand=%d pos=%.3f,%.3f,%.3f head=%.3f,%.3f,%.3f",hand,ray.position.x,ray.position.y,ray.position.z,multiplayerHead.position.x,multiplayerHead.position.y,multiplayerHead.position.z);
    XrPosef gripPose;
    if(input.pose(session,hand?input.leftGrip:input.grip,hand?input.leftGripSpace:input.gripSpace,space,time,gripPose)){
     auto d=rotated(ray.orientation,{0,0,-1});auto local=rotated(handInverse(gripPose.orientation),d);
     LOG("ChatAim hand=%d gripQ=%.3f,%.3f,%.3f,%.3f aimQ=%.3f,%.3f,%.3f,%.3f localDir=%.3f,%.3f,%.3f",hand,gripPose.orientation.x,gripPose.orientation.y,gripPose.orientation.z,gripPose.orientation.w,ray.orientation.x,ray.orientation.y,ray.orientation.z,ray.orientation.w,local.x,local.y,local.z);
    }
   }
   chatTrackingAim[hand]=ray;chatTrackingValid[hand]=true;
  }
 }

 refreshChatRays();
 auto action=buttons.step(raw,chatLeftHit,chatRightHit);
 auto press=[&](int index){if(!chat.open||index<0)return;auto keys=chatKeys(chat.shift);if(index<int(keys.size()))pressChatCode(keys[index].code);};
 if(action.enter)pressChatCode(260);
 else {
  if(action.backspace)pressChatCode(257);
  if(action.right)press(chatHover);
  if(action.left)press(chatLeftHover);
 }
 input.physical=false;
 auto available=[&](int logical){int i=input.routedKey(logical);return raw[i]&&!action.blocked[i];};
 leftTrigger=available(0);rightTrigger=available(1);a=available(2);x=available(3);y=available(4);chatBlockB=action.blocked[input.routedKey(5)];

}
