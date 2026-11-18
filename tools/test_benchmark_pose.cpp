#include <cstdio>
#include <cassert>
struct XrVector3f{float x,y,z;};struct XrQuaternionf{float x,y,z,w;};struct XrPosef{XrQuaternionf orientation;XrVector3f position;};struct XrView{XrPosef pose;};
#include "../src/vr/benchmark_pose.h"
int main(){XrView v[2]{};v[0].pose.position.y=7;benchmarkPose.views(v);assert(v[0].pose.position.y==7);benchmarkPose.enabled=true;for(int i=0;i<100;i++){v[0].pose.position={float(i),8,9};v[1].pose.orientation={1,2,3,4};benchmarkPose.views(v);assert(v[0].pose.position.x==-.032f&&v[1].pose.position.x==.032f);assert(v[0].pose.position.y==0&&v[1].pose.orientation.w==1);}auto l=benchmarkPose.hand(true),r=benchmarkPose.hand(false);assert(l.position.x==-r.position.x&&l.position.z==r.position.z);puts("PASS: disabled unchanged; fixed stereo and hands deterministic");}
