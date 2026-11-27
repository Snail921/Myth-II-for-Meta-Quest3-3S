#define XR_USE_PLATFORM_ANDROID
#define XR_USE_GRAPHICS_API_OPENGL_ES
#include <jni.h>
#include "sprite_direction.h"
#include "map_coordinates.h"
#include "color_pipeline.h"
#include "shadow_debug_shader.h"
#include "unified_shadows.h"
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include "scene_correction.h"
#include "bloom.h"
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <android_native_app_glue.h>
#include <android/log.h>
#include <android/asset_manager.h>
#include <vector>
#include <map>
#include <array>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <algorithm>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include "diagnostic_log.h"
#define LOG(...) diagnosticLog(__VA_ARGS__)
static void ck(XrResult r){if(XR_FAILED(r)){LOG("XR failure %d",r);throw std::runtime_error("OpenXR");}}
#include "loading_events.h"
#include "level_config.h"
#include "ui_contrast.h"
#include "pointer_style.h"
#include "benchmark_pose.h"
using Mat=std::array<float,16>;
static Mat mul(const Mat&a,const Mat&b){Mat c{};for(int r=0;r<4;r++)for(int k=0;k<4;k++)for(int j=0;j<4;j++)c[j*4+r]+=a[k*4+r]*b[j*4+k];return c;}
static Mat projection(XrFovf f){float l=tanf(f.angleLeft),r=tanf(f.angleRight),b=tanf(f.angleDown),t=tanf(f.angleUp),n=.05f,z=500;return {2/(r-l),0,0,0,0,2/(t-b),0,0,(r+l)/(r-l),(t+b)/(t-b),-(z+n)/(z-n),-1,0,0,-2*z*n/(z-n),0};}
static Mat view(XrPosef p){float x=p.orientation.x,y=p.orientation.y,z=p.orientation.z,w=p.orientation.w;Mat m={1-2*y*y-2*z*z,2*x*y-2*z*w,2*x*z+2*y*w,0,2*x*y+2*z*w,1-2*x*x-2*z*z,2*y*z-2*x*w,0,2*x*z-2*y*w,2*y*z+2*x*w,1-2*x*x-2*y*y,0,0,0,0,1};for(int i=0;i<3;i++)m[12+i]=-(m[i]*p.position.x+m[4+i]*p.position.y+m[8+i]*p.position.z);return m;}
static std::vector<unsigned char> sharedAsset(android_app*a,const char*n){
 std::string path=std::string(a->activity->internalDataPath)+"/shared-assets/"+n;
 FILE*f=fopen(path.c_str(),"rb");if(!f)throw std::runtime_error("shared asset missing");
 fseek(f,0,SEEK_END);long size=ftell(f);rewind(f);if(size<=0||size>16*1024*1024){fclose(f);throw std::runtime_error("shared asset size");}
 std::vector<unsigned char>b(size);bool ok=fread(b.data(),1,b.size(),f)==b.size();fclose(f);if(!ok)throw std::runtime_error("shared asset read");return b;
}
static std::vector<unsigned char> asset(android_app*a,const char*n){pumpLoadingEvents();if(!levelExternalRoot.empty()&&strcmp(n,"ground-attack-marker.rgba")){if(strchr(n,'/')||strchr(n,'\\'))throw std::runtime_error("asset name");std::string path=levelExternalRoot+n;FILE*f=fopen(path.c_str(),"rb");if(!f)throw std::runtime_error("external asset missing");if(fseek(f,0,SEEK_END)){fclose(f);throw std::runtime_error("external asset seek");}long size=ftell(f);if(size<0||size>512L*1024*1024){fclose(f);throw std::runtime_error("external asset size");}rewind(f);std::vector<unsigned char>b(size);bool ok=fread(b.data(),1,b.size(),f)==b.size();fclose(f);if(!ok)throw std::runtime_error("external asset short");return b;}std::string path=(!levelAssetPrefix.empty()&&strcmp(n,"ground-attack-marker.rgba")&&strcmp(n,"fog.pack")&&strcmp(n,"water-light.pack"))?levelAssetPrefix+n:n;AAsset*f=AAssetManager_open(a->activity->assetManager,path.c_str(),AASSET_MODE_BUFFER);if(!f)throw std::runtime_error(n);size_t len=AAsset_getLength(f);std::vector<unsigned char>b(len);if(AAsset_read(f,b.data(),len)!=(int)len)throw std::runtime_error("asset short");AAsset_close(f);return b;}
static uint32_t u32(const void*p){uint32_t v;memcpy(&v,p,4);return v;}
static uint16_t u16(const void*p){uint16_t v;memcpy(&v,p,2);return v;}
static int16_t i16(const void*p){int16_t v;memcpy(&v,p,2);return v;}
static int32_t i32(const void*p){int32_t v;memcpy(&v,p,4);return v;}
#include "render_culling.h"
struct Vertex{float x,y,z,u,v;};
#include "health_bar.h"
struct Quad{uint32_t col,seq,frame,dir,views,seqflags,flags,texture;float rx,ry,scale,w,h;};
// Original bitmap-instance flag 4 identifies alpha-blended art, including
// target-dummy straw and the warlock's fireball/contrail. Keep it out of cutout batches.
static bool blendedSprite(const Quad&q){return (q.flags&4u)!=0;}
struct Object{float x,y,z,yaw,scale;unsigned col,seq,frame,unitID;bool selected;int family=-1,team=-1,owner=-1;bool controllable=false,friendly=false;unsigned palette=0,objectKey=0,actorFlags=0;float motionX=1,motionY=0,motionZ=0;unsigned rawFlags=0;unsigned objectType=0;unsigned inheritedLightingOwner=0;int transfer=0,transferPhase=0;float health=-1,mana=-1;float frameRadius=0,frameBottom=0,frameTop=0;};
// Original 1.8.1 renderer 0x44b0bb: unconditional exclusion before normal/reflected sprite submission.
// Physical object sprites keep receiving/casting shadows after the actor is
// detached on death (type 4). Opaque projectile art is physical too;
// alpha-blended projectile effects and emitter particles remain excluded.
static bool enhancedSpritePath=true;static bool billboardShaderShadows=true,billboardShaderLighting=true;
static bool physicalProjectileSprite(const Object&o);
static bool physicalSprite(const Object&o){return o.unitID||o.objectType==1||o.objectType==2||o.objectType==4||o.inheritedLightingOwner||physicalProjectileSprite(o);}
static unsigned spriteLightingOwner(const Object&o,size_t index){return !enhancedSpritePath?0:o.inheritedLightingOwner?o.inheritedLightingOwner:(physicalSprite(o)?unsigned(index)+1:0);}
#include "sprite_overlay.h"
static bool objectVisible(const Object&o){return !(o.rawFlags&0x20000200u);}
static std::vector<Object> objects;
static std::map<unsigned,std::array<unsigned char,1024>> livePalettes;
static std::map<unsigned,std::array<unsigned char,84>> liveHues;
struct ModelPose{unsigned geometry;int32_t pos[3];int16_t matrix[9];unsigned owner=0;unsigned permutation=0,identity=0;};
static std::vector<ModelPose> modelPoses;static bool modelPosesValid=false;
#include "surface_updates.h"
#include "live_lights.h"
#include "live_particles.h"
static std::vector<LiveParticle> liveParticles;
static std::vector<LiveLight> liveLights;static bool liveLightsValid=false;
static surface::Updates surfaceUpdatesQueue;
static uint64_t snapshotRevision=0;
#include "game_speed.h"
static GameSpeedNotice gameSpeed;
#include "multiplayer_hud_wire.h"
static MultiplayerHud multiplayerHud;
#include "chat_state.h"
#include "map_pen_wire.h"
static MapPen mapPen;
#include "fps_display.h"
static FpsDisplay fpsDisplay;
static double seconds();
static bool liveSceneReady=false;
static std::array<unsigned char,1828> selectionInfo{};
static std::array<unsigned,10> unitGroups{};
struct CoopRecipient {unsigned slot,identity;unsigned char color[4];char name[32];};
static std::vector<CoopRecipient> coopRecipients;static unsigned coopSelectionHash=0;
static bool coopMenuOpen=false;
static std::array<unsigned,11> cameraInfo{};
static char sceneDiagnosticPath[1024]{};static unsigned lastDiagnosticTick=0;static double lastDiagnosticTime=0;
static double perfIPC=0,perfInput=0,perfSurface=0,perfScene=0,perfFX=0,perfControls=0;static unsigned spriteUploads=0,markerBuilds=0;
static double perfStart=0;static unsigned perfFrames=0;
static unsigned tick=0;static int originalMode=-1;
static void acknowledge(uint32_t seq,uint32_t status);
static void acceptFormation(const unsigned char*,size_t);
static std::vector<std::array<int32_t,7>> connectorEdges;
static bool snapshot(const unsigned char*b,size_t size){
 if(size<64||memcmp(b,"MYTHBR01",8)||(u32(b+8)<1||u32(b+8)>25)||u32(b+12)!=u32(b+16)||u32(b+32)!=216||u32(b+36)!=80)return false;
 unsigned version=u32(b+8),ac=u32(b+24),oc=u32(b+28);size_t rawSize=64+ac*216+oc*80;
 if(ac>400||oc>8192)return false;
 size_t expected=rawSize+(version>=2?272:0);unsigned paletteCount=0;
 if(version>=3){if(size<expected+4)return false;paletteCount=u32(b+expected);if(paletteCount>128)return false;expected+=4+paletteCount*1028;}
 size_t modelOffset=expected;unsigned modelCount=0;bool modelValid=false;
 if(version>=4){if(size<expected+4)return false;modelCount=u32(b+expected);modelValid=modelCount!=0xffffffff;if(modelValid&&modelCount>(version>=22?1024u:512u))return false;expected+=4+(modelValid?modelCount*(version>=22?40:36):0);}
 size_t surfaceOffset=expected;
 if(version>=5){if(size<expected+12)return false;unsigned count=u32(b+expected+8);if(count>4)return false;expected+=12+count*surface::PersistentRow;}
 size_t surfaceEnd=expected,lightOffset=expected;
 if(version>=6){if(size<expected+4)return false;unsigned bytes=u32(b+expected);if(bytes<4||bytes>262144)return false;expected+=4+bytes;}
 size_t particleOffset=expected;
 if(version>=7){if(size<expected+4)return false;unsigned pc=u32(b+expected);if(pc>2048)return false;expected+=4+pc*64;}
 size_t particleEnd=expected;
 if(version>=9){if(size<expected+4)return false;unsigned bytes=u32(b+expected);if(bytes!=0&&bytes!=(version>=10?4808:4304))return false;expected+=4+bytes;}
 size_t eligibilityOffset=expected;if(version>=11){expected+=ac;if(size<expected)return false;for(unsigned i=0;i<ac;i++)if(b[eligibilityOffset+i]>1)return false;}
 size_t infoOffset=expected;if(version>=12){expected+=1812;if(size<expected||u32(b+infoOffset)>126||b[infoOffset+259]||b[infoOffset+771]||b[infoOffset+1795])return false;}
 if(version>=13){expected+=16;if(size<expected||u32(b+infoOffset+1812)>127||(u32(b+infoOffset+1812)&&(u32(b+infoOffset+1816)>254||u32(b+infoOffset+1820)>255||u32(b+infoOffset+1824)>255)))return false;}
 size_t groupsOffset=expected;if(version>=14){expected+=40;if(size<expected)return false;for(unsigned i=0;i<10;i++)if(u32(b+groupsOffset+i*4)>126)return false;}
 size_t cameraOffset=expected;if(version>=15){expected+=44;if(size<expected)return false;}
 size_t manaOffset=expected;if(version>=16){expected+=ac*4;if(size<expected)return false;}
 size_t connectorOffset=expected;if(version>=17){if(size<expected+4||u32(b+expected)>4096)return false;expected+=4+u32(b+expected)*(version>=18?28:16);}
 size_t hueOffset=expected;if(version>=19){if(size<expected+4||u32(b+expected)>128)return false;expected+=4+u32(b+expected)*88;}
 size_t rosterOffset=expected;if(version>=20){if(size<expected+4||u32(b+expected)>32)return false;expected+=4+u32(b+expected)*44;}

 size_t speedOffset=expected;if(version>=21){expected+=4;if(size<expected||u32(b+speedOffset)>11)return false;}
 size_t multiplayerOffset=expected;if(version>=23){expected+=MultiplayerHud::bytes;if(size<expected||!MultiplayerHud::valid(b+multiplayerOffset))return false;}
 size_t mapPenOffset=expected;if(version>=24){expected+=MapPen::bytes;if(size<expected||!MapPen::valid(b+mapPenOffset))return false;}
 size_t chatOffset=expected;if(version>=25){expected+=ChatState::bytes;if(size<expected||!ChatState::valid(b+chatOffset))return false;}
 if(size!=expected)return false;
 std::vector<CoopRecipient> roster;if(version>=20)for(unsigned i=0;i<u32(b+rosterOffset);i++){auto r=b+rosterOffset+4+i*44;if(u32(r)>=32||r[43])return false;CoopRecipient p{};p.slot=u32(r);p.identity=u32(r+4);memcpy(p.color,r+8,4);memcpy(p.name,r+12,32);roster.push_back(p);}
 liveHues.clear();if(version>=19)for(unsigned i=0;i<u32(b+hueOffset);i++){auto r=b+hueOffset+4+i*88;if(u32(r+84)>8)return false;std::array<unsigned char,84> h;memcpy(h.data(),r+4,84);unsigned k=u32(r);if((k&255)==255||(k>>8)>=128)return false;unsigned col=mappedCollection(k&255);if(col!=255)liveHues[col|(k&~255u)]=h;}
 connectorEdges.clear();if(version>=17){for(unsigned i=0;i<u32(b+connectorOffset);i++){std::array<int32_t,7> e{};memcpy(e.data(),b+connectorOffset+4+i*(version>=18?28:16),version>=18?28:16);connectorEdges.push_back(e);}}
 if(diagnosticEnabled&&sceneDiagnosticPath[0]&&(diagnosticTraceInitial||u32(b+12)<lastDiagnosticTick||u32(b+12)-lastDiagnosticTick>=300||seconds()-lastDiagnosticTime>=10)){lastDiagnosticTime=seconds();lastDiagnosticTick=u32(b+12);std::string tmp=std::string(sceneDiagnosticPath)+".tmp";FILE*f=fopen(tmp.c_str(),"wb");if(f){bool ok=fwrite(b,1,size,f)==size;fclose(f);if(ok)rename(tmp.c_str(),sceneDiagnosticPath);}}
 std::vector<LiveParticle> newParticles;if(version>=7&&!parseParticles(b+particleOffset,particleEnd-particleOffset,newParticles))return false;
 std::vector<LiveLight> newLights;if(version>=6&&!parseLiveLights(b+lightOffset+4,particleOffset-lightOffset-4,newLights))return false;
 int player=0,team=0;std::vector<bool> selected(ac,false);
 if(version>=2){auto m=b+rawSize;player=i32(m);team=i32(m+4);originalMode=i32(m+8);unsigned n=u32(m+16);if(n>126)return false;for(unsigned i=0;i<n;i++){int slot=i16(m+20+i*2);if(slot<0||(unsigned)slot>=ac)return false;selected[slot]=true;}}
 std::vector<int> actorForObject(oc,-1);
 for(unsigned i=0;i<ac;i++){auto actor=b+64+i*216;int index=i16(actor+12);if(u16(actor)&&index>=0&&(unsigned)index<oc)actorForObject[index]=i;}
 std::map<unsigned,const Object*> previous;for(const auto&o:objects)previous[o.objectKey]=&o;
 std::vector<Object> next;
 for(unsigned i=0;i<oc;i++){auto o=b+64+ac*216+i*80;if(!u16(o)||mappedCollection(o[42])==255)continue;
  Object v{i32(o+8)/512.f,i32(o+12)/512.f,i32(o+16)/512.f,u16(o+28)/65536.f,i16(o+2)/256.f,mappedCollection(o[42]),o[43],o[46],0,(o[6]&1)!=0};
  v.rawFlags=u32(o+4);v.objectType=o[40];v.transfer=(int8_t)o[51];v.transferPhase=o[53]?uint16_t(std::min(65535,int((int8_t)o[52])*65536/int((int8_t)o[53]))):0;v.objectKey=(i<<16)|u16(o);v.motionX=cosf(v.yaw*6.283185307f);v.motionY=sinf(v.yaw*6.283185307f);
  auto old=previous.find(v.objectKey);if(old!=previous.end()&&u32(b+12)>=tick&&u32(b+12)-tick<=30){auto&p=*old->second;v.motionX=p.motionX;v.motionY=p.motionY;v.motionZ=p.motionZ;float mx=v.x-p.x,my=v.y-p.y,mz=v.z-p.z;if(mx*mx+my*my+mz*mz>1e-8f){v.motionX=mx;v.motionY=my;v.motionZ=mz;}}
  v.frameRadius=i16(o+0x20)/512.f;v.frameBottom=i16(o+0x22)/512.f;v.frameTop=i16(o+0x24)/512.f;if(u32(o+4)&0x40000u){float lo=v.frameBottom;v.frameBottom=-v.frameTop;v.frameTop=-lo;}v.palette=o[44];int slot=actorForObject[i];if(slot>=0){auto a=b+64+slot*216;v.health=unitHealth(o);if(version>=16){int maximum=i16(b+manaOffset+slot*4);unsigned flags=u16(b+manaOffset+slot*4+2);if(maximum>0&&!(flags&0x20)&&(!(flags&0x10)||(int8_t)a[22]==team))v.mana=std::clamp(float(i16(a+0x7a))/maximum,0.f,1.f);}v.actorFlags=u32(a+4);v.unitID=u16(a);v.family=i16(a+20);v.team=(int8_t)a[22];v.owner=(int8_t)a[23];v.friendly=v.team==team;v.controllable=v.friendly&&v.owner==player&&!(a[7]&0x10);if(version>=11)v.controllable=v.controllable&&b[eligibilityOffset+slot];if(version>=2)v.selected=selected[slot];}
  next.push_back(v);
 }
 resolveSpriteOverlays(next,b+64+ac*216,oc);
 std::map<unsigned,std::array<unsigned char,1024>> palettes;
 if(version>=3)for(unsigned i=0;i<paletteCount;i++){auto p=b+rawSize+276+i*1028;unsigned k=u32(p);if((k&255)==255||(k>>8)>=128)return false;std::array<unsigned char,1024> rgba;memcpy(rgba.data(),p+4,1024);if(mappedCollection(k&255)!=255)palettes[mappedCollection(k&255)|(k&~255u)]=rgba;}
 std::vector<ModelPose> poses;if(modelValid)for(unsigned i=0;i<modelCount;i++){auto p=b+modelOffset+4+i*(version>=22?40:36);ModelPose pose;pose.geometry=u32(p);memcpy(pose.pos,p+4,12);memcpy(pose.matrix,p+16,18);pose.owner=u16(p+34);if(version>=22){pose.permutation=u16(p+36);pose.identity=u16(p+38);}poses.push_back(pose);}
 if(version>=5&&!surfaceUpdatesQueue.stage(b+surfaceOffset,surfaceEnd-surfaceOffset))return false;
 for(auto&p:newParticles)p[13]=mappedCollection(p[13]);liveParticles.swap(newParticles);liveLights.swap(newLights);liveLightsValid=version>=6;
 // Opt-in lifecycle diagnostics: distinguish real actors from transient/hidden objects.
 if(diagnosticEnabled){
  auto describe=[&](const char*event,const Object&v){LOG("Actor trace event=%s tick=%u rev=%llu id=%u key=%08x family=%d col=%u seq=%u frame=%u team=%d owner=%d friendly=%d controllable=%d selected=%d pos=%.3f,%.3f,%.3f objectFlags=%08x actorFlags=%08x type=%u transfer=%d phase=%d",event,u32(b+12),(unsigned long long)snapshotRevision,v.unitID,v.objectKey,v.family,v.col,v.seq,v.frame,v.team,v.owner,v.friendly,v.controllable,v.selected,v.x,v.y,v.z,v.rawFlags,v.actorFlags,v.objectType,v.transfer,v.transferPhase);};
  std::map<unsigned,const Object*> current;
  for(const auto&v:next){current[v.objectKey]=&v;if(!v.unitID)continue;auto it=previous.find(v.objectKey);
   if(diagnosticTraceInitial||it==previous.end())describe("appear",v);
   else{const auto&p=*it->second;if(v.selected!=p.selected)describe(v.selected?"selected":"deselected",v);
    if(((v.rawFlags^p.rawFlags)&0x200)||((v.actorFlags^p.actorFlags)&0x50080000)||v.col!=p.col||v.family!=p.family||v.unitID!=p.unitID)describe("state",v);
    float dx=v.x-p.x,dy=v.y-p.y,dz=v.z-p.z;if(dx*dx+dy*dy+dz*dz>4)describe("jump",v);
   }
  }
  for(const auto&p:objects)if(p.unitID&&!current.count(p.objectKey))describe("disappear",p);
  diagnosticTraceInitial=false;
 }
 coopSelectionHash=2166136261u;if(version>=2){auto m=b+rawSize;for(unsigned i=0;i<u32(m+16);i++)coopSelectionHash=(coopSelectionHash^u16(b+64+i16(m+20+i*2)*216))*16777619u;}
 coopRecipients.swap(roster);
 cameraInfo.fill(0);if(version>=15)for(unsigned i=0;i<11;i++)cameraInfo[i]=u32(b+cameraOffset+i*4);
 unitGroups.fill(0);if(version>=14)for(unsigned i=0;i<10;i++)unitGroups[i]=u32(b+groupsOffset+i*4);
 {unsigned oldFlags=u32(multiplayerHud.data.data()),oldRows=u32(multiplayerHud.data.data()+4),oldKind=u32(multiplayerHud.data.data()+8);
 mapPen.accept(version>=24?b+mapPenOffset:nullptr,seconds());
 multiplayerHud.accept(version>=23?b+multiplayerOffset:nullptr,seconds());
 unsigned oldChatSerial=chat.serial;chat.accept(version>=25?b+chatOffset:nullptr,seconds(),u32(b+12));if(chat.serial!=oldChatSerial)LOG("Chat events serial=%u live=%d own_row=%u",chat.serial,chat.live,chat.ownRow);
 const auto*h=multiplayerHud.data.data();if(oldFlags!=u32(h)||oldRows!=u32(h+4)||oldKind!=u32(h+8))LOG("Multiplayer HUD flags=%u rows=%u kind=%u title=%s time=%s notice=%s",u32(h),u32(h+4),u32(h+8),h+16,h+208,h+272);}
 selectionInfo.fill(0);if(version>=12)memcpy(selectionInfo.data(),b+infoOffset,version>=13?1828:1812);
 if(version>=21){int before=gameSpeed.value;gameSpeed.accept(u32(b+speedOffset),seconds());if(before>=0&&before!=gameSpeed.value)LOG("Game Speed %s index=%d tick=%u",gameSpeed.label(),gameSpeed.value,u32(b+12));}
 snapshotRevision++;modelPoses.swap(poses);modelPosesValid=modelValid;livePalettes.swap(palettes);objects.swap(next);tick=u32(b+12);acknowledge(u32(b+56),u32(b+60));if(version>=9)acceptFormation(b+particleEnd+4,u32(b+particleEnd));return true;
}
// Loopback-only stream with a per-install 128-bit random capability encoded as 32 ASCII bytes.
// Android SELinux prohibits cross-app abstract Unix socket access on this device.
static int listener=-1,peer=-1;static std::array<unsigned char,32> token;static bool authenticated=false;static std::vector<unsigned char> pending;
static void ipc(android_app*a){char path[512];snprintf(path,sizeof(path),"%s/bridge.token",a->activity->internalDataPath);FILE*f=fopen(path,"rb");if(!f||fread(token.data(),1,32,f)!=32){if(f)fclose(f);LOG("IPC disabled: missing capability");return;}fclose(f);listener=socket(AF_INET,SOCK_STREAM|SOCK_NONBLOCK|SOCK_CLOEXEC,0);int yes=1;setsockopt(listener,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes));sockaddr_in addr{};addr.sin_family=AF_INET;addr.sin_port=htons(38434);addr.sin_addr.s_addr=htonl(INADDR_LOOPBACK);if(bind(listener,(sockaddr*)&addr,sizeof(addr))||listen(listener,1)){close(listener);listener=-1;}LOG("IPC loopback listener=%d",listener);}
static void pollIPC(){pollDiagnosticSetting();if(listener<0)return;if(peer<0){peer=accept4(listener,nullptr,nullptr,SOCK_NONBLOCK|SOCK_CLOEXEC);if(peer>=0){authenticated=false;pending.clear();}}if(peer<0)return;unsigned char buf[65536];for(int i=0;i<8;i++){ssize_t n=read(peer,buf,sizeof(buf));if(n>0)pending.insert(pending.end(),buf,buf+n);else{if(n==0){close(peer);peer=-1;}break;}}if(!authenticated){if(pending.size()<32)return;unsigned diff=0;for(unsigned i=0;i<32;i++)diff|=pending[i]^token[i];if(diff){pending.clear();close(peer);peer=-1;LOG("IPC rejected capability");return;}pending.erase(pending.begin(),pending.begin()+32);authenticated=true;LOG("IPC authenticated");}while(pending.size()>=64){unsigned ac=u32(pending.data()+24),oc=u32(pending.data()+28);if(memcmp(pending.data(),"MYTHBR01",8)||ac>400||oc>8192){pending.clear();close(peer);peer=-1;return;}unsigned version=u32(pending.data()+8);if(version<1||version>25){pending.clear();close(peer);peer=-1;return;}size_t n=64+ac*216+oc*80+(version>=2?272:0);if(version>=3){if(pending.size()<n+4)break;unsigned pc=u32(pending.data()+n);if(pc>128){pending.clear();close(peer);peer=-1;return;}n+=4+pc*1028;}if(version>=4){if(pending.size()<n+4)break;unsigned mc=u32(pending.data()+n);if(mc!=0xffffffff&&mc>(version>=22?1024u:512u)){pending.clear();close(peer);peer=-1;return;}n+=4+(mc==0xffffffff?0:mc*(version>=22?40:36));}if(version>=5){if(pending.size()<n+12)break;unsigned tc=u32(pending.data()+n+8);if(tc>4){pending.clear();close(peer);peer=-1;return;}n+=12+tc*surface::PersistentRow;}if(version>=6){if(pending.size()<n+4)break;unsigned lb=u32(pending.data()+n);if(lb<4||lb>262144){pending.clear();close(peer);peer=-1;return;}n+=4+lb;}if(version>=7){if(pending.size()<n+4)break;unsigned pc=u32(pending.data()+n);if(pc>2048){pending.clear();close(peer);peer=-1;return;}n+=4+pc*64;}if(version>=9){if(pending.size()<n+4)break;unsigned bytes=u32(pending.data()+n);if(bytes!=0&&bytes!=(version>=10?4808:4304)){pending.clear();close(peer);peer=-1;return;}n+=4+bytes;}if(version>=11)n+=ac;if(version>=12)n+=1812;if(version>=13)n+=16;if(version>=14)n+=40;if(version>=15)n+=44;if(version>=16)n+=ac*4;if(version>=17){if(pending.size()<n+4)break;unsigned ec=u32(pending.data()+n);if(ec>4096){pending.clear();close(peer);peer=-1;return;}n+=4+ec*(version>=18?28:16);}if(version>=19){if(pending.size()<n+4)break;unsigned hc=u32(pending.data()+n);if(hc>128){pending.clear();close(peer);peer=-1;return;}n+=4+hc*88;}if(version>=20){if(pending.size()<n+4)break;unsigned count=u32(pending.data()+n);if(count>32){pending.clear();close(peer);peer=-1;return;}n+=4+count*44;}if(version>=21)n+=4;if(version>=23)n+=MultiplayerHud::bytes;if(version>=24)n+=MapPen::bytes;if(version>=25)n+=ChatState::bytes;if(pending.size()<n)break;if(!snapshot(pending.data(),n))LOG("IPC invalid frame");else if(!liveSceneReady){liveSceneReady=true;LOG("First live scene ready tick=%u objects=%zu revision=%llu",tick,objects.size(),(unsigned long long)snapshotRevision);}pending.erase(pending.begin(),pending.begin()+n);}if(pending.size()>2097152){pending.clear();close(peer);peer=-1;}}
static GLuint program,whiteTexture;static GLuint unifiedTerrainProgram=0,unifiedMeshProgram=0;static GLuint lightweightProgram=0,billboardProgram=0,billboardPbrProgram=0,modelPbrProgram=0,modelShadowProgram=0;
static GLuint shader(GLenum kind,const char*src){GLuint s=glCreateShader(kind);glShaderSource(s,1,&src,nullptr);glCompileShader(s);GLint ok;glGetShaderiv(s,GL_COMPILE_STATUS,&ok);if(!ok){char b[2048];glGetShaderInfoLog(s,sizeof(b),nullptr,b);LOG("shader %s",b);throw std::runtime_error("shader");}return s;}
static std::vector<rendercull::Range> terrainRanges;
static unsigned terrainViewTested=0,terrainViewVisible=0,terrainViewDraws=0;
static GLuint vao,vbo,terrainVBO;static GLsizei terrainCount;static std::vector<GLuint> textures;static std::map<uint64_t,Quad> quads;
static uint64_t key(unsigned c,unsigned s,unsigned f,unsigned d){return ((uint64_t)c<<48)|((uint64_t)s<<32)|((uint64_t)f<<16)|d;}
#include "death_sprite_metadata.h"
static bool physicalDeathSequence[256][256]{};
static void loadPhysicalDeathSequences(android_app*a){
 memset(physicalDeathSequence,0,sizeof(physicalDeathSequence));auto b=asset(a,"collections.bin");
 bool tagged=b.size()>=8&&!memcmp(b.data(),"MVRCL002",8);size_t header=tagged?12:4,stride=tagged?32:28;
 if(b.size()<header)throw std::runtime_error("death collection header");unsigned n=u32(b.data()+header-4);
 if(n>255||b.size()!=header+n*stride)throw std::runtime_error("death collection bounds");
 unsigned matched=0;for(unsigned i=0;i<n;i++){auto row=b.data()+header+i*stride;unsigned col=u32(row);if(col>=255)throw std::runtime_error("death collection slot");
  for(auto&entry:deathSpriteMetadata){bool same=true;for(int k=0;k<6;k++)if(u32(row+4+k*4)!=entry[k]){same=false;break;}
   if(same&&entry[6]<256){physicalDeathSequence[col][entry[6]]=true;matched++;}}
 }LOG("Physical death sprite sequences=%u",matched);
}
static bool physicalProjectileSprite(const Object&o){
 if(o.objectType!=3)return false;
 auto it=quads.find(key(o.col,o.seq,o.frame,0));
 return it!=quads.end()&&(!blendedSprite(it->second)||(o.col<256&&o.seq<256&&physicalDeathSequence[o.col][o.seq]));
}

