#pragma once
#include <cmath>
// Shared eye-center facing basis: up = cross(toward, right), not its mirrored tilt.
template<class V> static bool snowBillboard(const V&t,V&right,V&up){
 float length=std::sqrt(t.x*t.x+t.y*t.y+t.z*t.z);if(length<.001f)return false;
 float horizontal=std::sqrt(t.x*t.x+t.z*t.z);
 right=horizontal>.001f?V{t.z/horizontal,0,-t.x/horizontal}:V{1,0,0};
 up=V{(t.y*right.z-t.z*right.y)/length,(t.z*right.x-t.x*right.z)/length,(t.x*right.y-t.y*right.x)/length};return true;
}
