#include <cstdio>
#include <cassert>
#include <limits>
#include <sys/stat.h>
#include <unistd.h>
struct XrVector3f{float x,y,z;};struct XrQuaternionf{float x,y,z,w;};struct XrPosef{XrQuaternionf orientation;XrVector3f position;};
struct XrFovf{float angleLeft,angleRight,angleUp,angleDown;};struct XrView{XrPosef pose;XrFovf fov;};
#include "../src/vr/benchmark_pose.h"
static void flag(const std::string&root,const char*name,bool on){FILE*f=fopen((root+name).c_str(),"wb");assert(f);fputc(on?'1':'0',f);assert(!fclose(f));}
int main(){
 BenchmarkPose clicks;assert(!clicks.leftClick(true,true));assert(!clicks.leftClick(true,false));assert(clicks.leftClick(true,true));assert(clicks.consumeLeftUntilRelease);assert(!clicks.leftClick(true,true));assert(!clicks.leftClick(true,false));assert(!clicks.consumeLeftUntilRelease);assert(clicks.leftClick(true,true));assert(!clicks.leftClick(false,false));assert(!clicks.leftClick(true,true));assert(!clicks.leftClick(true,false));assert(clicks.leftClick(true,true));
 char dir[]="/data/local/tmp/myth-benchmark-test-XXXXXX";assert(mkdtemp(dir));std::string root=dir;
 XrView v[2]{};v[0].pose.position.y=7;benchmarkPose.views(v);assert(v[0].pose.position.y==7);
 flag(root,"/benchmark-pose.enabled",true);benchmarkPose.load(dir,"level-a");assert(benchmarkPose.enabled&&!benchmarkPose.loaded&&benchmarkPose.reason=="missing");
 benchmarkPose.views(v);assert(v[0].pose.position.x==-.032f&&v[1].pose.position.x==.032f&&v[0].pose.orientation.w==1);
 auto l=benchmarkPose.hand(true),r=benchmarkPose.hand(false);assert(l.position.x==-r.position.x);
 BenchmarkPose::Saved record{};strcpy(record.level,"level-a");record.tick=1234;record.camera[0]=17;record.camera[1]=8;record.camera[2]=-9;record.yaw=.75f;record.height=4;
 for(int i=0;i<2;i++){record.eyes[i].pose.orientation={0,0,.6f,.8f};record.eyes[i].pose.position={float(i),2,3};record.eyes[i].fov={-.7f,.8f,.9f,-.6f};}
 for(int i=0;i<4;i++){record.hands[i].orientation.w=1;record.hands[i].position={float(i),5,6};}
 assert(benchmarkPose.save(record));BenchmarkPose restored;restored.load(dir,"level-a");assert(restored.loaded&&restored.enabled&&restored.saved.tick==1234&&restored.saved.camera[0]==17);
 restored.views(v);assert(v[1].pose.position.x==1&&v[0].pose.orientation.z==.6f&&v[1].fov.angleUp==.9f);
 for(int i=0;i<4;i++)assert(restored.hand(i&1,i>=2).position.x==i);
 record.tick=4321;assert(restored.save(record));restored.load(dir,"level-a");assert(restored.saved.tick==4321);
 restored.load(dir,"level-b");assert(restored.enabled&&!restored.loaded&&restored.reason=="level_mismatch");restored.views(v);assert(v[0].pose.position.y==0);
 restored.load(dir,"level-a");record.camera[0]=std::numeric_limits<float>::quiet_NaN();assert(!restored.save(record));restored.load(dir,"level-a");assert(restored.loaded&&restored.saved.tick==4321);
 FILE*f=fopen((root+"/benchmark-pose.bin").c_str(),"wb");fputs("broken",f);fclose(f);restored.load(dir,"level-a");assert(!restored.loaded&&restored.enabled&&restored.reason=="invalid");
 flag(root,"/benchmark-record.enabled",true);restored.load(dir,"level-a");assert(restored.enabled&&!restored.recording);
 flag(root,"/benchmark-pose.enabled",false);restored.load(dir,"level-a");assert(!restored.enabled&&restored.recording);
 v[0].pose.position.y=7;restored.views(v);assert(v[0].pose.position.y==7);
 for(auto n:{"/benchmark-pose.bin","/benchmark-pose.enabled","/benchmark-record.enabled"})remove((root+n).c_str());rmdir(dir);
 puts("PASS: fallback, persistence/overwrite, stereo FOV, four controller poses, mismatch/corruption, invalid save preservation, exclusive modes");
}
