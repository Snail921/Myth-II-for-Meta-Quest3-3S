#include "../src/vr/main.cpp"
#include <cassert>
int main(){
 wristMapShown=wristMapPoseValid=true;wristMapTexture=1;wristMapAspect=.7f;wristMapPose.orientation.w=1;wristMapViewer={0,1,0};
 for(float pitch:{-.785398f,0.f,.785398f})for(float yaw:{0.f,.7f,1.5708f,3.14159f})for(float size:{.5f,1.f,2.f})for(float side:{-1.f,1.f}){
  cameraSpeeds.mapPitch=pitch;cameraSpeeds.mapSize=size;cameraYaw=yaw;wristMapViewer.y=side;
  auto m=wristMapPlane();
  for(float u:{.05f,.5f,.95f})for(float v:{.05f,.5f,.95f}){
   auto p=m.local(u,v);Vec3 target=add(m.center,add(times(m.right,p.x),times(m.up,p.y)));
   Vec3 n{m.right.y*m.up.z-m.right.z*m.up.y,m.right.z*m.up.x-m.right.x*m.up.z,m.right.x*m.up.y-m.right.y*m.up.x};
   Vec3 hit;float a,b;assert(hitWristMap(add(target,n),times(n,-1),hit,a,b));assert(fabsf(a-u)<.0001&&fabsf(b-v)<.0001);
  }
 }
 terrain.width=terrain.height=4;terrain.scale=scale;terrain.ox=terrain.oy=terrain.oz=originX=originY=originZ=0;terrain.heights={0,0,0,0,0,10,10,0,0,10,600,0,0,0,0,0};levelWidth=levelHeight=4;terrain.rebuildRayCeiling();
 XrPosef head{};head.position={.2f,-1.f,.3f};cameraGroundOffset=.2f;
 assert(warpFromWristMap(.5f,.5f,head));auto h=add(camera,turn({head.position.x,head.position.y,head.position.z},cameraYaw));assert(fabsf(h.x+2)<.001&&fabsf(h.z-2)<.001);assert(h.y>=terrain.elevation(h.x,h.z)+.199f);
 assert(warpFromWristMap(1,1,head));Vec3 before=camera;assert(!warpFromWristMap(-1,0,head));terrain.heights[0]=NAN;terrain.rebuildRayCeiling();assert(!warpFromWristMap(.5f,.5f,head));assert(camera.x==before.x&&camera.y==before.y&&camera.z==before.z);
 // Local pointer/map coordinates are invariant under large camera translations.
 terrain.heights[0]=0;terrain.rebuildRayCeiling();wristMapViewer={0,1,0};
 auto plane=wristMapPlane(true);auto xy=plane.local(.3f,.7f);Vec3 target=add(plane.center,add(times(plane.right,xy.x),times(plane.up,xy.y)));
 Vec3 n{plane.right.y*plane.up.z-plane.right.z*plane.up.y,plane.right.z*plane.up.x-plane.right.x*plane.up.z,plane.right.x*plane.up.y-plane.right.y*plane.up.x};
 for(int i=0;i<100;i++){camera={float(i*100),float(i),-float(i*100)};Vec3 hit;float u,v;assert(hitWristMap(add(target,n),times(n,-1),hit,u,v,true));assert(fabsf(u-.3f)<.0001f&&fabsf(v-.7f)<.0001f);assert(warpFromWristMap(u,v,head));}
 puts("PASS local drag stable through 100 translations");
 puts("PASS map hit roundtrip 648 poses/UVs, pitch/heading/backface/size, terrain above 300m, HMD clearance, edges, failed warp preserves camera");
}
