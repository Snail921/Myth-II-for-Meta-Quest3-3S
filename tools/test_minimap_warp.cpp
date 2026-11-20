#include "../src/vr/main.cpp"
#include <cassert>
int main(){
 wristMapShown=wristMapPoseValid=true;wristMapTexture=1;wristMapAspect=.7f;wristMapPose.orientation.w=1;wristMapViewer={0,1,0};
 for(bool rotation:{false,true})for(float pitch:{-.785398f,0.f,.785398f})for(float yaw:{0.f,.7f,1.5708f,3.14159f})for(float size:{.5f,1.f,2.f})for(float side:{-1.f,1.f}){
  cameraSpeeds.mapRotation=rotation;wristMapForward={sinf(.4f),0,-cosf(.4f)};
  cameraSpeeds.mapPitch=pitch;cameraSpeeds.mapSize=size;cameraYaw=yaw;wristMapViewer.y=side;
  auto m=wristMapPlane();
  Vec3 expected=rotation?turn(wristMapForward,cameraYaw):Vec3{0,0,-1};
  assert(fabsf(m.forward.x-expected.x)<.00001f&&fabsf(m.forward.z-expected.z)<.00001f);
  Vec3 heading=turn(wristMapForward,cameraYaw),arrow=wristMapHeadingPoint(-heading.x,heading.z,m.forward);
  if(rotation){assert(fabsf(arrow.x)<.0001f&&fabsf(arrow.y-1)<.0001f);}
  else{assert(fabsf(arrow.x-heading.x)<.0001f&&fabsf(arrow.y+heading.z)<.0001f);}
  for(float u:{.05f,.5f,.95f})for(float v:{.05f,.5f,.95f}){
   auto p=m.local(u,v);Vec3 target=add(m.center,add(times(m.right,p.x),times(m.up,p.y)));
   Vec3 n{m.right.y*m.up.z-m.right.z*m.up.y,m.right.z*m.up.x-m.right.x*m.up.z,m.right.x*m.up.y-m.right.y*m.up.x};
   Vec3 hit;float a,b;assert(hitWristMap(add(target,n),times(n,-1),hit,a,b,false));assert(fabsf(a-u)<.0001&&fabsf(b-v)<.0001);
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
 puts("PASS fixed/rotating map and camera arrow, hit roundtrip 1296 poses/UVs, pitch/heading/backface/size, terrain above 300m, HMD clearance, edges, failed warp preserves camera");
 // Pen freezes world heading while both rig yaw and HMD heading move.
 mapPenEnabled=true;mapPenForward=normalized(Vec3{.6f,0,-.8f});cameraSpeeds.mapRotation=true;
 for(float yaw:{0.f,1.f,3.f}){cameraYaw=yaw;wristMapForward={1,0,0};auto m=wristMapPlane(true);
  assert(fabsf(m.forward.x-.6f)<.0001f&&fabsf(m.forward.z+.8f)<.0001f);
  auto q=m.local(.23f,.81f);Vec3 t=add(m.center,add(times(m.right,q.x),times(m.up,q.y)));
  Vec3 n{m.right.y*m.up.z-m.right.z*m.up.y,m.right.z*m.up.x-m.right.x*m.up.z,m.right.x*m.up.y-m.right.y*m.up.x},hit;float u,v;
  assert(hitWristMap(add(t,n),times(n,-1),hit,u,v,true)&&fabsf(u-.23f)<.0001f&&fabsf(v-.81f)<.0001f);
 }
 mapPenEnabled=false;assert(fabsf(wristMapPlane().forward.x-turn(wristMapForward,cameraYaw).x)<.0001f);
 puts("PASS pen world-heading freeze, rotated hit coordinates, rotation resume");

 // Both original toolbar icons must be hit-testable when capture enables live mode.
 mapPen.data.fill(0);mapPen.data[0]=1;mapPen.updated=seconds();infoPanelShown=true;originalDialogMode=0;
 auto m=wristMapPlane(true);float bottom=infoPanelBottom(m),scl=cameraSpeeds.mapSize;
 for(int button:{3,4}){float y=bottom+(button==3?.0265f:.0135f)*scl;
  auto t=add(m.center,add(times(m.right,(.158f-infoPanelCenter)*scl),times(m.up,y)));
  Vec3 n{m.right.y*m.up.z-m.right.z*m.up.y,m.right.z*m.up.x-m.right.x*m.up.z,m.right.x*m.up.y-m.right.y*m.up.x},hit;
  assert(hitSelectionPanel(add(t,n),times(n,-1),hit)&&infoPanelButton==button);
 }
 puts("PASS live multiplayer pen and eraser hit regions");

 // Exercise the renderer-to-engine wire with real socket I/O and acknowledgements.
 int sockets[2];assert(socketpair(AF_UNIX,SOCK_STREAM,0,sockets)==0);
 peer=sockets[0];authenticated=true;requestPending=0;commandQueue.clear();
 mapPenEnabled=true;mapPenStroke=false;mapPenX=mapPenY=-1;
 float rx=149,ry=128;memcpy(mapPen.data.data()+16,&rx,4);memcpy(mapPen.data.data()+20,&ry,4);mapPen.updated=seconds();
 uint32_t packet[8];
 mapPenPoint(.25f,.5f);assert(recv(sockets[1],packet,sizeof(packet),0)==sizeof(packet));
 assert(packet[2]==25&&packet[3]==2&&packet[5]==112&&packet[6]==64);acknowledge(packet[1],1);
 mapPenLastPoint=0;mapPenPoint(.5f,.75f);assert(recv(sockets[1],packet,sizeof(packet),0)==sizeof(packet));
 assert(packet[2]==25&&packet[3]==2&&packet[5]==75&&packet[6]==96);acknowledge(packet[1],1);
 endMapPenStroke();assert(recv(sockets[1],packet,sizeof(packet),0)==sizeof(packet));
 assert(packet[2]==25&&packet[3]==3&&!mapPenStroke);acknowledge(packet[1],1);
 close(sockets[0]);close(sockets[1]);peer=-1;authenticated=false;
 puts("PASS pen UV conversion, point transmission, ACK continuation and stroke end");

}
