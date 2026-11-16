#pragma once
#include "interaction_math.h"
// The arrow artwork points toward its left edge. Keep its long axis in the
// world flight direction; rotate only its thin transverse axis toward the HMD.
static Vec3 arrowRight(Vec3 flight){float n=sqrtf(dot(flight,flight));return n>1e-6f?times(flight,-1.f/n):Vec3{1,0,0};}
static Vec3 arrowUp(Vec3 right,Vec3 toCamera){
 auto cross=[](Vec3 a,Vec3 b){return Vec3{a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};};
 Vec3 up=cross(toCamera,right);float n=sqrtf(dot(up,up));
 if(n<1e-5f){up=sub(Vec3{0,1,0},times(right,right.y));n=sqrtf(dot(up,up));}
 if(n<1e-5f){up=sub(Vec3{1,0,0},times(right,right.x));n=sqrtf(dot(up,up));}
 return times(up,1.f/n);
}