static const float scale=1.f;
struct UnitFrameBounds{float left,right,bottom,top;};
static std::map<uint64_t,UnitFrameBounds> unitFrameBounds;
static UnitFrameBounds fixedUnitFrame(const Object&o,const Quad&fallback){
 if(o.frameRadius>0&&o.frameTop>o.frameBottom)return {-o.frameRadius*scale,o.frameRadius*scale,o.frameBottom*scale,o.frameTop*scale};
 if(!o.unitID){float ps=fallback.scale*(10.f/512.f)*scale*o.scale;return {-fallback.rx*ps,(fallback.w-fallback.rx)*ps,(fallback.ry-fallback.h)*ps,fallback.ry*ps};}
 uint64_t id=((uint64_t)o.col<<32)|o.unitID;
 auto found=unitFrameBounds.find(id);if(found!=unitFrameBounds.end())return found->second;
 // Direction zero is front. Freeze the first sequence's first frame, independent of animation.
 auto first=quads.find(key(o.col,o.seq,0,0));const Quad&q=first!=quads.end()?first->second:fallback;
 float ps=q.scale*(10.f/512.f)*scale*o.scale;
 UnitFrameBounds bounds{-q.rx*ps,(q.w-q.rx)*ps,(q.ry-q.h)*ps,q.ry*ps};
 unitFrameBounds[id]=bounds;return bounds;
}
#include "sun_angle_adjustment.h"
#include "touch_input.h"
#include "projectile_math.h"
struct IndexedSprite{unsigned w,h;std::vector<unsigned char> pairs;};
static std::vector<IndexedSprite> spriteIndices;
struct ColoredSprite{GLuint texture;std::array<unsigned char,1024> palette;uint64_t used=0;};
static uint64_t colorUse=0;
static void releaseSpriteMaterial(GLuint);
static void trimColors(std::map<uint64_t,ColoredSprite>&cache){while(cache.size()>1024){auto oldest=std::min_element(cache.begin(),cache.end(),[](const auto&a,const auto&b){return a.second.used<b.second.used;});releaseSpriteMaterial(oldest->second.texture);glDeleteTextures(1,&oldest->second.texture);cache.erase(oldest);}}
static std::map<uint64_t,ColoredSprite> coloredSprites;
static void clearColoredSprites(){for(auto&x:coloredSprites)glDeleteTextures(1,&x.second.texture);coloredSprites.clear();}
static void loadSpriteIndices(android_app*a){auto data=asset(a,"sprite-indices.pack");if(data.size()<12||memcmp(data.data(),"MYTHIX01",8)||u32(data.data()+8)!=textures.size())throw std::runtime_error("index header");size_t p=12;spriteIndices.clear();for(unsigned i=0;i<textures.size();i++){if(p+8>data.size())throw std::runtime_error("index size");unsigned w=u32(data.data()+p),h=u32(data.data()+p+4);p+=8;if(w>4096||h>4096||size_t(w)*h*2>data.size()-p)throw std::runtime_error("index bounds");size_t n=size_t(w)*h*2;spriteIndices.push_back({w,h,std::vector<unsigned char>(data.begin()+p,data.begin()+p+n)});p+=n;}if(p!=data.size())throw std::runtime_error("index trailing data");coloredSprites.clear();}
static void inheritSpriteMaterial(GLuint,GLuint);
static void applySpriteAlbedo(GLuint,unsigned,unsigned,std::vector<unsigned char>&);
static void atlasTextureChanged(GLuint,unsigned,unsigned,unsigned);
static GLuint coloredTexture(const Object&o,unsigned texture){
 auto p=livePalettes.find(o.col|(o.palette<<8));if(p==livePalettes.end()||texture>=spriteIndices.size())return textures[texture];
 auto&src=spriteIndices[texture];if(!src.w||!src.h)return textures[texture];uint64_t key=(uint64_t(p->first)<<32)|texture;auto found=coloredSprites.find(key);
 if(found!=coloredSprites.end()&&found->second.palette==p->second){found->second.used=++colorUse;return found->second.texture;}
 GLuint tx;if(found==coloredSprites.end())glGenTextures(1,&tx);else tx=found->second.texture;
 std::vector<unsigned char> rgba(src.w*src.h*4);for(size_t i=0;i<src.pairs.size()/2;i++){memcpy(rgba.data()+i*4,p->second.data()+src.pairs[i*2]*4,3);rgba[i*4+3]=src.pairs[i*2+1];}
 applySpriteAlbedo(textures[texture],src.w,src.h,rgba);glBindTexture(GL_TEXTURE_2D,tx);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,src.w,src.h,0,GL_RGBA,GL_UNSIGNED_BYTE,rgba.data());glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);inheritSpriteMaterial(tx,textures[texture]);atlasTextureChanged(tx,src.w,src.h,o.col);spriteUploads++;coloredSprites[key]={tx,p->second,++colorUse};return tx;
}
#include "sprite_pbr.h"
// Lighting owners are one-based object indices; overlays inherit the body owner.
static bool spritePbrCategoryEnabled(unsigned owner){
 if(!owner||owner>objects.size())return false;
 const auto&o=objects[owner-1];
 bool unit=o.unitID!=0||o.objectType==1||o.objectType==4;
 return unit?sprite_pbr::units:sprite_pbr::sceneObjects;
}
#include "sprite_material.h"
#include "sprite_atlas.h"
static bool selectSpriteProgram(bool pbr);
#include "hd_stream.h"
static void atlasTextureChanged(GLuint tx,unsigned w,unsigned h,unsigned col){sprite_atlas::registerTexture(tx,w,h,col);sprite_atlas::changed(tx);}
static void releaseSpriteMaterial(GLuint tx){sprite_atlas::forget(tx);sprite_material::bindings.erase(tx);}
static void applySpriteAlbedo(GLuint tx,unsigned w,unsigned h,std::vector<unsigned char>&rgba){sprite_material::applyAlbedo(sprite_material::get(tx),w,h,rgba);}
static void inheritSpriteMaterial(GLuint dst,GLuint src){sprite_material::inherit(dst,src);}
static sprite_material::Binding particleMaterial(unsigned);
static GLuint hdQuad(Quad&q,unsigned palette,unsigned kind=0){Quad original=q;uint64_t id=(uint64_t(kind)<<32)|q.texture;if(q.flags&1){auto mirror=id|(1u<<30);if(hd::entries.count(mirror))id=mirror;}auto h=liveHues.find(palette);GLuint tx=hd::request(id,h==liveHues.end()?nullptr:&h->second);if(!tx)return 0;auto&m=hd::entries.at(id).meta;float factor=hd::be32(m.data()+96)/65536.f;if(!std::isfinite(factor)||factor<=0||factor>16)return 0;q.scale*=factor;q.w=hd::be16(m.data()+80);q.h=hd::be16(m.data()+82);q.rx=int16_t(hd::be16(m.data()+92));q.ry=int16_t(hd::be16(m.data()+94));if(q.flags&1){if(id&(1u<<30))q.flags&=~1u;else q.rx=q.w-q.rx;}
 if(kind==0||kind==3){auto b=kind==0&&original.texture<textures.size()?sprite_material::get(textures[original.texture]):sprite_material::Binding{};
 if(kind==3)b=particleMaterial(original.texture);
 if(hd::preferStandardAlbedo(id,b)){q=original;return 0;}
 auto hdMaterial=hd::materials.find(id);
 if(hdMaterial!=hd::materials.end()&&hdMaterial->second.texture)b=hdMaterial->second;
 else if(b.texture){float sign=(original.flags&1)?-1.f:1.f,hsign=(q.flags&1)?-1.f:1.f;float ratio=q.scale/original.scale;
 b.sx=sign*hsign*q.w*ratio/original.w;b.ox=(original.flags&1?1.f:0.f)+sign*((q.flags&1?q.w:0.f)-q.rx)*ratio/original.w+sign*original.rx/original.w;
 float vs=(original.flags&2)?-1.f:1.f,hvs=(q.flags&2)?-1.f:1.f;b.sy=vs*hvs*q.h*ratio/original.h;b.oy=(original.flags&2?1.f:0.f)+vs*((q.flags&2?q.h:0.f)-q.ry)*ratio/original.h+vs*original.ry/original.h;}
 sprite_material::assign(tx,b);sprite_atlas::registerTexture(tx,unsigned(q.w),unsigned(q.h),q.col,true);}
 return tx;}
