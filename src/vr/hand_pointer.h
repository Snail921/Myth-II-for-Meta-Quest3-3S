#pragma once
// Use the working right controller's grip-to-aim transform for the mirrored
// left controller. This avoids an independent left aim-pose pitch offset.
static XrQuaternionf handMultiply(XrQuaternionf a,XrQuaternionf b){
 return {a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,
 a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w,a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z};
}
static XrQuaternionf handInverse(XrQuaternionf q){return {-q.x,-q.y,-q.z,q.w};}
static XrPosef leftAimOffset(const XrPosef&rightGrip,const XrPosef&rightAim){
 auto inverse=handInverse(rightGrip.orientation);auto q=handMultiply(inverse,rightAim.orientation);
 auto v=rotated(inverse,{rightAim.position.x-rightGrip.position.x,rightAim.position.y-rightGrip.position.y,rightAim.position.z-rightGrip.position.z});
 XrPosef offset{};offset.orientation={q.x,-q.y,-q.z,q.w};offset.position={-v.x,v.y,v.z};return offset;
}
static XrPosef applyHandOffset(const XrPosef&grip,const XrPosef&offset){
 XrPosef ray{};ray.orientation=handMultiply(grip.orientation,offset.orientation);
 auto v=rotated(grip.orientation,{offset.position.x,offset.position.y,offset.position.z});
 ray.position={grip.position.x+v.x,grip.position.y+v.y,grip.position.z+v.z};return ray;
}
