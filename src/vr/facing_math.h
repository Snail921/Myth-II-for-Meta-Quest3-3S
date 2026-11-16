#pragma once
#include "interaction_math.h"
// World X is reflected relative to Myth X. Encoded bit16 distinguishes east from auto.
static unsigned destinationFacing(Vec3 start,Vec3 end){
 float dx=-(end.x-start.x),dy=end.z-start.z;
 if(dx*dx+dy*dy<.04f)return 0;
 float angle=atan2f(dy,dx);if(angle<0)angle+=6.28318530718f;
 return 0x10000u|(unsigned(lroundf(angle*(65536.f/6.28318530718f)))&65535u);
}
static unsigned facingDistance(unsigned a,unsigned b){unsigned d=(a-b)&65535u;return std::min(d,65536u-d);}