static GLuint hdSprite(const Object&o,Quad&q){return hdQuad(q,o.col|(o.palette<<8));}
static GLuint hdModel(GLuint tx){auto i=hd::modelKeys.find(tx);if(i==hd::modelKeys.end())return tx;GLuint hi=hd::request(i->second);if(!hi)return tx;
 auto material=hd::materials.find(i->second);
 // A material inherited from the standard bitmap does not mean the HD colour
 // bitmap received its albedo edit. Keep the edited standard image until an
 // independently matched unshadowed HD bitmap is available.
 if(hd::preferStandardAlbedo(i->second,sprite_material::get(tx)))return tx;
 sprite_material::assign(hi,material!=hd::materials.end()&&material->second.texture?material->second:sprite_material::get(tx));
 return hi;}
#include "fog_render.h"
#include "detail_terrain.h"
#include "terrain_normals.h"
static void geometry(android_app*a){loadPhysicalDeathSequences(a);sprite_material::load(a->activity->externalDataPath);hd::load(a);loadFog(a);loadDetailTerrain(a);auto makeProgram=[&](bool shadows,bool lighting,bool atlas,bool pbr=false,bool model=false,bool farShadow=false,bool unified=false,bool unifiedMesh=false){std::string vs=model?modelShaderSource(mythVertexShader,shadows,lighting):spriteShaderSource(mythVertexShader,shadows,lighting,atlas,pbr),fs=scene_correction::fragment(farShadow?modelShadowLod(spriteShaderSource(mythFragmentShader,false,false,false),true):model?modelShaderSource(mythFragmentShader,shadows,lighting):spriteShaderSource(mythFragmentShader,shadows,lighting,atlas,pbr),scene_correction::enabled);if(unified)fs=unifiedTerrainShader(fs,unifiedMesh);GLuint p=glCreateProgram(),v=shader(GL_VERTEX_SHADER,vs.c_str()),f=shader(GL_FRAGMENT_SHADER,fs.c_str());glAttachShader(p,v);glAttachShader(p,f);glLinkProgram(p);glDeleteShader(v);glDeleteShader(f);GLint ok;glGetProgramiv(p,GL_LINK_STATUS,&ok);if(!ok)throw std::runtime_error("scene shader program");glUseProgram(p);glUniform1i(glGetUniformLocation(p,"tex"),0);glUniform1i(glGetUniformLocation(p,"detailTex"),1);glUniform4f(glGetUniformLocation(p,"tint"),1,1,1,1);return p;};lightweightProgram=makeProgram(false,false,false);unifiedTerrainProgram=unifiedGroundShadows()?makeProgram(false,false,false,false,false,false,true):lightweightProgram;unifiedMeshProgram=unifiedGroundShadows()?makeProgram(false,false,false,false,false,false,true,true):lightweightProgram;billboardProgram=(enhancedSpritePath||sprite_atlas::enabled)?makeProgram(billboardShaderShadows,billboardShaderLighting,sprite_atlas::enabled):lightweightProgram;billboardPbrProgram=(sprite_pbr::enabled()&&(sprite_pbr::units||sprite_pbr::sceneObjects))?makeProgram(billboardShaderShadows,billboardShaderLighting,sprite_atlas::enabled,true):billboardProgram;modelPbrProgram=(enhancedSpritePath&&sprite_pbr::meshes)?makeProgram(billboardShaderShadows,billboardShaderLighting,false,true,true):lightweightProgram;modelShadowProgram=(sprite_pbr::meshes&&billboardShaderShadows)?makeProgram(false,false,false,false,false,true):lightweightProgram;program=lightweightProgram;glUseProgram(program);LOG("Mesh PBR enabled=%d program=%u",sprite_pbr::meshes,modelPbrProgram);LOG("Scene shaders terrain=lightweight mesh=lightweight billboard_shadows=%d billboard_lighting=%d light_program=%u billboard_program=%u",billboardShaderShadows,billboardShaderLighting,lightweightProgram,billboardProgram);glGenTextures(1,&whiteTexture);glBindTexture(GL_TEXTURE_2D,whiteTexture);unsigned char white[]={255,255,255,255};glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,white);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);glGenVertexArrays(1,&vao);glBindVertexArray(vao);glGenBuffers(1,&vbo);glGenBuffers(1,&terrainVBO);
 auto b=asset(a,"scene.pack");size_t p=0;auto take=[&](size_t n){if(n>b.size()-p)throw std::runtime_error("pack bounds");auto r=b.data()+p;p+=n;return r;};if(memcmp(take(8),"MYTHVR01",8))throw std::runtime_error("pack magic");auto hdr=take(16);unsigned w=u32(hdr),h=u32(hdr+4),nt=u32(hdr+8),nq=u32(hdr+12);if(w!=levelWidth||h!=levelHeight||nt>20000||nq>1000000)throw std::runtime_error("pack header");auto heights=take(w*h*2);terrain.width=w;terrain.height=h;terrain.ox=originX;terrain.oy=originY;terrain.oz=originZ;terrain.scale=scale;terrain.heights.resize(w*h);for(unsigned i=0;i<w*h;i++)terrain.heights[i]=i16(heights+i*2)/512.f;terrain.rebuildRayCeiling();buildTerrainNormals(w,h,terrain.heights);std::vector<Vertex> terrain;auto vert=[&](unsigned x,unsigned y){return Vertex{mythWorldX(x,originX,scale),(i16(heights+2*(y*w+x))/512.f-originZ)*scale,(y-originY)*scale,x/(float)w,y/(float)h};};terrainRanges.clear();for(unsigned by=0;by<h-1;by+=16)for(unsigned bx=0;bx<w-1;bx+=16){rendercull::Range range;range.first=terrain.size();for(unsigned y=by;y<std::min(by+16,h-1);y++)for(unsigned x=bx;x<std::min(bx+16,w-1);x++){auto a0=vert(x,y),b0=vert(x+1,y),c0=vert(x,y+1),d0=vert(x+1,y+1);if((x^y)&1)terrain.insert(terrain.end(),{a0,b0,c0,b0,d0,c0});else terrain.insert(terrain.end(),{a0,b0,d0,a0,d0,c0});}range.count=terrain.size()-range.first;for(size_t i=range.first;i<terrain.size();i++)range.bounds.add(terrain[i].x,terrain[i].y,terrain[i].z);terrainRanges.push_back(range);}buildDetailTerrain(terrain);terrainCount=terrain.size();glBindBuffer(GL_ARRAY_BUFFER,terrainVBO);glBufferData(GL_ARRAY_BUFFER,terrain.size()*sizeof(Vertex),terrain.data(),GL_STATIC_DRAW);
 textures.resize(nt);glGenTextures(nt,textures.data());for(unsigned i=0;i<nt;i++){pumpLoadingEvents();auto sz=take(8);unsigned tw=u32(sz),th=u32(sz+4);if(!tw||!th||tw>4096||th>4096)throw std::runtime_error("texture dimensions");auto pixels=take(tw*th*4);sprite_material::assign(textures[i],sprite_material::match(tw,th,pixels));std::vector<unsigned char> albedo(pixels,pixels+size_t(tw)*th*4);applySpriteAlbedo(textures[i],tw,th,albedo);glBindTexture(GL_TEXTURE_2D,textures[i]);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,tw,th,0,GL_RGBA,GL_UNSIGNED_BYTE,albedo.data());glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);}
 for(unsigned i=0;i<nq;i++){Quad q;memcpy(&q,take(sizeof(q)),sizeof(q));if(q.texture>=nt)throw std::runtime_error("texture reference");quads[key(q.col,q.seq,q.frame,q.dir)]=q;}
 LOG("Scene assets ready terrain=%d textures=%u directionFrames=%u; waiting for live scene",terrainCount,nt,nq);
}
static void attributes(GLuint buffer){glDisableVertexAttribArray(4);glVertexAttrib1f(4,0);glBindBuffer(GL_ARRAY_BUFFER,buffer);glEnableVertexAttribArray(0);glEnableVertexAttribArray(1);glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)0);glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)12);}
static void uiAccent(float alpha=1){glUniform4f(glGetUniformLocation(program,"tint"),ui_contrast::red(),ui_contrast::green(),ui_contrast::blue(),alpha);}
#include "radial_draw.h"
#include "selection_draw.h"
#include "wrist_map.h"
#include "coop_transfer_draw.h"
static GLuint groundAttackTexture=0;
static uint64_t rendererGeneration=0;
static void loadGroundAttackMarker(android_app*a){
 auto b=asset(a,"ground-attack-marker.rgba");if(b.size()<8)throw std::runtime_error("ground marker header");unsigned w=u32(b.data()),h=u32(b.data()+4);if(!w||!h||w>1024||h>1024||b.size()!=8+size_t(w)*h*4)throw std::runtime_error("ground marker dimensions");
 glGenTextures(1,&groundAttackTexture);glBindTexture(GL_TEXTURE_2D,groundAttackTexture);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,b.data()+8);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
}
static GLuint healthTexture=0;
static void drawHealthBars(const std::vector<Vertex>& vertices){
 if(vertices.empty())return;
 if(!healthTexture){unsigned char pixels[256*2*4];for(int y=0;y<2;y++)for(int x=0;x<256;x++){int i=(y*256+x)*4;pixels[i]=255-x;pixels[i+1]=x;pixels[i+2]=0;pixels[i+3]=y?46:166;}glGenTextures(1,&healthTexture);glBindTexture(GL_TEXTURE_2D,healthTexture);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,256,2,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);}
 glBindTexture(GL_TEXTURE_2D,healthTexture);attributes(vbo);glUniform4f(glGetUniformLocation(program,"tint"),1,1,1,1);glUniform1f(glGetUniformLocation(program,"alphaCutoff"),.003f);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glDepthMask(GL_FALSE);
 glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(Vertex),vertices.data(),GL_STREAM_DRAW);glDrawArrays(GL_TRIANGLES,0,vertices.size());
 glDepthMask(GL_TRUE);glDisable(GL_BLEND);glUniform1f(glGetUniformLocation(program,"alphaCutoff"),.5f);glBindTexture(GL_TEXTURE_2D,whiteTexture);
}
static void drawControls(const std::vector<Vertex>& selectionFrames,const std::vector<Vertex>& friendlyHover,const std::vector<Vertex>& enemyHover,const std::vector<Vertex>& itemHover,const std::vector<Vertex>& healthBars,const std::vector<Vertex>& manaBars){
 glBindTexture(GL_TEXTURE_2D,whiteTexture);attributes(vbo);
 auto mesh=[&](const std::vector<Vertex>& vertices){if(vertices.empty())return;glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(Vertex),vertices.data(),GL_STREAM_DRAW);glDrawArrays(GL_TRIANGLES,0,vertices.size());};
 glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glDepthMask(GL_FALSE);
 glUniform4f(glGetUniformLocation(program,"tint"),1,.05f,.05f,.55f);mesh(enemyHover);
 uiAccent(.55f);mesh(friendlyHover);
 glUniform4f(glGetUniformLocation(program,"tint"),0,1,.1f,.55f);mesh(itemHover);
 glDepthMask(GL_TRUE);glDisable(GL_BLEND);
 uiAccent(1);mesh(selectionFrames);
 glUniform4f(glGetUniformLocation(program,"tint"),.1f,.22f,1.f,1.f);mesh(manaBars);
 drawHealthBars(healthBars);
 bool movePreview=selectionGesture.pending&&pressWaypoint<0&&movePressValid&&!selectionGesture.box;
 if(!selectionGesture.box&&(movePreview||routePoints||seconds()<patrolFlashUntil||(lastMoveTime>0&&seconds()-lastMoveTime<(lastMoveGroundAttack?3.0:.5)))){
  Vec3 destination=movePreview?movePressPoint:lastMove;float diameter=movePreview?movePressDiameter:lastMoveDiameter;
  std::vector<Vertex> marker;
  bool attack=movePreview?pressGroundAttack:lastMoveGroundAttack;
  if(attack){
   glBindTexture(GL_TEXTURE_2D,groundAttackTexture);glUniform4f(glGetUniformLocation(program,"tint"),1,1,1,1);glUniform1f(glGetUniformLocation(program,"alphaCutoff"),.003f);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glDepthMask(GL_FALSE);
   auto point=[&](int i,int j){float u=i/8.f,v=j/8.f,x=destination.x+(u-.5f)*diameter,z=destination.z+(v-.5f)*diameter;return Vertex{x,terrain.elevation(x,z)+.04f,z,u,v};};
   for(int j=0;j<8;j++)for(int i=0;i<8;i++){auto a=point(i,j),b=point(i+1,j),c=point(i,j+1),d=point(i+1,j+1);marker.insert(marker.end(),{a,b,c,b,d,c});}mesh(marker);
   glDepthMask(GL_TRUE);glDisable(GL_BLEND);glUniform1f(glGetUniformLocation(program,"alphaCutoff"),.5f);glBindTexture(GL_TEXTURE_2D,whiteTexture);
  }else{
   // World-space marker geometry is immutable until the command/preview changes.
   // Share one GPU buffer between both eyes and all subsequent frames.
   static GLuint markerBuffer=0,blockedBuffer=0;static GLsizei markerCount=0,blockedCount=0;
   static uint64_t cachedRevision=~uint64_t(0);static bool cachedPreview=false;static unsigned cachedRoute=~0u;static Vec3 cachedPoint{};
   static uint64_t markerGeneration=~uint64_t(0);if(markerGeneration!=rendererGeneration){markerGeneration=rendererGeneration;markerBuffer=blockedBuffer=0;markerCount=blockedCount=0;cachedRevision=~uint64_t(0);cachedPreview=false;}
   if(!markerBuffer){glGenBuffers(1,&markerBuffer);glGenBuffers(1,&blockedBuffer);cachedRevision=~uint64_t(0);}
   if(cachedRoute!=routePoints||cachedRevision!=moveMarkerRevision||cachedPreview!=movePreview||(movePreview&&(cachedPoint.x!=movePressPoint.x||cachedPoint.y!=movePressPoint.y||cachedPoint.z!=movePressPoint.z))){

   std::vector<Vertex> redMarker;
   for(size_t index=0;index<formationMarkerPoints;index++)for(size_t unitIndex=0;unitIndex<formationMarkerUnits;unitIndex++){
    const auto&unit=moveMarkerUnits[unitIndex];Vec3 center=formationMarkers[index*formationMarkerUnits+unitIndex];bool blocked=formationBlocked[index*formationMarkerUnits+unitIndex]!=0;bool dotMarker=!blocked&&(index+1<formationMarkerPoints||((routePoints||seconds()<patrolFlashUntil)&&!movePreview));auto&vertices=blocked?redMarker:marker;
    float radius=unit.diameter*(dotMarker?.25f:.5f),halfWidth=std::clamp(unit.diameter*.025f,.012f,.035f)*pointer_style::scale;
    auto point=[&](float angle,float r){float x=center.x+cosf(angle)*r,z=center.z+sinf(angle)*r;return Vertex{x,terrain.elevation(x,z)+.04f,z,0,0};};
    for(int i=0;i<32;i++){float a=i*6.2831853f/32,b=(i+1)*6.2831853f/32;
     if(dotMarker){vertices.insert(vertices.end(),{point(0,0),point(a,radius),point(b,radius)});}
     else{auto p=point(a,radius-halfWidth),q=point(a,radius+halfWidth),r=point(b,radius-halfWidth),t=point(b,radius+halfWidth);vertices.insert(vertices.end(),{p,q,r,q,t,r});}
    }
    if(blocked){
     // Diagonal bar follows the terrain, matching the ring's line thickness.
     auto slash=[&](float t,float side){float x=center.x+(t+side)*.70710678f,z=center.z+(t-side)*.70710678f;return Vertex{x,terrain.elevation(x,z)+.041f,z,0,0};};
     for(int i=0;i<8;i++){float a=-radius+i*(2*radius/8),b=-radius+(i+1)*(2*radius/8);auto p=slash(a,-halfWidth),q=slash(a,halfWidth),r=slash(b,-halfWidth),t=slash(b,halfWidth);vertices.insert(vertices.end(),{p,q,r,q,t,r});}
    }
   }

    attributes(markerBuffer);glBufferData(GL_ARRAY_BUFFER,marker.size()*sizeof(Vertex),marker.data(),GL_STATIC_DRAW);markerCount=marker.size();attributes(blockedBuffer);glBufferData(GL_ARRAY_BUFFER,redMarker.size()*sizeof(Vertex),redMarker.data(),GL_STATIC_DRAW);blockedCount=redMarker.size();
    cachedRoute=routePoints;cachedRevision=moveMarkerRevision;cachedPreview=movePreview;cachedPoint=movePressPoint;markerBuilds++;
   }
   uiAccent(1);attributes(markerBuffer);glDrawArrays(GL_TRIANGLES,0,markerCount);glUniform4f(glGetUniformLocation(program,"tint"),1,.03f,.03f,1);attributes(blockedBuffer);glDrawArrays(GL_TRIANGLES,0,blockedCount);uiAccent(1);attributes(vbo);
  }
 }
 if(seconds()<patrolFlashUntil){
  std::vector<Vertex> flash;float r=patrolFlashDiameter*.5f,w=std::clamp(patrolFlashDiameter*.025f,.012f,.035f);
  auto p=[&](float a,float radius){float x=patrolFlashPoint.x+cosf(a)*radius,z=patrolFlashPoint.z+sinf(a)*radius;return Vertex{x,terrain.elevation(x,z)+.045f,z,0,0};};
  for(int i=0;i<32;i++){float a=i*6.2831853f/32,b=(i+1)*6.2831853f/32;auto x=p(a,r-w),y=p(a,r+w),z=p(b,r-w),q=p(b,r+w);flash.insert(flash.end(),{x,y,z,y,q,z});}
  uiAccent(1);mesh(flash);
 }
 std::vector<Vertex> lines;auto line=[&](Vec3 a,Vec3 b){lines.push_back({a.x,a.y,a.z,0,0});lines.push_back({b.x,b.y,b.z,0,0});};
 if(movePreview&&moveFacing&&!pressGroundAttack&&formationMarkerPoints){
  float angle=(moveFacing&65535)*(6.28318530718f/65536.f);Vec3 forward{-cosf(angle),0,sinf(angle)},right{-forward.z,0,forward.x};
  std::vector<Vertex> arrows;
  auto ground=[&](Vec3 p){return Vertex{p.x,terrain.elevation(p.x,p.z)+.055f,p.z,0,0};};
  for(unsigned i=0;i<formationMarkerUnits;i++){
   Vec3 center=formationMarkers[(formationMarkerPoints-1)*formationMarkerUnits+i];float diameter=moveMarkerUnits[i].diameter;
   Vec3 start=add(center,times(forward,diameter*.5f)),tip=add(start,times(forward,std::max(.4f,diameter*.6f)));
   // A narrow solid shaft and triangle head remain legible in VR.
   Vec3 neck=sub(tip,times(forward,.18f));float half=.015f*pointer_style::scale;
   auto a=ground(add(start,times(right,half))),b=ground(sub(start,times(right,half))),c=ground(add(neck,times(right,half))),d=ground(sub(neck,times(right,half)));
   arrows.insert(arrows.end(),{a,b,c,b,d,c,ground(tip),ground(add(neck,times(right,.10f*pointer_style::scale))),ground(sub(neck,times(right,.10f*pointer_style::scale)))});
  }
  uiAccent(1);mesh(arrows);
 }
 if(pointerValid&&!(cameraSpeeds.swapHands?chatLeftHit:chatRightHit)&&!coopMenuOpen&&!mapHovered&&!mapGesture&&!infoPanelHovered&&!infoPanelGesture&&!originalDialogMode){line(pointerStart,pointerEnd);if(!hoverID&&!hoverItem)for(int i=0;i<32;i++){float a=i*6.2831853f/32,b=(i+1)*6.2831853f/32;line({pointerEnd.x+cosf(a)*.08f,pointerEnd.y+.04f,pointerEnd.z+sinf(a)*.08f},{pointerEnd.x+cosf(b)*.08f,pointerEnd.y+.04f,pointerEnd.z+sinf(b)*.08f});}
  if(hoverItem&&pickupGreen())glUniform4f(glGetUniformLocation(program,"tint"),0,1,.1f,1);else if(hoverID){if(hoverFriendly)uiAccent();else glUniform4f(glGetUniformLocation(program,"tint"),1,.05f,.05f,1);}else if(commandFailed)glUniform4f(glGetUniformLocation(program,"tint"),1,.15f,.1f,1);else if(groundValid||chat.open)uiAccent(1);else glUniform4f(glGetUniformLocation(program,"tint"),1,1,1,1);
  pointer_style::Lines width;glBufferData(GL_ARRAY_BUFFER,lines.size()*sizeof(Vertex),lines.data(),GL_STREAM_DRAW);glDrawArrays(GL_LINES,0,lines.size());
 }glUniform4f(glGetUniformLocation(program,"tint"),1,1,1,1);
}
struct ModelBatch{GLuint texture,buffer;GLsizei count;rendercull::Bounds bounds;};static std::vector<ModelBatch> models,animatedFallback,normalModels,normalFallback;
static void bindModelMaterial(GLuint texture,const rendercull::Bounds&bounds,bool moving);
static bool modelColorPass=false;
#include "animated_models.h"
#include "model_appearance.h"
struct ConnectorPart{ModelBatch batch;std::array<int32_t,4> endpoints;std::vector<Vertex> vertices;float top,rise;};static std::vector<ConnectorPart> connectorParts;
static void loadModels(android_app*a){
 modelAppearances.clear();appearancePixels.clear();
 auto cb=asset(a,"connectors.pack");std::map<unsigned,std::array<int32_t,4>> cr;std::map<std::array<int32_t,4>,std::pair<float,float>> cs;connectorParts.clear();if(cb.size()>=4){unsigned n=u32(cb.data());if(n>4096||cb.size()!=4+n*32)throw std::runtime_error("connector pack");for(unsigned i=0;i<n;i++){auto r=cb.data()+4+i*32;std::array<int32_t,4> e;memcpy(e.data(),r+8,16);cs[e]={i32(r+24)/512.f,i32(r+28)/512.f};for(unsigned j=0;j<u32(r+4);j++)cr[u32(r)+j]=e;}}
auto b=asset(a,"models.pack");size_t p=0;auto take=[&](size_t n){if(n>b.size()-p)throw std::runtime_error("model bounds");auto r=b.data()+p;p+=n;return r;};auto magic=take(8);bool identities=!memcmp(magic,"MYTHMD03",8);bool appearances=identities||!memcmp(magic,"MYTHMD02",8);if(!appearances&&memcmp(magic,"MYTHMD01",8))throw std::runtime_error("model magic");auto h=take(8);unsigned nt=u32(h),nc=u32(h+4);if(nt>4096||nc>1000000)throw std::runtime_error("model sizes");std::vector<AppearancePixels> appearanceSource(nt);std::vector<GLuint> tx(nt);glGenTextures(nt,tx.data());for(unsigned i=0;i<nt;i++)hd::modelKeys[tx[i]]=(uint64_t(1)<<32)|i;for(unsigned i=0;i<nt;i++){auto wh=take(8);unsigned w=u32(wh),h=u32(wh+4);if(w>4096||h>4096)throw std::runtime_error("model texture");const unsigned char*identity=identities?take(32):nullptr;auto pixels=take(w*h*4);auto material=identity?sprite_material::matchIdentity(w,h,identity):sprite_material::match(w,h,pixels);sprite_material::assign(tx[i],material);if(material.packed)LOG("Model PBR static texture=%u size=%ux%u matched=1",i,w,h);std::vector<uint8_t> rgba(pixels,pixels+w*h*4);sprite_material::applyAlbedo(material,w,h,rgba);extendAlphaEdges(rgba,w,h,true);if(appearances)appearanceSource[i]={w,h,rgba,material};glBindTexture(GL_TEXTURE_2D,tx[i]);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,rgba.data());glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);}auto appearanceOverrides=appearances?loadModelAppearances(b,p+size_t(nc)*64,nc,tx,appearanceSource):std::map<unsigned,unsigned>{};std::map<std::array<int32_t,4>,std::map<unsigned,std::vector<Vertex>>> connectorGeometry;std::vector<Vertex> selectionFrames;std::map<unsigned,std::vector<Vertex>> batches,fallback;for(unsigned i=0;i<nc;i++){auto r=take(64);unsigned txid=appearanceOverrides.count(i)?appearanceOverrides.at(i):u32(r);if(txid>=tx.size())throw std::runtime_error("model texture index");for(int j=0;j<3;j++){float v[5];memcpy(v,r+4+j*20,20);if(!cr.count(i))std::swap(v[3],v[4]);(cr.count(i)?connectorGeometry[cr[i]]:(isAnimatedTriangle(i)?fallback:batches))[txid].push_back({mythWorldX(v[0],originX,scale),(v[2]-originZ)*scale,(v[1]-originY)*scale,v[3],v[4]});}}auto upload=[&](const std::map<unsigned,std::vector<Vertex>>&source,std::vector<ModelBatch>&target,bool split=true){
 target.clear();for(auto&item:source){std::map<std::pair<int,int>,std::vector<Vertex>> cells;for(size_t j=0;j+2<item.second.size();j+=3){auto&a=item.second[j];auto&b=item.second[j+1];auto&c=item.second[j+2];auto cell=std::make_pair(split?(int)floorf((a.x+b.x+c.x)/48.f):0,split?(int)floorf((a.z+b.z+c.z)/48.f):0);cells[cell].insert(cells[cell].end(),{a,b,c});}
 for(auto&cell:cells){ModelBatch batch{tx[item.first],0,(GLsizei)cell.second.size(),{}};for(auto&v:cell.second)batch.bounds.add(v.x,v.y,v.z);glGenBuffers(1,&batch.buffer);glBindBuffer(GL_ARRAY_BUFFER,batch.buffer);glBufferData(GL_ARRAY_BUFFER,cell.second.size()*sizeof(Vertex),cell.second.data(),GL_STATIC_DRAW);target.push_back(batch);}}};for(auto&part:connectorGeometry){std::vector<ModelBatch> temp;upload(part.second,temp,false);for(auto&v:temp)connectorParts.push_back({v,part.first,part.second.begin()->second,cs[part.first].first,cs[part.first].second});}upload(batches,models);upload(fallback,animatedFallback);upload(batches,normalModels,false);upload(fallback,normalFallback,false);LOG("Static models triangles=%u textures=%u batches=%zu (animated models first pose)",nc,nt,models.size());}
