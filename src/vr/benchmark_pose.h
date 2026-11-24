#pragma once
#include <cstdio>
#include <cstring>
#include <cmath>
#include <string>
#include <cstdint>
// XR-local eyes/hands plus game-camera transform; no process-local XR pointers.
struct BenchmarkPose {
 struct Eye {XrPosef pose{};XrFovf fov{};};
 struct Saved {
  char magic[8]={'M','Y','T','H','B','P','0','1'};
  char level[128]{};uint32_t tick=0;
  float camera[3]{},yaw=0,height=0;
  Eye eyes[2]{};XrPosef hands[4]{}; // right grip, left grip, right aim, left aim
 } saved{};
 bool enabled=false,recording=false,loaded=false,capturePending=false,lastLeft=true;
 bool consumeLeftUntilRelease=false;std::string notice;double noticeUntil=0;
 bool fixedMode=false,started=false;std::string root,level,reason="missing";
 XrView tracked[2]{};
 static bool validPose(const XrPosef&p){
  const auto&q=p.orientation;const auto&v=p.position;
  if(!std::isfinite(v.x)||!std::isfinite(v.y)||!std::isfinite(v.z)||fabsf(v.x)>1e6f||fabsf(v.y)>1e6f||fabsf(v.z)>1e6f)return false;
  float n=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;return std::isfinite(n)&&fabsf(n-1)<.02f;
 }
 static bool valid(const Saved&s){
  if(memcmp(s.magic,"MYTHBP01",8)||!memchr(s.level,0,sizeof(s.level)))return false;
  for(float v:s.camera)if(!std::isfinite(v)||fabsf(v)>1e6f)return false;
  if(!std::isfinite(s.yaw)||!std::isfinite(s.height))return false;
  for(auto&e:s.eyes){auto&f=e.fov;if(!validPose(e.pose)||!std::isfinite(f.angleLeft)||!std::isfinite(f.angleRight)||!std::isfinite(f.angleUp)||!std::isfinite(f.angleDown)||f.angleLeft>=f.angleRight||f.angleDown>=f.angleUp||f.angleLeft<=-1.57f||f.angleRight>=1.57f||f.angleDown<=-1.57f||f.angleUp>=1.57f)return false;}
  for(auto&p:s.hands)if(!validPose(p))return false;return true;
 }
 // Rising-edge only; a held diagnostic click stays consumed after unlock.
 bool leftClick(bool active,bool pressed){
  if(!active){lastLeft=true;return false;}
  if(!pressed)consumeLeftUntilRelease=false;
  bool edge=pressed&&!lastLeft;lastLeft=pressed;
  if(edge)consumeLeftUntilRelease=true;return edge;
 }
 void load(const char*directory,const std::string&key){
  *this=BenchmarkPose{};root=directory;level=key;
  auto flag=[&](const char*name){FILE*f=fopen((root+name).c_str(),"rb");if(!f)return false;bool on=fgetc(f)=='1';fclose(f);return on;};
  enabled=fixedMode=flag("/benchmark-pose.enabled");recording=!enabled&&flag("/benchmark-record.enabled");
  FILE*f=fopen((root+"/benchmark-pose.bin").c_str(),"rb");if(!f)return;
  Saved candidate{};bool ok=fread(&candidate,1,sizeof(candidate),f)==sizeof(candidate);int tail=fgetc(f);fclose(f);
  if(!ok||tail!=EOF||!valid(candidate)){reason="invalid";return;}
  if(key!=candidate.level){reason="level_mismatch";return;}
  saved=candidate;loaded=true;reason="saved";
 }
 bool save(const Saved&value){
  if(!valid(value)||level!=value.level)return false;
  std::string path=root+"/benchmark-pose.bin",tmp=path+".tmp";
  FILE*f=fopen(tmp.c_str(),"wb");if(!f)return false;
  bool ok=fwrite(&value,1,sizeof(value),f)==sizeof(value);if(fclose(f))ok=false;
  if(!ok||rename(tmp.c_str(),path.c_str())){remove(tmp.c_str());return false;}
  saved=value;loaded=true;reason="saved";return true;
 }
 void views(XrView*v)const{
  if(!enabled)return;
  for(int i=0;i<2;i++){if(loaded){v[i].pose=saved.eyes[i].pose;v[i].fov=saved.eyes[i].fov;}else{v[i].pose={};v[i].pose.orientation.w=1;v[i].pose.position.x=i?.032f:-.032f;}}
 }
 XrPosef hand(bool left,bool aim=false)const{
  if(loaded)return saved.hands[(aim?2:0)+(left?1:0)];
  XrPosef p{};p.orientation.w=1;p.position={left?-.25f:.25f,-.35f,-.4f};return p;
 }
};
static BenchmarkPose benchmarkPose;
