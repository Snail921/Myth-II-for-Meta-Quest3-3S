
#include <cassert>
#include <cstdio>
#include "../src/vr/interaction_math.h"
struct XrQuaternionf {float x,y,z,w;};
struct XrPosef {XrQuaternionf orientation;Vec3 position;};
static Vec3 rotated(XrQuaternionf q,Vec3 v){Vec3 u{q.x,q.y,q.z};Vec3 cross{u.y*v.z-u.z*v.y,u.z*v.x-u.x*v.z,u.x*v.y-u.y*v.x};Vec3 cross2{u.y*cross.z-u.z*cross.y,u.z*cross.x-u.x*cross.z,u.x*cross.y-u.y*cross.x};return add(v,add(times(cross,2*q.w),times(cross2,2)));}

#include "../src/vr/hand_pointer.h"
static void near(Vec3 a,Vec3 b){assert(dot(sub(a,b),sub(a,b))<.000001f);}
int main(){
 // Realistic grip pitch plus an asymmetric local aim yaw and translation.
 XrPosef rightGrip{{.38268343f,0,0,.92387953f},{.3f,1,-.2f}};
 XrPosef local{{.17364818f,0,0,.98480775f},{.02f,.03f,-.05f}};
 auto rightAim=applyHandOffset(rightGrip,local);
 auto offset=leftAimOffset(rightGrip,rightAim);
 XrPosef leftGrip=rightGrip;leftGrip.position.x=-.3f;
 auto left=applyHandOffset(leftGrip,offset);
 auto r=rotated(rightAim.orientation,{0,0,-1}),l=rotated(left.orientation,{0,0,-1});
 near(l,{-r.x,r.y,r.z});near({left.position.x,left.position.y,left.position.z},{-rightAim.position.x,rightAim.position.y,rightAim.position.z});
 // Reflection also handles nonzero yaw/roll, rather than negating pitch.
 local.orientation={0,.25881904f,0,.96592583f};rightAim=applyHandOffset(rightGrip,local);
 left=applyHandOffset(leftGrip,leftAimOffset(rightGrip,rightAim));
 r=rotated(rightAim.orientation,{0,0,-1});l=rotated(left.orientation,{0,0,-1});near(l,{-r.x,r.y,r.z});
 puts("PASS mirrored grip-to-aim pitch/yaw and translated ray origin");
}
