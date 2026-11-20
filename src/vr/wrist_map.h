// Original overhead image plus the same live unit visibility used by the VR scene.
static void loadInfoFont(android_app*);
static void initOriginalDialogs(android_app*);
static GLuint wristMapTexture=0;
static float wristMapAspect=1;
static void loadWristMap(android_app*a){
 mapPenEnabled=mapPenStroke=false;mapPenMesh=0;wristMapTexture=0;wristMapPoseValid=false;wristMapShown=true;infoPanelShown=true;loadInfoFont(a);initOriginalDialogs(a);
 try{
  auto b=asset(a,"minimap.rgba");if(b.size()<8)throw std::runtime_error("map header");
  unsigned w=u32(b.data()),h=u32(b.data()+4);
  if(!w||!h||w>1024||h>1024||b.size()!=8+size_t(w)*h*4)throw std::runtime_error("map size");
  wristMapAspect=float(h)/w;glGenTextures(1,&wristMapTexture);glBindTexture(GL_TEXTURE_2D,wristMapTexture);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,b.data()+8);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  LOG("Wrist map loaded %ux%u",w,h);
 }catch(const std::exception&e){LOG("Wrist map unavailable: %s",e.what());}
}
// Choose a readable face once from the shared head center, never per eye.
static Vec3 wristMapReadableRight(const XrPosef&hand,Vec3 center,Vec3 viewer){
 Vec3 right=rotated(hand.orientation,{1,0,0}),normal=rotated(hand.orientation,{0,cosf(cameraSpeeds.mapPitch),sinf(cameraSpeeds.mapPitch)});
 return dot(normal,sub(viewer,center))<0?times(right,-1):right;
}
static Vec3 wristMapHeadingPoint(float originalX,float originalY,Vec3 forward){
 Vec3 world{-originalX,0,originalY},right{-forward.z,0,forward.x};
 return {dot(world,right),dot(world,forward),0};
}
struct WristMapPlane {
 Vec3 center,right,up,forward;float width,height;
 Vec3 local(float u,float v)const{return wristMapHeadingPoint((u-.5f)*width,(v-.5f)*height,forward);}
};
static WristMapPlane wristMapPlane(bool tracking=false){
 XrPosef hand=tracking?wristMapPose:worldPose(wristMapPose);
 Vec3 center=add({hand.position.x,hand.position.y,hand.position.z},rotated(hand.orientation,{0,.08f,.10f}));
 Vec3 viewer=tracking?wristMapViewer:add(camera,turn(wristMapViewer,cameraYaw));
 float width=.15f*cameraSpeeds.mapSize;
 return {center,wristMapReadableRight(hand,center,viewer),rotated(hand.orientation,{0,sinf(cameraSpeeds.mapPitch),-cosf(cameraSpeeds.mapPitch)}),mapPenEnabled?mapPenForward:(cameraSpeeds.mapRotation?turn(wristMapForward,cameraYaw):Vec3{0,0,-1}),width,width*wristMapAspect};
}
#include "selection_panel.h"
#include "original_dialog.h"
#include "multiplayer_hud_draw.h"
static bool hitWristMap(Vec3 origin,Vec3 direction,Vec3&hit,float&u,float&v,bool tracking){
 if(!wristMapShown||!wristMapPoseValid||!wristMapTexture||radial.kind)return false;
 auto m=wristMapPlane(tracking);Vec3 n{m.right.y*m.up.z-m.right.z*m.up.y,m.right.z*m.up.x-m.right.x*m.up.z,m.right.x*m.up.y-m.right.y*m.up.x};
 float denominator=dot(direction,n);if(fabsf(denominator)<.00001f)return false;
 float t=dot(sub(m.center,origin),n)/denominator;if(t<=0||!std::isfinite(t))return false;
 hit=add(origin,times(direction,t));Vec3 delta=sub(hit,m.center);float x=dot(delta,m.right),y=dot(delta,m.up);
 // Inverse heading transform. Texture-U flip belongs only to the artwork.
 u=.5f+(m.forward.z*x-m.forward.x*y)/m.width;
 v=.5f+(m.forward.x*x+m.forward.z*y)/m.height;
 if(tracking)hit=add(camera,turn(hit,cameraYaw));
 return std::isfinite(u)&&std::isfinite(v)&&u>=0&&u<=1&&v>=0&&v<=1;
}
static bool warpFromWristMap(float u,float v,const XrPosef&head){
 if(scriptCameraBlocked||!std::isfinite(u)||!std::isfinite(v)||u<0||u>1||v<0||v>1||terrain.width<3||terrain.height<3)return false;
 float gx=std::clamp(u*levelWidth,.5f,terrain.width-1.5f),gy=std::clamp(v*levelHeight,.5f,terrain.height-1.5f);
 Vec3 surface;if(!terrain.rayDownFromAbove(mythWorldX(gx,originX,scale),(gy-originY)*scale,surface)){return false;}
 Vec3 headOffset=turn({head.position.x,head.position.y,head.position.z},cameraYaw);
 if(!std::isfinite(headOffset.x)||!std::isfinite(headOffset.y)||!std::isfinite(headOffset.z)||!std::isfinite(cameraGroundOffset))return false;
 // Preserve the rig's ground offset; compensate horizontal room-scale tracking,
 // and guarantee the actual HMD remains above the ray's surface intersection.
 Vec3 next{surface.x-headOffset.x,surface.y+std::max(cameraGroundOffset,.2f-headOffset.y),surface.z-headOffset.z};
 Vec3 delta=sub(next,camera);followCamera.active=false;orbitCamera.active=false;camera=next;cameraGroundOffset=next.y-surface.y;
 pointerStart=add(pointerStart,delta);pointerEnd=add(pointerEnd,delta);mapPointerHit=add(mapPointerHit,delta);
 static double lastLog=0;if(seconds()-lastLog>.5){lastLog=seconds();LOG("Minimap warp game=%.2f,%.2f ground=%.3f headClearance=%.3f",gx,gy,surface.y,camera.y+headOffset.y-surface.y);}return true;
}
static void drawWristMap(const XrPosef&eye){
 if(!wristMapShown||!wristMapPoseValid||!wristMapTexture||radial.kind==2)return;
 auto plane=wristMapPlane();float width=plane.width,height=plane.height;auto center=plane.center,right=plane.right,up=plane.up;
 auto local=[&](float u,float v){return plane.local(u,v);};
 auto panelPoint=[&](float x,float y,float u=0,float v=0){Vec3 p=add(center,add(times(right,x),times(up,y)));return Vertex{p.x,p.y,p.z,u,v};};
 // Original overhead art is stored with horizontal axis opposite to mesh x.
 // Flip texture U only: actor and camera positions already use mesh coordinates.
 auto point=[&](float u,float v){auto p=local(u,v);return panelPoint(p.x,p.y,1-u,v);};
 auto quad=[&](std::vector<Vertex>&out,float u,float v,float du,float dv){auto a=point(u-du,v-dv),b=point(u+du,v-dv),c=point(u-du,v+dv),d=point(u+du,v+dv);out.insert(out.end(),{a,c,b,b,c,d});};
 attributes(vbo);glDisable(GL_DEPTH_TEST);glDepthMask(GL_FALSE);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
 glUniform1f(glGetUniformLocation(program,"alphaCutoff"),.003f);
 auto drawMesh=[&](const std::vector<Vertex>&v,GLuint texture,float r,float g,float b){if(v.empty())return;glBindTexture(GL_TEXTURE_2D,texture);glUniform4f(glGetUniformLocation(program,"tint"),r,g,b,1);glBufferData(GL_ARRAY_BUFFER,v.size()*sizeof(Vertex),v.data(),GL_STREAM_DRAW);glDrawArrays(GL_TRIANGLES,0,v.size());};
 std::vector<Vertex> background;quad(background,.5f,.5f,.515f,.5f+.015f/wristMapAspect);drawMesh(background,whiteTexture,.08f,.1f,.12f);
 background.clear();quad(background,.5f,.5f,.5f,.5f);drawMesh(background,wristMapTexture,1,1,1);
 // Original byte coordinates follow the artwork X axis, opposite mesh X.
 if(seconds()-mapPen.updated<3&&mapPen.rangeX()>0&&mapPen.rangeY()>0){
  auto*p=mapPen.data.data();unsigned count=u32(p+4);int team=int(u32(p+8))-1;bool previous=false;float pu=0,pv=0;
  auto line=[&](float u,float v,float uu,float vv,const unsigned char*color){
   auto a=local(u,v),b=local(uu,vv);float dx=b.x-a.x,dy=b.y-a.y,len=sqrtf(dx*dx+dy*dy);if(len<.000001f)return;
   float r=.00045f*cameraSpeeds.mapSize,ox=-dy*r/len,oy=dx*r/len;
   drawMesh({panelPoint(a.x+ox,a.y+oy),panelPoint(a.x-ox,a.y-oy),panelPoint(b.x+ox,b.y+oy),panelPoint(a.x-ox,a.y-oy),panelPoint(b.x-ox,b.y-oy),panelPoint(b.x+ox,b.y+oy)},whiteTexture,color[0]/255.f,color[1]/255.f,color[2]/255.f);
  };
  for(unsigned i=0;i<count;i++){unsigned x=p[160+i*2],y=p[161+i*2];if(!x){previous=false;if(y)team=int(y)-2;continue;}
   float u=std::clamp(1-x/mapPen.rangeX(),0.f,1.f),v=std::clamp(y/mapPen.rangeY(),0.f,1.f);
   unsigned char fallback[4]={255,64,64,255};const auto*color=team>=0&&team<32?p+32+team*4:fallback;
   if(previous)line(pu,pv,u,v,color);pu=u;pv=v;previous=true;
  }
  if(mapPenEnabled){const unsigned char red[4]={255,32,32,255};line(0,0,1,0,red);line(1,0,1,1,red);line(1,1,0,1,red);line(0,1,0,0,red);}
 }
 std::vector<Vertex> markers[4];
 const float dot=.0004f*cameraSpeeds.mapSize;
 for(const auto&o:objects){
  if(!o.unitID||!objectVisible(o)||o.health<=0||o.x<0||o.y<0||o.x>levelWidth||o.y>levelHeight)continue;
  unsigned group=o.selected?0:o.controllable?1:(o.friendly||o.team<0)?3:2;
  quad(markers[group],o.x/levelWidth,o.y/levelHeight,dot/width,dot/height);
 }
 const float colors[4][3]={{ui_contrast::red(),ui_contrast::green(),ui_contrast::blue()},{.1f,1,.2f},{1,.15f,.1f},{.1f,.4f,1}};
 for(int i=0;i<4;i++)drawMesh(markers[i],whiteTexture,colors[i][0],colors[i][1],colors[i][2]);
 // Original map x is reversed relative to OpenXR world x; map y follows world z.
 Vec3 headWorld=add(camera,turn(wristMapViewer,cameraYaw));
 float u=std::clamp((originX-headWorld.x/scale)/levelWidth,0.f,1.f),v=std::clamp((originY+headWorld.z/scale)/levelHeight,0.f,1.f);
 auto marker=local(u,v);float size=.006f*cameraSpeeds.mapSize;
 // With a fixed map, rotate the camera marker instead of the artwork.
 Vec3 heading=turn(wristMapForward,cameraYaw);
 Vec3 direction=wristMapHeadingPoint(-heading.x,heading.z,plane.forward);
 float px[3]={0,-.5f,.5f},py[3]={1,-.5f,-.5f};
 for(int i=0;i<3;i++){float x=px[i],y=py[i];px[i]=x*direction.y+y*direction.x;py[i]=-x*direction.x+y*direction.y;}
 std::vector<Vertex> arrow;constexpr float inner=.70f;
 for(int i=0;i<3;i++){int j=(i+1)%3;
  auto a=panelPoint(marker.x+px[i]*size,marker.y+py[i]*size),b=panelPoint(marker.x+px[j]*size,marker.y+py[j]*size);
  auto c=panelPoint(marker.x+px[i]*size*inner,marker.y+py[i]*size*inner),d=panelPoint(marker.x+px[j]*size*inner,marker.y+py[j]*size*inner);
  arrow.insert(arrow.end(),{a,b,c,b,d,c});
 }
 drawMesh(arrow,whiteTexture,ui_contrast::red(),ui_contrast::green(),ui_contrast::blue());
 if(mapHovered&&pointerValid){
  std::vector<Vertex> line{{pointerStart.x,pointerStart.y,pointerStart.z,0,0},{mapPointerHit.x,mapPointerHit.y,mapPointerHit.z,0,0}};
  auto p=local(mapPointerU,mapPointerV);float r=.002f*cameraSpeeds.mapSize;
  for(int i=0;i<24;i++){float a=i*6.2831853f/24,b=(i+1)*6.2831853f/24;line.push_back(panelPoint(p.x+cosf(a)*r,p.y+sinf(a)*r));line.push_back(panelPoint(p.x+cosf(b)*r,p.y+sinf(b)*r));}
  glBindTexture(GL_TEXTURE_2D,whiteTexture);glUniform4f(glGetUniformLocation(program,"tint"),1,mapWarpRejected?.15f:1.f,0,1);glBufferData(GL_ARRAY_BUFFER,line.size()*sizeof(Vertex),line.data(),GL_STREAM_DRAW);{pointer_style::Lines width;glDrawArrays(GL_LINES,0,2);}if(line.size()>2)glDrawArrays(GL_LINES,2,line.size()-2);
 }
 auto drawMapNotice=[&](const std::string& text,bool right,float row=0){float pixel=.00022f*cameraSpeeds.mapSize,total=0;
  for(unsigned char c:text)total+=infoAdvance[c]*pixel;
  float pen=right?width*.5f-.003f*cameraSpeeds.mapSize-total:-width*.5f+.003f*cameraSpeeds.mapSize,top=height*.5f-(.002f+row*.008f)*cameraSpeeds.mapSize;
  std::vector<Vertex> glyphs;
  for(unsigned char ch:text){float u=(ch%16)/16.f,v=(ch/16)/16.f;
   auto a=panelPoint(pen-4*pixel,top,u,v),b=panelPoint(pen+28*pixel,top,u+1.f/16,v),c=panelPoint(pen-4*pixel,top-32*pixel,u,v+1.f/16),d=panelPoint(pen+28*pixel,top-32*pixel,u+1.f/16,v+1.f/16);
   glyphs.insert(glyphs.end(),{a,b,c,b,d,c});pen+=infoAdvance[ch]*pixel;
  }
  // Eight neighboring copies form a dark grey outline, then the white face.
  std::vector<Vertex> outline;outline.reserve(glyphs.size()*8);
  for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++)if(dx||dy){
   auto a=panelPoint(0,0),b=panelPoint(dx*pixel,dy*pixel);Vec3 offset{b.x-a.x,b.y-a.y,b.z-a.z};
   for(auto v:glyphs){v.x+=offset.x;v.y+=offset.y;v.z+=offset.z;outline.push_back(v);}
  }
  drawMesh(outline,infoFont,.10f,.10f,.10f);
  drawMesh(glyphs,infoFont,1,1,1);
 };
 if(infoFont&&!originalDialogMode){
  if(shadowDebugEnabled&&(sunAngleAdjustment.active||seconds()<sunAngleAdjustment.noticeUntil))drawMapNotice(sunAngleAdjustment.label(),false,1);
  if(fpsDisplay.enabled)drawMapNotice(fpsDisplay.label(),false);
  if(gameSpeed.value>=0&&seconds()<gameSpeed.until)drawMapNotice(std::string("Game Speed ")+gameSpeed.label(),true);
 }
 drawSelectionPanel();drawMultiplayerHud();drawOriginalDialog();
 glDepthMask(GL_TRUE);glEnable(GL_DEPTH_TEST);glDisable(GL_BLEND);glUniform1f(glGetUniformLocation(program,"alphaCutoff"),.5f);glUniform4f(glGetUniformLocation(program,"tint"),1,1,1,1);
}