#include "surface_render.h"
static XrVector3f shadowLodEye{};
#include "particle_render.h"
#include "reflection_render.h"
#include "water_light.h"
#include "sprite_wait_profile.h"
#include "sprite_api_profile.h"
#include "local_lighting.h"
#include "sun_shadows.h"
#include "render_profile.h"
#include "sprite_stream.h"
#include "sprite_cache.h"
#include "scene_fade.h"
#include "skybox_render.h"
#include "skybox_load.h"
#include "weather_render.h"
// New VR Activity creates a new EGL context: discard stale names and per-map state.
static void resetMapRenderer(){
 program=lightweightProgram=billboardProgram=billboardPbrProgram=0,modelPbrProgram=0;resetSunShadows();rendererGeneration++;soundRangeApplied=0;soundRangePoll=0;healthTexture=0;weatherProgram=0;weatherStream=WeatherStream{};weatherReady=false;
 textures.clear();quads.clear();unitFrameBounds.clear();coloredSprites.clear();spriteIndices.clear();
 models.clear();normalModels.clear();animatedFallback.clear();normalFallback.clear();animatedShapes.clear();animatedRanges.clear();
 particleTextures.clear();particleQuads.clear();particleColors.clear();spriteViews[0]=SpriteViewCache{};spriteViews[1]=SpriteViewCache{};reflectionCandidates=ReflectionCandidateFrame{};reflectionCandidateFrames=reflectionCandidateTests=reflectionCandidateSelected=reflectionUnionUploaded=0;reflectionIndexCPU=reflectionSelectCPU=reflectionVertexUploadCPU=reflectionElementUploadCPU=0;streamIndexUploads=streamIndexBytes=streamIndexReuses=0;reflectionGeometryBuilds=reflectionGeometryReuses=0;
 reflectionFBO=reflectionTexture=reflectionDepth=reflectionWaterVBO=reflectionFlowVBO=reflectionProgram=0;reflectionW=reflectionH=0;reflectionReady=false;reflectionPlanes.clear();activeReflectionPlane=-1;reflectionBounds.clear();reflectionWater.clear();
 gpuProfile=GPUProfile{};spriteProfile=SpriteProfile{};sceneFadeProgram=0;sceneFadeStart=-1;sceneFade=0;
 camera={0,1.6f,4};cameraYaw=0;cameraGroundOffset=1.6f;cameraReady=false;
 coopMenuOpen=false;coopRecipients.clear();
 radial={};selectionGesture={};selectionPreview.clear();clearMoveMarkers();mapHovered=mapGesture=mapWarpRejected=false;routePoints=0;patrolRequest=0;patrolFlashUntil=0;pressWaypoint=-1;movePressValid=false;moveFacing=previewFacing=acceptedFormation=0;
 lastTapID=hoverID=hoverItem=pressItem=pickupKey=pickupRequest=0;lastTapTime=lastMoveTime=0;pickupEligible=false;pointerValid=groundValid=false;
 commandQueue.clear();pending.clear();authenticated=false;peer=listener=-1;perfStart=0;perfFrames=0;perfIPC=perfInput=perfSurface=perfScene=perfFX=perfControls=0;
}
static void useSceneProgram(GLuint target,const Mat&mvp,bool reflected){
 program=target;glUseProgram(program);scene_correction::apply(program);glUniformMatrix4fv(glGetUniformLocation(program,"mvp"),1,GL_FALSE,mvp.data());glUniform4f(glGetUniformLocation(program,"tint"),1,1,1,1);glUniform1f(glGetUniformLocation(program,"alphaCutoff"),.5f);glUniform1f(glGetUniformLocation(program,"reflectionClip"),reflected?1.f:0.f);glUniform1f(glGetUniformLocation(program,"reflectionHeight"),reflectionLevel);glUniform1i(glGetUniformLocation(program,"detailEnabled"),0);glUniform1i(glGetUniformLocation(program,"softOnly"),0);glUniform1i(glGetUniformLocation(program,"spriteCutout"),0);glUniform1i(glGetUniformLocation(program,"spriteNormalEnabled"),0);glActiveTexture(GL_TEXTURE0);applyFog(program);
}
// Units 2/3 belong to the unified receiver throughout the model pass. PBR
// materials use units 8..12, so switching materials does not invalidate them.
static bool unifiedMeshPrepared=false;
static unsigned unifiedMeshPreparations=0,unifiedMeshReuses=0,unifiedMeshSwitches=0;
static void beginUnifiedTerrain(const Mat&mvp,bool reflected,bool mesh=false){
 if(!unifiedGroundShadows()||!waterSunShadowReady||!spriteLiveDepth)return;
 if(mesh&&unifiedMeshPrepared){
  if(program!=unifiedMeshProgram){program=unifiedMeshProgram;glUseProgram(program);unifiedMeshSwitches++;}
  unifiedMeshReuses++;return;
 }
 if(mesh){unifiedMeshPrepared=true;unifiedMeshPreparations++;}
 useSceneProgram(mesh?unifiedMeshProgram:unifiedTerrainProgram,mvp,reflected);
 if(reflected)glUniform1f(glGetUniformLocation(program,"reflectionHeight"),reflectionLevel+.01f);
 glActiveTexture(GL_TEXTURE2);glBindTexture(GL_TEXTURE_2D,spriteLiveDepth);
 glUniform1i(glGetUniformLocation(program,"unifiedDepth"),2);
 glActiveTexture(GL_TEXTURE3);glBindTexture(GL_TEXTURE_2D,waterStaticDepth);glUniform1i(glGetUniformLocation(program,"unifiedStaticDepth"),3);
 glUniform1i(glGetUniformLocation(program,"unifiedBase"),mesh||unifiedBaseReady);
 glUniform3fv(glGetUniformLocation(program,"unifiedSun"),1,waterSun);
 if(!mesh){
  glUniform1i(glGetUniformLocation(program,"unifiedSmoothTerrain"),terrainNormalTexture!=0);
  glActiveTexture(GL_TEXTURE4);glBindTexture(GL_TEXTURE_2D,terrainNormalTexture);glUniform1i(glGetUniformLocation(program,"unifiedTerrainNormals"),4);
  if(terrainNormalTexture)glUniform4f(glGetUniformLocation(program,"unifiedNormalUV"),-1.f/(scale*terrain.width),1.f/(scale*terrain.height),(originX+.5f)/terrain.width,(originY+.5f)/terrain.height);
 }
 glUniformMatrix4fv(glGetUniformLocation(program,"unifiedSunMatrix"),1,GL_FALSE,waterSunMatrix.data());
 glUniform1f(glGetUniformLocation(program,"unifiedBias"),waterSunDepthBias);
 glUniform1f(glGetUniformLocation(program,"unifiedAmount"),enhancedShadowAmount);
 glUniform1i(glGetUniformLocation(program,"unifiedSamples"),enhancedShadowSamples);
 glActiveTexture(GL_TEXTURE0);
}
// Lazily prepare each variant once per eye/reflection pass. Switching ranges
// only changes the program; each stream restores its per-draw material state.
static Mat spritePassMVP;static bool spritePassReflected=false,spriteProgramReady[2]{};
static bool selectSpriteProgram(bool pbr){
 pbr=pbr&&billboardPbrProgram!=billboardProgram;GLuint target=pbr?billboardPbrProgram:billboardProgram;
 bool changed=program!=target;program=target;
 if(!spriteProgramReady[pbr]){useSceneProgram(target,spritePassMVP,spritePassReflected);bindSpriteShadows();spriteProgramReady[pbr]=true;return true;}
 if(changed)glUseProgram(program);return changed;
}
static void beginSpritePrograms(const Mat&mvp,bool reflected){
 spritePassMVP=mvp;spritePassReflected=reflected;spriteProgramReady[0]=spriteProgramReady[1]=false;
 sprite_pbr::sampleSun(waterSun);selectSpriteProgram(false);
}
static Mat modelMVP;static bool modelReflected=false,modelPrepared=false;
static void beginModelMaterials(const Mat&mvp,bool reflected){modelMVP=mvp;modelReflected=reflected;modelPrepared=false;unifiedMeshPrepared=false;modelColorPass=true;}
static void bindModelMaterial(GLuint texture,const rendercull::Bounds&bounds,bool moving){
 if(!modelColorPass)return;
 if(!sprite_pbr::meshes){if(unifiedGroundShadows()){beginUnifiedTerrain(modelMVP,modelReflected,true);return;}if(program!=lightweightProgram)useSceneProgram(lightweightProgram,modelMVP,modelReflected);return;}
 auto material=sprite_material::get(texture);bool enabled=enhancedSpritePath&&material.texture&&material.packed;
 float x=(bounds.lo[0]+bounds.hi[0])*.5f,y=(bounds.lo[1]+bounds.hi[1])*.5f,z=(bounds.lo[2]+bounds.hi[2])*.5f;
 float dx=bounds.hi[0]-x,dy=bounds.hi[1]-y,dz=bounds.hi[2]-z;
 float radius=sqrtf(dx*dx+dy*dy+dz*dz);
 if(!enabled||(!sprite_pbr::excludeMeshLod&&SpriteLightBoundsStream::farPbrSphere(x,y,z,radius))){
  if(unifiedGroundShadows()){beginUnifiedTerrain(modelMVP,modelReflected,true);return;}
  bool shadow=enabled&&enhancedShadowsEnabled&&waterSunShadowReady&&spriteLiveDepth&&modelShadowProgram!=lightweightProgram;
  GLuint target=shadow?modelShadowProgram:lightweightProgram;
  if(program!=target)useSceneProgram(target,modelMVP,modelReflected);
  if(shadow){
   glUniform1f(glGetUniformLocation(target,"spriteShadowAmount"),enhancedShadowAmount);
   glUniform3f(glGetUniformLocation(target,"spriteSunDirection"),waterSun[0],waterSun[1],waterSun[2]);
  }
  return;
 }
 if(!modelPrepared){useSceneProgram(modelPbrProgram,modelMVP,modelReflected);bindSpriteShadows();sprite_pbr::bind(program,waterSun,true);modelPrepared=true;}
 else if(program!=modelPbrProgram){program=modelPbrProgram;glUseProgram(program);}
 glUniform1i(glGetUniformLocation(program,"spriteLightCull"),1);
 glDisableVertexAttribArray(4);glVertexAttrib1f(4,moving?16777215.f:16777214.f);
 glDisableVertexAttribArray(5);glVertexAttrib4f(5,x,y,z,radius);
 glDisableVertexAttribArray(9);glVertexAttrib4f(9,0,0,0,1);
 sprite_material::bind(program,texture);
}
#include "tracking_gizmo.h"
static void draw(const XrView&eye,const XrVector3f&center,bool reflected=false,unsigned eyeIndex=0){
 if(!reflected)shadowLodEye=center;
 if(!reflected){stageProfile.begin(7);updateSunShadows();stageProfile.end();}
 Mat normalMVP=mul(projection(eye.fov),view(eye.pose));
 if(!reflected){weatherCenter=center;pollSkyReflectionBlend();pollReflectionSetting();for(unsigned plane=0;plane<reflectionPlanes.size();plane++){activateReflectionPlane(plane);reflectionReady=false;
  if(reflectionEnabled&&!reflectionPlanes[plane].visibility.render){++reflectionPlanes[plane].visibility.skipped;continue;}
  if(reflectionEnabled&&eye.pose.position.y>reflectionLevel+.05f&&visibleReflectionWater(normalMVP)){
   GLint savedFBO,viewport[4];glGetIntegerv(GL_FRAMEBUFFER_BINDING,&savedFBO);glGetIntegerv(GL_VIEWPORT,viewport);double started=seconds();
   if(prepareReflectionTarget(viewport[2],viewport[3])){reflectionScreenRect=projectReflectionWater(normalMVP);if(reflectionScreenRect.valid()){float px=8.f/reflectionW,py=8.f/reflectionH;reflectionScreenRect.l=std::max(-1.f,reflectionScreenRect.l-px);reflectionScreenRect.r=std::min(1.f,reflectionScreenRect.r+px);reflectionScreenRect.b=std::max(-1.f,reflectionScreenRect.b-py);reflectionScreenRect.t=std::min(1.f,reflectionScreenRect.t+py);reflectionCull.set(mul(normalMVP,reflectionMatrix()),reflectionScreenRect,reflectionLevel);glViewport(0,0,reflectionW,reflectionH);draw(eye,{center.x,2*reflectionLevel-center.y,center.z},true,eyeIndex);reflectionReady=true;reflectionPasses++;}}
   glDisable(GL_SCISSOR_TEST);
   glBindFramebuffer(GL_FRAMEBUFFER,savedFBO);glViewport(viewport[0],viewport[1],viewport[2],viewport[3]);perfReflection+=seconds()-started;
  }
 }}
 pollFog();fogCenter=center;
 stageProfile.begin(reflected?8:0);double sceneStart=seconds();program=lightweightProgram;glUseProgram(program);scene_correction::apply(program);glUniform1f(glGetUniformLocation(program,"alphaCutoff"),.5f);Mat mvp=reflected?mul(normalMVP,reflectionMatrix()):normalMVP;glUseProgram(program);glUniform1f(glGetUniformLocation(program,"reflectionClip"),reflected?1.f:0.f);glUniform1f(glGetUniformLocation(program,"reflectionHeight"),reflectionLevel);glUniform4f(glGetUniformLocation(program,"tint"),1,1,1,1);glUniformMatrix4fv(glGetUniformLocation(program,"mvp"),1,GL_FALSE,mvp.data());glBindVertexArray(vao);glEnable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);glDisable(GL_BLEND);glClearColor(0,0,0,(reflected||bloom::active())?0.f:1.f);glDepthMask(GL_TRUE);glDisable(GL_SCISSOR_TEST);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);if(reflected){int x=std::max(0,(int)floorf((reflectionScreenRect.l+1)*.5f*reflectionW)),y=std::max(0,(int)floorf((reflectionScreenRect.b+1)*.5f*reflectionH));int right=std::min(reflectionW,(int)ceilf((reflectionScreenRect.r+1)*.5f*reflectionW)),top=std::min(reflectionH,(int)ceilf((reflectionScreenRect.t+1)*.5f*reflectionH));glEnable(GL_SCISSOR_TEST);glScissor(x,y,right-x,top-y);}if(reflected)maskReflectionWater(normalMVP);applyFog(program);if(!reflected&&bloom::active())glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_FALSE);drawSkybox(eye,reflected);if(!reflected&&bloom::active())glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);stageProfile.begin(reflected?9:1);if(!reflected){beginUnifiedTerrain(mvp,reflected);attributes(terrainVBO);useDetailTerrain(true);glBindTexture(GL_TEXTURE_2D,textures[0]);{TerrainCullScope terrainCull(false);rendercull::Frustum terrainView;terrainView.set(normalMVP,{-1,-1,1,1},-INFINITY);static std::vector<rendercull::DrawRange> visible;terrainViewTested+=terrainRanges.size();terrainViewVisible+=rendercull::visibleRanges(terrainRanges,terrainView,visible);terrainViewDraws+=visible.size();for(const auto&r:visible)glDrawArrays(GL_TRIANGLES,r.first,r.count);}useDetailTerrain(false);useSceneProgram(lightweightProgram,mvp,reflected);stageProfile.begin(2);for(unsigned plane=0;plane<reflectionPlanes.size();plane++){activateReflectionPlane(plane);waterEye[0]=eye.pose.position.x;waterEye[1]=eye.pose.position.y;waterEye[2]=eye.pose.position.z;waterSkyTexture=skyMap;waterSkyYaw=skyYaw;waterFogDensity=fogEnabled?fogDensity:0;compositeReflections(mvp);}drawShadowDebug(mvp);}else if(reflectionFull){glUniform1f(glGetUniformLocation(program,"reflectionHeight"),reflectionLevel+.01f);beginUnifiedTerrain(mvp,reflected);attributes(terrainVBO);useDetailTerrain(true);glBindTexture(GL_TEXTURE_2D,textures[0]);{TerrainCullScope terrainCull(true);for(auto&r:terrainRanges){reflectTerrainTested++;if(!reflectionCull.visible(r.bounds))continue;reflectTerrainDrawn++;glDrawArrays(GL_TRIANGLES,r.first,r.count);}}useDetailTerrain(false);useSceneProgram(lightweightProgram,mvp,reflected);glUniform1f(glGetUniformLocation(program,"reflectionHeight"),reflectionLevel);}if(reflected)reflectTerrainCPU+=seconds()-sceneStart;stageProfile.begin(reflected?11:3);double modelStart=seconds();setSpriteLightView(reflected,reflectionMatrix());beginModelMaterials(mvp,reflected);for(auto&m:(reflected?models:normalModels)){if(reflected){reflectModelsTested++;if(!reflectionCull.visible(m.bounds))continue;reflectModelsDrawn++;}attributes(m.buffer);GLuint colorTexture=hdModel(m.texture);bindModelMaterial(colorTexture,m.bounds,false);glBindTexture(GL_TEXTURE_2D,colorTexture);glDrawArrays(GL_TRIANGLES,0,m.count);}useSceneProgram(lightweightProgram,mvp,reflected);if(!connectorParts.empty()){static std::vector<std::array<int32_t,7>> cached;static GLuint source=0,buffer=0;static GLsizei count=0;if(!buffer)glGenBuffers(1,&buffer);if(source!=connectorParts[0].batch.buffer||cached!=connectorEdges){source=connectorParts[0].batch.buffer;cached=connectorEdges;std::vector<Vertex> verts;for(auto&p:connectorParts)for(auto&e:cached){if(!std::equal(p.endpoints.begin(),p.endpoints.end(),e.begin())||e[6]<=0)continue;double dx=e[2]-e[0],dy=e[3]-e[1],dz=e[5]-e[4];int segments=std::max(1,std::min(512,(int)(sqrt(dx*dx+dy*dy+dz*dz)/e[6])));auto point=[&](int j,float v,float u){int t=256*j/segments;float x=(e[0]+floor(dx*t/256))/512.f,z=(e[1]+floor(dy*t/256))/512.f,y=(e[4]+floor(dz*t/256))/512.f+p.top-v*p.rise;return Vertex{mythWorldX(x,originX,scale),(y-originZ)*scale,(z-originY)*scale,u,v};};for(int j=0;j<segments;j++){Vertex q[4]={point(j,0,0),point(j+1,0,1),point(j+1,1,1),point(j,1,0)};for(int k:{0,1,2,0,2,3})verts.push_back(q[k]);}}count=(GLsizei)verts.size();glBindBuffer(GL_ARRAY_BUFFER,buffer);glBufferData(GL_ARRAY_BUFFER,verts.size()*sizeof(Vertex),verts.data(),GL_DYNAMIC_DRAW);}if(count){if(unifiedGroundShadows())beginUnifiedTerrain(mvp,reflected,true);attributes(buffer);glBindTexture(GL_TEXTURE_2D,connectorParts[0].batch.texture);glDrawArrays(GL_TRIANGLES,0,count);}}drawAnimatedModels(reflected);modelColorPass=false;useSceneProgram(lightweightProgram,mvp,reflected);if(!reflected)queryReflectionVisibility(normalMVP,eyeIndex);if(reflected)reflectModelsCPU+=seconds()-modelStart;stageProfile.begin(reflected?12:4);double spritesStart=seconds();bool spritePass=!reflected||prepareReflectionSpritePass(center);if(spritePass){setSpriteLightView(reflected,reflectionMatrix());spriteWait.kind=reflected;beginSpritePrograms(mvp,reflected);attributes(vbo);}
 // Both eyes share billboard orientation and sprite direction to avoid stereo rivalry.
 {if(spritePass){spriteWait.kind=reflected;SpriteWaitScope whole(reflected,0);drawPreparedSprites(center,reflected,mvp);}else reflectionCandidates.passReady=false;}auto&cache=spriteView(reflected);auto&selectionFrames=cache.selectionFrames;auto&friendlyHover=cache.friendlyHover;auto&enemyHover=cache.enemyHover;auto&itemHover=cache.itemHover;if(reflected){reflectSpritesCPU+=seconds()-spritesStart;stageProfile.begin(13);double fxStart=seconds();if(reflectionFull){if(!spritePass){setSpriteLightView(true,reflectionMatrix());beginSpritePrograms(mvp,true);attributes(vbo);}drawParticles(center,true);drawWeather(mvp,true);}reflectFXCPU+=seconds()-fxStart;glUniform1i(glGetUniformLocation(program,"spriteShadows"),0);glDisable(GL_STENCIL_TEST);stageProfile.end();return;}perfScene+=seconds()-sceneStart;stageProfile.begin(5);double fxStart=seconds();particleViewCull.set(normalMVP,{-1,-1,1,1},-INFINITY);drawParticles(center);glUniform1i(glGetUniformLocation(program,"spriteShadows"),0);drawWeather(mvp,false);perfFX+=seconds()-fxStart;if(bloom::active()){stageProfile.begin(14);bloom::render();}stageProfile.begin(6);double controlsStart=seconds();useSceneProgram(lightweightProgram,mvp,false);scene_correction::apply(program,false);applyFog(program,false);drawControls(selectionFrames,friendlyHover,enemyHover,itemHover,cache.healthBars,cache.manaBars);drawSelectionBox();drawWristMap(eye.pose);drawRadial();drawCoopTransfer();drawMultiplayerNotice(multiplayerHead);drawChatKeyboard();drawSceneFade();perfControls+=seconds()-controlsStart;stageProfile.end();
}
static void diagnostic(android_app*a){GLuint fb,tx,depth;glGenFramebuffers(1,&fb);glBindFramebuffer(GL_FRAMEBUFFER,fb);glGenTextures(1,&tx);glBindTexture(GL_TEXTURE_2D,tx);glTexImage2D(GL_TEXTURE_2D,0,GL_SRGB8_ALPHA8,1024,1024,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,tx,0);glGenRenderbuffers(1,&depth);glBindRenderbuffer(GL_RENDERBUFFER,depth);glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT24,1024,1024);glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,depth);if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)throw std::runtime_error("diagnostic FBO");XrView v{XR_TYPE_VIEW};v.pose.position={11,4,-9};float yaw=.2f,pitch=-.25f;v.pose.orientation={sinf(pitch/2)*cosf(yaw/2),cosf(pitch/2)*sinf(yaw/2),-sinf(pitch/2)*sinf(yaw/2),cosf(pitch/2)*cosf(yaw/2)};v.fov={-.6f,.6f,.6f,-.6f};glViewport(0,0,1024,1024);beginSpriteFrame();beginReflectionFrame();weatherReady=false;draw(v,v.pose.position);std::vector<unsigned char>pixels(1024*1024*4);glReadPixels(0,0,1024,1024,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());char path[512];snprintf(path,sizeof(path),"%s/model-diagnostic.rgba",a->activity->internalDataPath);FILE*f=fopen(path,"wb");if(f){uint32_t n=1024;fwrite(&n,4,1,f);fwrite(&n,4,1,f);fwrite(pixels.data(),1,pixels.size(),f);fclose(f);}glDeleteFramebuffers(1,&fb);glDeleteTextures(1,&tx);glDeleteRenderbuffers(1,&depth);LOG("Model diagnostic saved");}
#include "display_runtime.h"
struct Swap{XrSwapchain handle{};uint32_t w,h;std::vector<XrSwapchainImageOpenGLESKHR> images;GLuint depth;EyeMSAA msaa;};
void android_main(android_app*a){
 camera={0,1.6f,4};cameraYaw=0;cameraGroundOffset=1.6f;cameraReady=false;
 coopMenuOpen=false;coopRecipients.clear();
 input=TouchInput{};radial.close();selectionGesture.cancel();selectionPreview.clear();lastTapID=0;
 pointerValid=groundValid=movePressValid=false;commandQueue.clear();requestPending=0;commandFailed=false;
 lastMoveTime=lastCameraSync=0;objects.clear();models.clear();quads.clear();textures.clear();unitFrameBounds.clear();
 peer=listener=-1;authenticated=false;pending.clear();
 XrInstance instance=XR_NULL_HANDLE;XrSession session=XR_NULL_HANDLE;XrSpace space=XR_NULL_HANDLE;bool jniAttached=false;
 try{bloom::load(a->activity->internalDataPath);scene_correction::load(a->activity->internalDataPath);LOG("Scene correction enabled=%d strength=%.2f black=0 white=199 sky_ui=excluded",scene_correction::enabled,scene_correction::strength);pointer_style::load(a->activity->internalDataPath);LOG("Pointer thickness=%.1fx",pointer_style::scale);sprite_atlas::load(a->activity->internalDataPath);ui_contrast::load(a->activity->internalDataPath);loadSpriteCutout(a->activity->internalDataPath);fpsDisplay.load(a->activity->internalDataPath);loadEnhancedShadows(a->activity->internalDataPath);loadEnhancedLighting(a->activity->internalDataPath);sprite_pbr::load(a->activity->internalDataPath);enhancedSpritePath=enhancedShadowsEnabled||enhancedLightingEnabled||sprite_pbr::enabled();billboardShaderShadows=enhancedShadowsEnabled;billboardShaderLighting=enhancedLightingEnabled;LOG("Enhanced sprite path=%s",enhancedSpritePath?"full":"bypass");LOG("Vertex shading LOD distance=20..24 angular_radius=0.02..0.03 large_objects=detail normal_map_far=off");snprintf(soundRangePath,sizeof(soundRangePath),"%s/sound.range",a->activity->internalDataPath);loadLevelConfig(a->activity->internalDataPath);atlas_usage::begin(a->activity->externalDataPath,levelAssetPrefix,levelName);initDiagnosticLog(a->activity->internalDataPath);benchmarkPose.load(a->activity->internalDataPath,levelAssetPrefix);reflection_sprite_lod::load(a->activity->internalDataPath);LOG("Reflection sprite LOD settings enabled=%d distance_m=%.1f pixels=%.1f",reflection_sprite_lod::enabled,reflection_sprite_lod::distance,reflection_sprite_lod::pixels);diagnosticBenchmarkActive=benchmarkPose.enabled;if(diagnosticBenchmarkActive)diagnosticEnabled=true;LOG("DebugSpecularLightingMode enabled=%d view=specular-only",sprite_pbr::debugSpecular);LOG("DebugSSSLightingMode enabled=%d view=sss-only",sprite_pbr::debugSss);LOG("Bloom enabled=%d divisor=%d iterations=%d strength=%.2f sky_ui=excluded",bloom::enabled,bloom::divisors[bloom::resolution],bloom::iterations,bloom::strength);spriteApi.load(a->activity->internalDataPath);for(auto&c:vertexLodCounts)c={};stageProfile=StageProfile{};spriteDetail=SpriteDetailProfile{};spriteWait=SpriteWaitProfile{};streamUploadCalls=streamUploadBytes=streamDrawCalls=streamVertices=0;streamUploadSeconds=0;LOG("Sprite HD edge mode=%s cutoff=0.5",spriteCutoutOnePass?"cutout-one-pass":"smooth-two-pass");LOG("Shadow debug overlay=%d pixel_receivers=%d",shadowDebugEnabled,shadowDebugEnabled);LOG("Diagnostic settings shadows=%d lighting=%d samples=%d lod=%.2f",enhancedShadowsEnabled,enhancedLightingEnabled,enhancedShadowSamples,enhancedShadowLodDistance);LOG("Shadow comparison skip_sprite_lookup=%d keep_generation=1 keep_self_shading=1",spriteSkipShadowLookup);LOG("VR transition lifecycle wait mono=%.3f",seconds());awaitLoadingResume(a);LOG("VR transition resume acknowledged mono=%.3f",seconds());snprintf(sceneDiagnosticPath,sizeof(sceneDiagnosticPath),"%s/scene-diagnostic.bin",a->activity->externalDataPath);lastDiagnosticTick=0;sceneFade=0;sceneFadeStart=-1;liveSceneReady=false;gameSpeed.reset();multiplayerHud={};chat={};snapshotRevision=0;objects.clear();tick=0;JNIEnv*env=nullptr;a->activity->vm->AttachCurrentThread(&env,nullptr);jniAttached=true;PFN_xrInitializeLoaderKHR init=nullptr;ck(xrGetInstanceProcAddr(XR_NULL_HANDLE,"xrInitializeLoaderKHR",(PFN_xrVoidFunction*)&init));XrLoaderInitInfoAndroidKHR li{XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR};li.applicationVM=a->activity->vm;li.applicationContext=a->activity->clazz;ck(init((XrLoaderInitInfoBaseHeaderKHR*)&li));bool refreshExtension=supportsRefreshExtension();std::vector<const char*> extensions={XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME,XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME};if(refreshExtension)extensions.push_back(XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME);XrInstanceCreateInfoAndroidKHR ai{XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR};ai.applicationVM=a->activity->vm;ai.applicationActivity=a->activity->clazz;XrInstanceCreateInfo ci{XR_TYPE_INSTANCE_CREATE_INFO};ci.next=&ai;strcpy(ci.applicationInfo.applicationName,"Myth II VR Bridge");ci.applicationInfo.apiVersion=XR_MAKE_VERSION(1,0,34);ci.enabledExtensionCount=extensions.size();ci.enabledExtensionNames=extensions.data();ck(xrCreateInstance(&ci,&instance));XrSystemGetInfo si{XR_TYPE_SYSTEM_GET_INFO};si.formFactor=XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;XrSystemId system;ck(xrGetSystem(instance,&si,&system));PFN_xrGetOpenGLESGraphicsRequirementsKHR reqfn;ck(xrGetInstanceProcAddr(instance,"xrGetOpenGLESGraphicsRequirementsKHR",(PFN_xrVoidFunction*)&reqfn));XrGraphicsRequirementsOpenGLESKHR req{XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR};ck(reqfn(instance,system,&req));EGLDisplay display=eglGetDisplay(EGL_DEFAULT_DISPLAY);if(!eglInitialize(display,nullptr,nullptr))throw std::runtime_error("egl init");EGLint attrib[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_DEPTH_SIZE,24,EGL_NONE};EGLConfig config;EGLint count;eglChooseConfig(display,attrib,&config,1,&count);EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE},pa[]={EGL_WIDTH,16,EGL_HEIGHT,16,EGL_NONE};EGLContext context=eglCreateContext(display,config,EGL_NO_CONTEXT,ca);EGLSurface surface=eglCreatePbufferSurface(display,config,pa);if(!eglMakeCurrent(display,surface,surface,context))throw std::runtime_error("egl current");XrGraphicsBindingOpenGLESAndroidKHR binding{XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR};binding.display=display;binding.config=config;binding.context=context;XrSessionCreateInfo sci{XR_TYPE_SESSION_CREATE_INFO};sci.next=&binding;sci.systemId=system;ck(xrCreateSession(instance,&sci,&session));DisplayRuntime displayOptions;displayOptions.init(instance,session,refreshExtension,a->activity->internalDataPath);XrReferenceSpaceCreateInfo rci{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};rci.referenceSpaceType=XR_REFERENCE_SPACE_TYPE_LOCAL;rci.poseInReferenceSpace.orientation.w=1;ck(xrCreateReferenceSpace(session,&rci,&space));input=TouchInput{};input.init(instance,session);uint32_t vc;ck(xrEnumerateViewConfigurationViews(instance,system,XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,0,&vc,nullptr));if(vc!=2)throw std::runtime_error("stereo required");std::vector<XrViewConfigurationView> configs(vc,{XR_TYPE_VIEW_CONFIGURATION_VIEW});ck(xrEnumerateViewConfigurationViews(instance,system,XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,vc,&vc,configs.data()));uint32_t nf;ck(xrEnumerateSwapchainFormats(session,0,&nf,nullptr));std::vector<int64_t> formats(nf);ck(xrEnumerateSwapchainFormats(session,nf,&nf,formats.data()));int64_t format=GL_SRGB8_ALPHA8;if(std::find(formats.begin(),formats.end(),format)==formats.end()){format=GL_RGBA8;if(std::find(formats.begin(),formats.end(),format)==formats.end())throw std::runtime_error("No supported RGB swapchain");}LOG("Color pipeline: display RGB decoded in shader, swapchain=0x%llx (%s)",(long long)format,format==GL_SRGB8_ALPHA8?"sRGB":"linear fallback");std::vector<Swap> swaps(vc);for(unsigned i=0;i<vc;i++){auto&s=swaps[i];s.w=configs[i].recommendedImageRectWidth;s.h=configs[i].recommendedImageRectHeight;if(displayOptions.requested.width){s.w=std::min(displayOptions.requested.width,configs[i].maxImageRectWidth);s.h=std::min(displayOptions.requested.height,configs[i].maxImageRectHeight);}XrSwapchainCreateInfo c{XR_TYPE_SWAPCHAIN_CREATE_INFO};c.usageFlags=XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;c.format=format;c.sampleCount=1;c.width=s.w;c.height=s.h;c.faceCount=1;c.arraySize=1;c.mipCount=1;ck(xrCreateSwapchain(session,&c,&s.handle));uint32_t n;ck(xrEnumerateSwapchainImages(s.handle,0,&n,nullptr));s.images.resize(n,{XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR});ck(xrEnumerateSwapchainImages(s.handle,n,&n,(XrSwapchainImageBaseHeader*)s.images.data()));glGenRenderbuffers(1,&s.depth);glBindRenderbuffer(GL_RENDERBUFFER,s.depth);glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT24,s.w,s.h);if(displayOptions.requested.samples>1&&!s.msaa.create(s.w,s.h,GLenum(format),displayOptions.requested.samples))LOG("Eye %u MSAA unsupported; using OFF",i);LOG("Eye %u swapchain %ux%u images=%u MSAA=%u",i,s.w,s.h,n,s.msaa.fbo?2:1);}if(!swaps[0].msaa.fbo||!swaps[1].msaa.fbo){for(auto&s:swaps)s.msaa.destroy();}displayOptions.width=swaps[0].w;displayOptions.height=swaps[0].h;displayOptions.samples=swaps[0].msaa.fbo?2:1;resetMapRenderer();surfaceUpdatesQueue={};surfaceUpdatesQueue.tileLimit=(levelWidth/32)*(levelHeight/32);liveLights.clear();liveParticles.clear();liveLightsValid=false;livePalettes.clear();liveHues.clear();modelPoses.clear();modelPosesValid=false;LOG("VR transition geometry begin mono=%.3f",seconds());pumpLoadingEvents(true);geometry(a);pumpLoadingEvents(true);LOG("VR transition geometry end mono=%.3f",seconds());loadSpriteIndices(a);loadAnimatedModels(a);loadModels(a);loadSurfaceEffects(a);loadGroundAttackMarker(a);loadWristMap(a);loadCameraSpeeds(a->activity->internalDataPath);loadParticles(a);pumpLoadingEvents(true);loadReflections(a);loadWaterLight(a);loadWaterNormalAsset(a);initSkybox();initSkyReflectionBlend(a->activity->internalDataPath);initWeatherSetting(a->activity->internalDataPath);loadSkybox(env,a->activity->externalDataPath,levelName.c_str(),levelSkyYaw,a->activity->internalDataPath);initSceneFade();pumpLoadingEvents(true);loadingApp=nullptr;LOG("VR transition assets ready mono=%.3f",seconds());ipc(a);GLuint fbo;glGenFramebuffers(1,&fbo);bool running=false,focused=false;XrTime lastFrameTime=0;unsigned frames=0;while(!a->destroyRequested){int events;android_poll_source*source;while(ALooper_pollOnce(running?0:10,nullptr,&events,(void**)&source)>=0){if(source)source->process(a,source);if(a->destroyRequested)break;if(!running)break;}XrEventDataBuffer event{XR_TYPE_EVENT_DATA_BUFFER};while(xrPollEvent(instance,&event)==XR_SUCCESS){if(event.type==XR_TYPE_EVENT_DATA_DISPLAY_REFRESH_RATE_CHANGED_FB){auto*rate=(XrEventDataDisplayRefreshRateChangedFB*)&event;displayOptions.actualHz=rate->toDisplayRefreshRate;displayOptions.report();}if(event.type==XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED){auto*state=(XrEventDataSessionStateChanged*)&event;LOG("Session state=%d",state->state);focused=state->state==XR_SESSION_STATE_FOCUSED;if(focused)loadCameraSpeeds(a->activity->internalDataPath);if(state->state==XR_SESSION_STATE_READY){XrSessionBeginInfo bi{XR_TYPE_SESSION_BEGIN_INFO};bi.primaryViewConfigurationType=XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;ck(xrBeginSession(session,&bi));running=true;displayOptions.applyRate(session);}if(state->state==XR_SESSION_STATE_STOPPING){running=false;ck(xrEndSession(session));}if(state->state==XR_SESSION_STATE_EXITING||state->state==XR_SESSION_STATE_LOSS_PENDING)a->destroyRequested=1;}event={XR_TYPE_EVENT_DATA_BUFFER};}double ipcStart=seconds();pollIPC();perfIPC+=seconds()-ipcStart;if(!running)continue;XrFrameWaitInfo wi{XR_TYPE_FRAME_WAIT_INFO};XrFrameState state{XR_TYPE_FRAME_STATE};double waitStart=diagnosticEnabled?seconds():0;ck(xrWaitFrame(session,&wi,&state));if(diagnosticEnabled)stageProfile.waits[0]+=seconds()-waitStart;XrFrameBeginInfo bi{XR_TYPE_FRAME_BEGIN_INFO};ck(xrBeginFrame(session,&bi));std::array<XrCompositionLayerProjectionView,2> layers{{{XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW},{XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW}}};XrCompositionLayerProjection layer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};layer.space=space;layer.viewCount=2;layer.views=layers.data();bool rendered=false;if(state.shouldRender&&liveSceneReady){XrViewLocateInfo vi{XR_TYPE_VIEW_LOCATE_INFO};vi.viewConfigurationType=XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;vi.displayTime=state.predictedDisplayTime;vi.space=space;XrViewState vst{XR_TYPE_VIEW_STATE};std::array<XrView,2> views{{{XR_TYPE_VIEW},{XR_TYPE_VIEW}}};uint32_t n;ck(xrLocateViews(session,&vi,&vst,2,&n,views.data()));if(n==2&&(vst.viewStateFlags&XR_VIEW_STATE_POSITION_VALID_BIT)&&(vst.viewStateFlags&XR_VIEW_STATE_ORIENTATION_VALID_BIT)){double surfaceStart=seconds();updateSurfaceEffects();perfSurface+=seconds()-surfaceStart;auto localViews=views;for(int i=0;i<2;i++)benchmarkPose.tracked[i]=views[i];benchmarkPose.views(views.data());float dt=lastFrameTime?std::clamp((state.predictedDisplayTime-lastFrameTime)/1e9f,.001f,.05f):1.f/72;lastFrameTime=state.predictedDisplayTime;double inputStart=seconds();updateInput(session,space,state.predictedDisplayTime,views.data(),dt,focused);perfInput+=seconds()-inputStart;bool benchmarkStarting=benchmarkPose.enabled&&!benchmarkPose.started;captureBenchmarkPose(session,space,state.predictedDisplayTime,views.data());if(benchmarkStarting){LOG("Benchmark settings eye=%ux%u msaa=%u hz=%.2f reflection=%d full=%d high=%d waves=%d shadows=%d lighting=%d mesh_pbr=%d bloom=%d",displayOptions.width,displayOptions.height,displayOptions.samples,displayOptions.actualHz,reflectionEnabled,reflectionFull,reflectionHigh,waterWavesEnabled,enhancedShadowsEnabled,enhancedLightingEnabled,sprite_pbr::meshes,bloom::enabled);}for(auto&v:views)v.pose=worldPose(v.pose);for(unsigned eye=0;eye<2;eye++)lightingEyeMatrices[eye]=mul(projection(views[eye].fov),view(views[eye].pose));XrVector3f center{(views[0].pose.position.x+views[1].pose.position.x)/2,(views[0].pose.position.y+views[1].pose.position.y)/2,(views[0].pose.position.z+views[1].pose.position.z)/2};updateSceneFade();double spriteFrameStart=seconds();beginSpriteFrame();beginReflectionFrame();weatherReady=false;perfScene+=seconds()-spriteFrameStart;for(unsigned i=0;i<2;i++){auto&s=swaps[i];uint32_t ix;XrSwapchainImageAcquireInfo aq{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};waitStart=diagnosticEnabled?seconds():0;ck(xrAcquireSwapchainImage(s.handle,&aq,&ix));XrSwapchainImageWaitInfo sw{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};sw.timeout=XR_INFINITE_DURATION;ck(xrWaitSwapchainImage(s.handle,&sw));if(diagnosticEnabled)stageProfile.waits[1]+=seconds()-waitStart;glBindFramebuffer(GL_FRAMEBUFFER,fbo);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,s.images[ix].image,0);glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,s.depth);if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)throw std::runtime_error("FBO");glBindFramebuffer(GL_FRAMEBUFFER,s.msaa.fbo?s.msaa.fbo:fbo);glViewport(0,0,s.w,s.h);draw(views[i],center,false,i);waitStart=diagnosticEnabled?seconds():0;s.msaa.resolve(fbo);if(diagnosticEnabled)stageProfile.waits[2]+=seconds()-waitStart;if(diagnosticEnabled&&!benchmarkPose.enabled&&frames==120){std::vector<unsigned char>pixels(s.w*s.h*4);glReadPixels(0,0,s.w,s.h,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());char path[512];snprintf(path,sizeof(path),"%s/eye-%u.rgba",a->activity->internalDataPath,i);FILE*f=fopen(path,"wb");if(f){fwrite(&s.w,4,1,f);fwrite(&s.h,4,1,f);fwrite(pixels.data(),1,pixels.size(),f);fclose(f);LOG("Saved eye %u",i);}}waitStart=diagnosticEnabled?seconds():0;glFlush();XrSwapchainImageReleaseInfo rel{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};ck(xrReleaseSwapchainImage(s.handle,&rel));if(diagnosticEnabled)stageProfile.waits[3]+=seconds()-waitStart;layers[i].pose=localViews[i].pose;layers[i].fov=views[i].fov;layers[i].subImage.swapchain=s.handle;layers[i].subImage.imageRect.extent={(int32_t)s.w,(int32_t)s.h};}rendered=true;if(frames%120==0)LOG("Stereo frame=%u tick=%u objects=%zu center=%.3f,%.3f,%.3f gl=%u palettes=%zu colored=%zu models=%zu valid=%d first=%08x matrix=%d,%d,%d",frames,tick,objects.size(),center.x,center.y,center.z,glGetError(),livePalettes.size(),coloredSprites.size(),modelPoses.size(),modelPosesValid,modelPoses.empty()?0:modelPoses[0].geometry,modelPoses.empty()?0:modelPoses[0].matrix[0],modelPoses.empty()?0:modelPoses[0].matrix[4],modelPoses.empty()?0:modelPoses[0].matrix[8]);}}XrFrameEndInfo ei{XR_TYPE_FRAME_END_INFO};ei.displayTime=state.predictedDisplayTime;ei.environmentBlendMode=XR_ENVIRONMENT_BLEND_MODE_OPAQUE;const XrCompositionLayerBaseHeader*lp=(XrCompositionLayerBaseHeader*)&layer;ei.layerCount=rendered?1:0;ei.layers=rendered?&lp:nullptr;waitStart=diagnosticEnabled?seconds():0;ck(xrEndFrame(session,&ei));if(diagnosticEnabled)stageProfile.waits[4]+=seconds()-waitStart;fpsDisplay.frame(seconds(),rendered);if(rendered){double now=seconds();if(!perfStart)perfStart=now;if(++perfFrames&&(benchmarkPose.enabled?now-perfStart>=1.0:perfFrames>=120)){LOG("Benchmark fixed=%d tick=%u normal_bytes=%zu normal_textures=%zu",benchmarkPose.enabled,tick,sprite_material::normalGpuBytes,sprite_material::bindings.size());LOG("Benchmark sample source=%s saved_tick=%u current_tick=%u level=%s",benchmarkPose.reason.c_str(),benchmarkPose.loaded?benchmarkPose.saved.tick:0,tick,levelAssetPrefix.c_str());LOG("VR cadence fps=%.2f frames=%u interval=%.3f particles=%zu weather=%u",perfFrames/(now-perfStart),perfFrames,now-perfStart,liveParticles.size(),weatherDrops);LOG("VR CPU ms/frame ipc=%.3f input=%.3f ground=%.3f scene=%.3f fx=%.3f controls=%.3f sprite_uploads=%u marker_builds=%u route=%u",perfIPC*1000/perfFrames,perfInput*1000/perfFrames,perfSurface*1000/perfFrames,perfScene*1000/perfFrames,perfFX*1000/perfFrames,perfControls*1000/perfFrames,spriteUploads,markerBuilds,routePoints);LOG("Reflection enabled=%d full=%d high=%d cpu_ms=%.3f passes=%u resolution=%dx%d",reflectionEnabled,reflectionFull,reflectionHigh,perfReflection*1000/perfFrames,reflectionPasses,reflectionW,reflectionH);LOG("Sprite cache builds=%u,%u reuse=%u,%u prepare_ms_frame=%.3f,%.3f",spriteBuilds[0],spriteBuilds[1],spriteReuse[0],spriteReuse[1],spritePrepareCPU[0]*1000/perfFrames,spritePrepareCPU[1]*1000/perfFrames);spriteBuilds[0]=spriteBuilds[1]=spriteReuse[0]=spriteReuse[1]=0;spritePrepareCPU[0]=spritePrepareCPU[1]=0;LOG("Reflection geometry shared_builds=%u frame_prepare_reuses=%u",reflectionGeometryBuilds,reflectionGeometryReuses);reflectionGeometryBuilds=reflectionGeometryReuses=0;LOG("Unified mesh settings prepares=%u reuses=%u switches=%u",unifiedMeshPreparations,unifiedMeshReuses,unifiedMeshSwitches);unifiedMeshPreparations=unifiedMeshReuses=unifiedMeshSwitches=0;LOG("Terrain view chunks=%u/%u draw_calls=%u",terrainViewVisible,terrainViewTested,terrainViewDraws);terrainViewVisible=terrainViewTested=terrainViewDraws=0;LOG("Sprite lighting selections=%u reused=%u",spriteLightSelections,spriteLightBindingReuses);spriteLightSelections=spriteLightBindingReuses=0;LOG("Sprite stream uploads=%llu bytes=%llu cpu_ms_frame=%.3f draw_calls=%llu vertices=%llu",(unsigned long long)streamUploadCalls,(unsigned long long)streamUploadBytes,streamUploadSeconds*1000/perfFrames,(unsigned long long)streamDrawCalls,(unsigned long long)streamVertices);streamUploadCalls=streamUploadBytes=streamDrawCalls=streamVertices=0;streamUploadSeconds=0;LOG("Sprite cache sizes normal_references=%zu normal_sprites=%zu normal_ranges=%zu colored=%zu hd_textures=%zu",spriteViews[0].inputs.size(),spriteViews[0].sprites.size(),spriteViews[0].stream.ranges.size(),coloredSprites.size(),hd::textures.size());LOG("Sprite batch CPU ms_frame normal=%.3f reflection=%.3f builds=%u,%u",spriteBatchCPU[0]*1000/perfFrames,spriteBatchCPU[1]*1000/perfFrames,spriteBatchPasses[0],spriteBatchPasses[1]);spriteBatchCPU[0]=spriteBatchCPU[1]=0;spriteBatchPasses[0]=spriteBatchPasses[1]=0;LOG("Sprite stereo shared_upload_reuses=%u",stereoUploadReuses);stereoUploadReuses=0;LOG("Sprite view cull normal_visible=%u normal_tested=%u",normalSpriteDrawn,normalSpriteTested);normalSpriteDrawn=normalSpriteTested=0;LOG("Sprite grouping estimate uploaded_ranges=%llu groups6_same_material=%llu",(unsigned long long)streamCandidateRanges,(unsigned long long)streamCandidateGroups6);streamCandidateRanges=streamCandidateGroups6=0;sprite_atlas::report();sprite_material::reportDraws();reportWeather(perfFrames);reportSpriteLights();reportVertexLod();spriteApi.report();spriteWait.report(perfFrames);spriteDetail.report();stageProfile.report(perfFrames);spriteProfile.report();gpuProfile.report();hd::report();reportReflectionWater();reportReflectionSprites(perfFrames);LOG("Reflection cull terrain=%u/%u models=%u/%u objects=%u/%u fx=%u/%u stage_ms=%.3f,%.3f,%.3f,%.3f",reflectTerrainDrawn,reflectTerrainTested,reflectModelsDrawn,reflectModelsTested,reflectObjectsDrawn,reflectObjectsTested,reflectFXDrawn,reflectFXTested,reflectTerrainCPU*1000/perfFrames,reflectModelsCPU*1000/perfFrames,reflectSpritesCPU*1000/perfFrames,reflectFXCPU*1000/perfFrames);reflectTerrainDrawn=reflectTerrainTested=reflectModelsDrawn=reflectModelsTested=reflectObjectsDrawn=reflectObjectsTested=reflectFXDrawn=reflectFXTested=0;reflectTerrainCPU=reflectModelsCPU=reflectSpritesCPU=reflectFXCPU=0;perfReflection=0;reflectionPasses=0;perfIPC=perfInput=perfSurface=perfScene=perfFX=perfControls=0;spriteUploads=markerBuilds=0;perfStart=now;perfFrames=0;}}frames++;}if(benchmarkPose.enabled)LOG("Benchmark END tick=%u reason=vr_exit",tick);if(peer>=0)close(peer);if(listener>=0)close(listener);if(shadowDebugEnabled)sunAngleAdjustment.record("exit",seconds());LOG("VR transition cleanup begin");releaseTerrainNormals();bloom::release();stageProfile.end();gpuProfile.release();releaseWaterVisibility();sprite_pbr::clearSky();sprite_atlas::clear();sprite_material::clear();hd::stop();xrDestroySpace(space);space=XR_NULL_HANDLE;for(auto&s:swaps){s.msaa.destroy();glDeleteRenderbuffers(1,&s.depth);xrDestroySwapchain(s.handle);}glDeleteFramebuffers(1,&fbo);xrDestroySession(session);session=XR_NULL_HANDLE;xrDestroyInstance(instance);instance=XR_NULL_HANDLE;eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);eglDestroySurface(display,surface);eglDestroyContext(display,context);/* EGLDisplay is shared with the compatibility panel; do not terminate it here. */a->activity->vm->DetachCurrentThread();jniAttached=false;}catch(const std::exception&e){if(eglGetCurrentContext()!=EGL_NO_CONTEXT){releaseTerrainNormals();bloom::release();releaseWaterVisibility();}sprite_pbr::clearSky();sprite_atlas::clear();sprite_material::clear();hd::stop();if(eglGetCurrentContext()!=EGL_NO_CONTEXT){stageProfile.end();gpuProfile.release();}if(space)xrDestroySpace(space);if(session)xrDestroySession(session);if(instance)xrDestroyInstance(instance);auto failedDisplay=eglGetCurrentDisplay();auto failedContext=eglGetCurrentContext();auto failedSurface=eglGetCurrentSurface(EGL_DRAW);if(failedDisplay!=EGL_NO_DISPLAY){eglMakeCurrent(failedDisplay,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);if(failedSurface!=EGL_NO_SURFACE)eglDestroySurface(failedDisplay,failedSurface);if(failedContext!=EGL_NO_CONTEXT)eglDestroyContext(failedDisplay,failedContext);}if(jniAttached)a->activity->vm->DetachCurrentThread();LOG("FATAL %s",e.what());}loadingApp=nullptr;LOG("VR transition finish request");ANativeActivity_finish(a->activity);}
