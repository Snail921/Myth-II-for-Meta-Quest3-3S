#pragma once
#include "reflection_selection.h"
#include "reflection_sprite_lod.h"
struct PreparedSprite {size_t object;unsigned texture;GLuint colored=0;std::array<Vertex,6> vertices;rendercull::Bounds bounds;std::array<float,4> lightBounds{};float clipTop=0,clipHeight=0;int plane=-1;};
struct SpriteViewCache {
 bool ready=false,uploaded=false;std::vector<PreparedSprite> sprites;
 std::vector<Vertex> selectionFrames,friendlyHover,enemyHover,itemHover,healthBars,manaBars;
 std::vector<SpriteStream::Input> inputs;
 SpriteStream stream;
 std::vector<size_t> visible,previousVisible;
 void reset(){ready=uploaded=false;visible.clear();previousVisible.clear();sprites.clear();selectionFrames.clear();friendlyHover.clear();enemyHover.clear();itemHover.clear();healthBars.clear();manaBars.clear();inputs.clear();}

};
// All reflection planes/eyes share ordinary geometry. Axial projectiles have
// one candidate per plane because their orientation depends on reflected Y.
static unsigned reflectionGeometryBuilds=0,reflectionGeometryReuses=0;
static SpriteViewCache spriteViews[2];
static SpriteViewCache& spriteView(bool reflected){return spriteViews[reflected?1:0];}
struct ReflectionCandidateFrame {
 bool ready=false,passReady=false;reflection_selection::SpatialIndex spatial;
 std::vector<std::vector<int8_t>> lod;
 std::vector<std::array<rendercull::Rect,2>> waterRects;
 std::vector<std::array<Mat,2>> lodMatrices;
 std::vector<std::array<rendercull::Frustum,2>> lodFull,lodCropped;
 std::vector<unsigned char> lodNear;
 std::vector<size_t> selected,unionVisible,toInput,inputSelection;
 std::vector<unsigned char> included;
};
static ReflectionCandidateFrame reflectionCandidates;
static unsigned reflectionUnionRepairs=0,reflectionUnionAdded=0;
static unsigned reflectionCandidateFrames=0,reflectionCandidateTests=0,reflectionCandidateSelected=0,reflectionUnionUploaded=0;
static double reflectionIndexCPU=0,reflectionSelectCPU=0,reflectionVertexUploadCPU=0,reflectionElementUploadCPU=0;
static unsigned normalSpriteTested=0,normalSpriteDrawn=0,stereoUploadReuses=0;
static double spriteBatchCPU[2]{};static unsigned spriteBatchPasses[2]{};
static double spritePrepareCPU[2]{};static unsigned spriteBuilds[2]{},spriteReuse[2]{};
// Shared only within a frame: object state, palette and HD readiness are frozen
// for both eyes/modes. Axial projectile orientation remains pass-specific.
struct SpritePreparation {
 bool chosen=false,valid=false,ready=false;
 Quad fallback{},q{};GLuint highTexture=0;
 float px=0,py=0,pz=0,dx=0,dz=0,len=0,rx=0,rz=0;
 float left=0,right=0,bottom=0,top=0,clipTop=0,clipHeight=0,u0=0,u1=0,v0=0,v1=0;
 Vertex a{},b{},c{},d{};rendercull::Bounds coarse;
};
static std::vector<SpritePreparation> spritePreparations;
static unsigned spriteCoarseTested[2]{},spriteCoarseRejected[2]{},spritePreparationBuilt=0,spritePreparationShared=0;
static float spriteQuadRadius(const Quad&q,float objectScale){
 float ps=fabsf(q.scale*(10.f/512.f)*scale*objectScale);
 // L1 radius contains any rotation, flipped origin and axial billboard.
 return (std::max(fabsf(q.rx),fabsf(q.w-q.rx))+std::max(fabsf(q.ry),fabsf(q.ry-q.h)))*ps;
}
static bool chooseSpritePreparation(size_t oi,const XrVector3f&center){
 auto&g=spritePreparations[oi];if(g.chosen)return g.valid;g.chosen=true;const auto&o=objects[oi];
 auto it=quads.find(key(o.col,o.seq,o.frame,0));if(it==quads.end())return false;
 g.fallback=it->second;g.q=it->second;if(o.unitID)fixedUnitFrame(o,g.q);
 g.px=mythWorldX(o.x,originX,scale);g.py=(o.z-originZ)*scale;g.pz=(o.y-originY)*scale;
 g.dx=center.x-g.px;g.dz=center.z-g.pz;g.len=sqrtf(g.dx*g.dx+g.dz*g.dz);if(g.len<.001f)return false;g.rx=g.dz/g.len;g.rz=-g.dx/g.len;
 unsigned flips=(o.rawFlags>>17)&3,dir=myth_object_sprite_direction(g.dx,g.dz,o.yaw,g.q.views,flips);
 if(!(g.q.seqflags&4)){auto jt=quads.find(key(o.col,o.seq,o.frame,dir));if(jt!=quads.end())g.q=jt->second;}
 float radius=spriteQuadRadius(g.q,o.scale);
 // Read dimensions from the resident index, without requesting/decoding art.
 uint64_t id=g.q.texture;if((g.q.flags&1)&&hd::entries.count(id|(1u<<30)))id|=1u<<30;
 auto hi=hd::entries.find(id);if(hi!=hd::entries.end()){
  auto&m=hi->second.meta;float factor=hd::be32(m.data()+96)/65536.f;
  if(std::isfinite(factor)&&factor>0&&factor<=16){Quad q=g.q;q.scale*=factor;q.w=hd::be16(m.data()+80);q.h=hd::be16(m.data()+82);q.rx=int16_t(hd::be16(m.data()+92));q.ry=int16_t(hd::be16(m.data()+94));radius=std::max(radius,spriteQuadRadius(q,o.scale));}
 }
 // Non-finite/unknown bounds must never reject an object.
 if(!std::isfinite(radius))radius=INFINITY;else radius+=.001f;
 g.coarse.add(g.px-radius,g.py-radius,g.pz-radius);g.coarse.add(g.px+radius,g.py+radius,g.pz+radius);g.valid=true;return true;
}
static void buildSpritePreparation(size_t oi){
 auto&g=spritePreparations[oi];if(g.ready){++spritePreparationShared;return;}g.ready=true;++spritePreparationBuilt;const auto&o=objects[oi];auto&q=g.q;
 sprite_atlas::registerTexture(textures[q.texture],unsigned(q.w),unsigned(q.h),q.col);g.highTexture=hdSprite(o,q);
 float ps=q.scale*(10.f/512.f)*scale*o.scale;
 g.left=-q.rx*ps;g.right=(q.w-q.rx)*ps;g.bottom=(q.ry-q.h)*ps;g.top=q.ry*ps;g.clipTop=g.py+g.top;g.clipHeight=g.top-g.bottom;
 unsigned flags=q.flags,flips=(o.rawFlags>>17)&3;
 if(flips&1){float old=g.left;g.left=-g.right;g.right=-old;flags^=1;}
 if(flips&2){float old=g.bottom;g.bottom=-g.top;g.top=-old;flags^=2;}
 g.u0=(flags&1)?1:0;g.u1=1-g.u0;g.v0=(flags&2)?1:0;g.v1=1-g.v0;
 auto vertex=[&](float x,float y,float u,float v){return Vertex{g.px+g.rx*x,g.py+y,g.pz+g.rz*x,u,v};};
 g.a=vertex(g.left,g.top,g.u0,g.v0);g.b=vertex(g.right,g.top,g.u1,g.v0);g.c=vertex(g.left,g.bottom,g.u0,g.v1);g.d=vertex(g.right,g.bottom,g.u1,g.v1);
}
static void beginSpriteFrame(){updateModelAppearances();hd::begin();waterFrameTime=seconds();reflectionCandidates.ready=false;reflectionCandidates.passReady=false;spritePreparations.resize(objects.size());for(auto&g:spritePreparations)g.chosen=g.valid=g.ready=false;for(auto&c:spriteViews)c.reset();trimColors(coloredSprites);}
static void prepareSprites(const XrVector3f&center,bool reflected){
 auto&cache=spriteView(reflected);if(cache.ready){spriteReuse[reflected]++;if(reflected)++reflectionGeometryReuses;return;}cache.ready=true;double began=seconds();spriteBuilds[reflected]++;
 std::vector<reflection_selection::UnionFrustum> coarseViews;
 if(reflected){for(size_t pi=0;pi<reflectionPlanes.size();pi++){
  // Include every plane, even if currently occluded: later passes may need it.
  float h=int(pi)==activeReflectionPlane?reflectionLevel:reflectionPlanes[pi].level;
  Mat mirror={1,0,0,0,0,-1,0,0,0,0,1,0,0,2*h,0,1};
  for(auto&m:lightingEyeMatrices){reflection_selection::UnionFrustum f;f.set(mul(m,mirror),h);coarseViews.push_back(f);}
 }}else for(auto&m:lightingEyeMatrices){reflection_selection::UnionFrustum f;f.set(m,-INFINITY);coarseViews.push_back(f);}
 auto&selectionFrames=cache.selectionFrames;auto&friendlyHover=cache.friendlyHover;auto&enemyHover=cache.enemyHover;auto&itemHover=cache.itemHover;
 for(size_t oi=0;oi<objects.size();oi++){const auto&o=objects[oi];if(!objectVisible(o))continue;
if(reflected&&!reflectionFull&&(o.objectType==3||o.objectType==4))continue;

 if(!chooseSpritePreparation(oi,center))continue;auto&g=spritePreparations[oi];
 ++spriteCoarseTested[reflected];
 bool overlay=!reflected&&(o.selected||showHealthBar(o.unitID,o.health,o.selected,pointerValid&&!selectionGesture.box&&o.unitID==hoverID)||(pointerValid&&!selectionGesture.box&&((o.unitID&&o.unitID==hoverID)||(!o.unitID&&o.objectKey==hoverItem&&pickupGreen())))||(o.unitID&&std::find(selectionPreview.begin(),selectionPreview.end(),o.unitID)!=selectionPreview.end()));
 bool visible=overlay||!std::isfinite(g.coarse.lo[0]);for(auto&f:coarseViews)if(f.visible(g.coarse)){visible=true;break;}
 if(!visible){++spriteCoarseRejected[reflected];continue;}
 buildSpritePreparation(oi);const Quad&q=g.q;GLuint highTexture=g.highTexture;
 float px=g.px,py=g.py,pz=g.pz,dx=g.dx,dz=g.dz,len=g.len,rx=g.rx,rz=g.rz;
 float left=g.left,right=g.right,bottom=g.bottom,top=g.top,clipTop=g.clipTop,clipHeight=g.clipHeight,u0=g.u0,u1=g.u1,v0=g.v0,v1=g.v1;
 Vertex a=g.a,b=g.b,c=g.c,d=g.d;
 // Flight textures marked by projectile dependency metadata use axial billboards.
 if(!o.unitID&&(q.flags&0x40000000u)&&q.views==1){
  Vec3 pos{px,py,pz},toward{center.x-px,center.y-py,center.z-pz};
  Vec3 ar=arrowRight({-o.motionX,o.motionZ,o.motionY}),au=arrowUp(ar,toward);
  auto vertex=[&](float x,float y,float u,float v){auto p=add(pos,add(times(ar,x),times(au,y)));return Vertex{p.x,p.y,p.z,u,v};};
  a=vertex(left,top,u0,v0);b=vertex(right,top,u1,v0);c=vertex(left,bottom,u0,v1);d=vertex(right,bottom,u1,v1);
 }
 if(!reflected&&((!o.unitID&&pointerValid&&!selectionGesture.box&&o.objectKey==hoverItem&&pickupGreen())||(o.unitID&&((pointerValid&&!selectionGesture.box&&o.unitID==hoverID)||std::find(selectionPreview.begin(),selectionPreview.end(),o.unitID)!=selectionPreview.end())))){
 auto bounds=fixedUnitFrame(o,g.fallback);
 auto point=[&](float x,float y){return Vertex{px+rx*x+dx/len*.018f,py+y,pz+rz*x+dz/len*.018f,0,0};};
 auto&h=!o.unitID?itemHover:(o.friendly?friendlyHover:enemyHover);
 auto strip=[&](float l,float bot,float r,float t){auto a=point(l,t),b=point(r,t),c=point(l,bot),d=point(r,bot);h.insert(h.end(),{a,b,c,b,d,c});};
 float l=bounds.left-.025f,r=bounds.right+.025f,bot=bounds.bottom-.025f,t=bounds.top+.025f,thickness=.022f*(((!o.unitID&&o.objectKey==hoverItem)||(o.unitID&&o.unitID==hoverID))?pointer_style::scale:1.f);
 strip(l,bot,l+thickness,t);strip(r-thickness,bot,r,t);strip(l+thickness,bot,r-thickness,bot+thickness);strip(l+thickness,t-thickness,r-thickness,t);
 }
 if(!reflected&&o.selected&&o.unitID){
 // Four triangle strips share the sprite plane and stereo orientation. Offset toward
 // the viewer slightly to avoid coplanar depth flicker; keep terrain occlusion.
 auto bounds=fixedUnitFrame(o,g.fallback);
 float pad=.025f,thickness=.022f;
 auto point=[&](float x,float y){return Vertex{px+rx*x+dx/len*.012f,py+y,pz+rz*x+dz/len*.012f,0,0};};
 auto strip=[&](float l,float bot,float r,float t){auto a=point(l,t),b=point(r,t),c=point(l,bot),d=point(r,bot);selectionFrames.insert(selectionFrames.end(),{a,b,c,b,d,c});};
 float l=bounds.left-pad,r=bounds.right+pad,bot=bounds.bottom-pad,t=bounds.top+pad;
 strip(l,bot,l+thickness,t);strip(r-thickness,bot,r,t);strip(l+thickness,bot,r-thickness,bot+thickness);strip(l+thickness,t-thickness,r-thickness,t);
 }
 if(!reflected&&showHealthBar(o.unitID,o.health,o.selected,pointerValid&&!selectionGesture.box&&o.unitID==hoverID)){
 auto bounds=fixedUnitFrame(o,g.fallback);
 auto point=[&](float x,float y){return Vertex{px+rx*x+dx/len*.018f,py+y,pz+rz*x+dz/len*.018f,0,0};};
 appendHealthBar(cache.healthBars,bounds.right+.025f+.025f,bounds.bottom-.025f,bounds.top+.025f,.055f,o.health,point);
 }
 if(!reflected&&o.unitID&&o.selected&&o.mana>0){
 auto bounds=fixedUnitFrame(o,g.fallback);
 float l=bounds.left-.025f,r=l+(bounds.right-bounds.left+.05f)*o.mana,bot=bounds.top+.065f,t=bot+.055f;
 auto point=[&](float x,float y){return Vertex{px+rx*x+dx/len*.018f,py+y,pz+rz*x+dz/len*.018f,0,0};};
 auto a=point(l,t),b=point(r,t),c=point(l,bot),d=point(r,bot);cache.manaBars.insert(cache.manaBars.end(),{a,b,c,b,d,c});
 }
 // Object FX are also present in the live particle stream. That pass sorts
 // transparent sprites and retains their original alpha instead of clipping at .5.
 if(blendedSprite(q)||(o.transfer==1&&o.transferPhase>0)||(o.transfer==2&&o.transferPhase<65535))continue;
 PreparedSprite prepared;prepared.object=oi;prepared.colored=highTexture;prepared.texture=q.texture;prepared.clipTop=clipTop;prepared.clipHeight=clipHeight;
 auto finish=[&](int plane){prepared.plane=plane;prepared.vertices={a,b,c,b,d,c};prepared.bounds={};for(auto&v:prepared.vertices)prepared.bounds.add(v.x,v.y,v.z);if(enhancedSpritePath)prepared.lightBounds=SpriteLightBoundsStream::calculate(prepared.vertices.data(),6);cache.sprites.push_back(prepared);};
 if(reflected){
  if(!reflectionFull&&!o.unitID&&clipHeight<.65f)continue;
  bool axial=!o.unitID&&(q.flags&0x40000000u)&&q.views==1;
  if(axial){float normalY=2*reflectionLevel-center.y;
   for(size_t pi=0;pi<reflectionPlanes.size();pi++){float level=int(pi)==activeReflectionPlane?reflectionLevel:reflectionPlanes[pi].level;if(clipTop<level)continue;
    Vec3 pos{px,py,pz},toward{center.x-px,2*level-normalY-py,center.z-pz};Vec3 ar=arrowRight({-o.motionX,o.motionZ,o.motionY}),au=arrowUp(ar,toward);
    auto vertex=[&](float x,float y,float u,float v){auto p=add(pos,add(times(ar,x),times(au,y)));return Vertex{p.x,p.y,p.z,u,v};};
    a=vertex(left,top,u0,v0);b=vertex(right,top,u1,v0);c=vertex(left,bottom,u0,v1);d=vertex(right,bottom,u1,v1);finish(int(pi));
   }
  }else{finish(-1);++reflectionGeometryBuilds;}
 }else finish(-1);
 }
 spritePrepareCPU[reflected]+=seconds()-began;
}
static float reflectionPlaneHeight(size_t plane){return int(plane)==activeReflectionPlane?reflectionLevel:reflectionPlanes[plane].level;}
template<class View> static void selectReflectionCandidates(int plane,const View&view,std::vector<size_t>&out){
 auto&sprites=spriteViews[1].sprites;
 reflectionCandidates.spatial.query(view,[&](size_t i)->const rendercull::Bounds&{return sprites[i].bounds;},[&](size_t i){auto&p=sprites[i];return reflection_selection::accepts(p.plane,p.clipTop,p.clipHeight,objects[p.object].unitID!=0,plane,reflectionPlaneHeight(plane),reflectionFull);},out,reflectionCandidateTests);
}
// Each decision is made once for both eyes and reused by the union and every
// pass. The active plane stores its geometry in globals; do not swap GL targets.
static void prepareReflectionLod(){
 double lodStart=seconds();auto&state=reflectionCandidates;if(!reflection_sprite_lod::enabled||reflection_sprite_lod::pixels<=0){state.lod.clear();state.waterRects.clear();return;}state.lod.assign(reflectionPlanes.size(),std::vector<int8_t>(spriteViews[1].sprites.size(),-1));state.waterRects.resize(reflectionPlanes.size());state.lodMatrices.resize(reflectionPlanes.size());state.lodFull.resize(reflectionPlanes.size());state.lodCropped.resize(reflectionPlanes.size());
 state.lodNear.resize(spriteViews[1].sprites.size());
 for(size_t id=0;id<state.lodNear.size();id++){const auto&o=objects[spriteViews[1].sprites[id].object];float dx=mythWorldX(o.x,originX,scale)-shadowLodEye.x,dy=(o.z-originZ)*scale-shadowLodEye.y,dz=(o.y-originY)*scale-shadowLodEye.z;float d2=dx*dx+dy*dy+dz*dz;state.lodNear[id]=!std::isfinite(d2)||sqrtf(d2)<reflection_sprite_lod::distance;}
 for(size_t pi=0;pi<reflectionPlanes.size();pi++)for(int eye=0;eye<2;eye++){
  auto&out=state.waterRects[pi][eye];out={};const auto&bounds=int(pi)==activeReflectionPlane?reflectionBounds:reflectionPlanes[pi].bounds;
  if(bounds.empty()){out={-1,-1,1,1};} // Unknown geometry: conservative fallback.
  for(auto&b:bounds){rendercull::Bounds box;for(int j=0;j<3;j++){box.lo[j]=b.lo[j];box.hi[j]=b.hi[j];}auto r=rendercull::projectedBounds(lightingEyeMatrices[eye],box);if(r.valid()){out.add(r.l,r.b);out.add(r.r,r.t);}}
  if(out.valid()&&reflectionW>0&&reflectionH>0){out.l=std::max(-1.f,out.l-8.f/reflectionW);out.r=std::min(1.f,out.r+8.f/reflectionW);out.b=std::max(-1.f,out.b-8.f/reflectionH);out.t=std::min(1.f,out.t+8.f/reflectionH);}
  float height=reflectionPlaneHeight(pi);Mat mirror={1,0,0,0,0,-1,0,0,0,0,1,0,0,2*height,0,1};
  state.lodMatrices[pi][eye]=mul(lightingEyeMatrices[eye],mirror);
  state.lodFull[pi][eye].set(state.lodMatrices[pi][eye],{-1,-1,1,1},-INFINITY);
  state.lodCropped[pi][eye].set(state.lodMatrices[pi][eye],out,height);
 }
 if(waterVisibilityFrame%120==0){for(auto it=reflection_sprite_lod::history.begin();it!=reflection_sprite_lod::history.end();){if(waterVisibilityFrame-it->second.seen>120)it=reflection_sprite_lod::history.erase(it);else ++it;}}
 reflection_sprite_lod::cpu+=seconds()-lodStart;
}
static bool keepReflectionSprite(size_t id,int plane){
 using namespace reflection_sprite_lod;
 if(!enabled||pixels<=0||plane<0||size_t(plane)>=reflectionCandidates.lod.size())return true;
 auto&decision=reflectionCandidates.lod[plane][id];if(decision>=0)return decision!=0;
 ++tested;auto&state=reflectionCandidates;auto&p=spriteViews[1].sprites[id];const auto&o=objects[p.object];
 Key key{unsigned(plane),o.objectKey,o.unitID,o.col};bool identifiable=o.objectKey||o.unitID;
 auto previous=identifiable?history.find(key):history.end();
 bool wasHidden=previous!=history.end()&&waterVisibilityFrame-previous->second.seen<=120&&previous->second.hidden;
 bool retained=state.lodNear[id]!=0;
 if(retained)++nearKept;
 else {float threshold=pixels*(wasHidden?1.25f:1.f);
  for(int eye=0;eye<2;eye++)if(projectedAtLeast(state.lodMatrices[plane][eye],state.lodFull[plane][eye],p.bounds,state.waterRects[plane][eye],reflectionW,reflectionH,threshold)){retained=true;break;}
 }
 decision=retained?1:0;if(!retained)++rejected;
 if(identifiable){if(retained){if(previous!=history.end())history.erase(previous);}else if(previous!=history.end())previous->second={true,waterVisibilityFrame};else history.emplace(key,History{true,waterVisibilityFrame});}
 return retained;
}
static void prepareReflectionStream(){
 auto&state=reflectionCandidates;auto&cache=spriteViews[1];if(state.ready)return;state.ready=true;++reflectionCandidateFrames;
 double began=seconds();state.spatial.build(cache.sprites.size(),[&](size_t i)->const rendercull::Bounds&{return cache.sprites[i].bounds;});reflectionIndexCPU+=seconds()-began;
 prepareReflectionLod();
 state.included.assign(cache.sprites.size(),0);state.toInput.assign(cache.sprites.size(),size_t(-1));
 began=seconds();
 // Conservative stereo union for every potentially rendered plane. The exact
 // cropped frustum is still applied per pass below. Do not use normal visibility.
 for(size_t plane=0;plane<reflectionPlanes.size();plane++){
  if(!reflectionPlanes[plane].visibility.render)continue;
  float height=reflectionPlaneHeight(plane);Mat mirror={1,0,0,0,0,-1,0,0,0,0,1,0,0,2*height,0,1};
  for(int eye=0;eye<2;eye++){
   if(reflection_sprite_lod::enabled&&reflection_sprite_lod::pixels>0){
    if(!state.waterRects[plane][eye].valid())continue;
    // Apply the same cropped frustum used by a real pass before invoking LOD.
    selectReflectionCandidates(int(plane),state.lodCropped[plane][eye],state.unionVisible);
   }else{reflection_selection::UnionFrustum view;view.set(mul(lightingEyeMatrices[eye],mirror),height);selectReflectionCandidates(int(plane),view,state.unionVisible);}
   double lodStart=seconds();for(auto id:state.unionVisible)if(keepReflectionSprite(id,int(plane)))state.included[id]=1;reflection_sprite_lod::cpu+=seconds()-lodStart;
  }
 }
 reflectionSelectCPU+=seconds()-began;cache.inputs.clear();
 for(size_t i=0;i<cache.sprites.size();i++)if(state.included[i]){auto&p=cache.sprites[i];if(!p.colored)p.colored=coloredTexture(objects[p.object],p.texture);state.toInput[i]=cache.inputs.size();cache.inputs.push_back({p.colored,spriteLightingOwner(objects[p.object],p.object),p.vertices.data(),6,&p.lightBounds});}
 reflectionUnionUploaded+=cache.inputs.size();began=seconds();cache.stream.uploadReferences(cache.inputs,true);reflectionVertexUploadCPU+=seconds()-began;cache.uploaded=true;
}
static bool prepareReflectionSpritePass(const XrVector3f&center){
 prepareSprites(center,true);prepareReflectionStream();auto&state=reflectionCandidates;double began=seconds();unsigned previous=reflectionCandidateTests;
 selectReflectionCandidates(activeReflectionPlane,reflectionCull,state.selected);reflectObjectsTested+=reflectionCandidateTests-previous;
 double lodStart=seconds();state.selected.erase(std::remove_if(state.selected.begin(),state.selected.end(),[&](size_t id){return !keepReflectionSprite(id,activeReflectionPlane);}),state.selected.end());reflection_sprite_lod::cpu+=seconds()-lodStart;
 reflectObjectsDrawn+=state.selected.size();reflectionCandidateSelected+=state.selected.size();reflectionSelectCPU+=seconds()-began;
 state.passReady=true;if(state.selected.empty())++reflection_sprite_lod::emptyPasses;return !state.selected.empty();
}
static void drawReflectionSprites(const XrVector3f&center){
 SpriteWaitScope phase(true,8);if(!reflectionCandidates.passReady)prepareReflectionSpritePass(center);reflectionCandidates.passReady=false;
 auto&state=reflectionCandidates;auto&cache=spriteViews[1];if(state.selected.empty())return;phase.next(9);double began=seconds();
 // A broad-phase union is an optimization, not a rendering invariant. Cropped
 // AABB tests, floating-point boundaries or changed pass state can select an
 // additional candidate. Keep existing mappings and append all missing inputs
 // before rebuilding the shared stream, so this and later passes remain valid.
 size_t added=0;
 for(auto id:state.selected)if(state.toInput[id]==size_t(-1)){
  auto&p=cache.sprites[id];if(!p.colored)p.colored=coloredTexture(objects[p.object],p.texture);
  state.toInput[id]=cache.inputs.size();state.included[id]=1;
  cache.inputs.push_back({p.colored,spriteLightingOwner(objects[p.object],p.object),p.vertices.data(),6,&p.lightBounds});++added;
 }
 if(added){
  ++reflectionUnionRepairs;reflectionUnionAdded+=added;reflectionUnionUploaded+=added;
  if(reflectionUnionRepairs==1)LOG("Reflection union repaired plane=%d height=%.6f added=%zu selected=%zu",activeReflectionPlane,reflectionLevel,added,state.selected.size());
  reflectionSelectCPU+=seconds()-began;began=seconds();cache.stream.uploadReferences(cache.inputs,true);reflectionVertexUploadCPU+=seconds()-began;began=seconds();
 }
 state.inputSelection.clear();for(auto id:state.selected)state.inputSelection.push_back(state.toInput[id]);
 reflectionSelectCPU+=seconds()-began;phase.next(10);began=seconds();cache.stream.selectReferences(state.inputSelection);reflectionElementUploadCPU+=seconds()-began;phase.next(11);cache.stream.draw(true);
}
static void reportReflectionSprites(unsigned frames){
 LOG("Reflection sprite LOD enabled=%d distance_m=%.1f pixels=%.1f tested=%u near_kept=%u rejected=%u empty_passes=%u cpu_ms_frame=%.3f",reflection_sprite_lod::enabled,reflection_sprite_lod::distance,reflection_sprite_lod::pixels,reflection_sprite_lod::tested,reflection_sprite_lod::nearKept,reflection_sprite_lod::rejected,reflection_sprite_lod::emptyPasses,reflection_sprite_lod::cpu*1000/std::max(1u,frames));
 LOG("Reflection LOD work projected_eyes=%u projected_corners=%u early_kept=%u",reflection_sprite_lod::projectedEyes,reflection_sprite_lod::projectedCorners,reflection_sprite_lod::earlyKept);
 reflection_sprite_lod::projectedEyes=reflection_sprite_lod::projectedCorners=reflection_sprite_lod::earlyKept=0;
 reflection_sprite_lod::tested=reflection_sprite_lod::nearKept=reflection_sprite_lod::rejected=reflection_sprite_lod::emptyPasses=0;reflection_sprite_lod::cpu=0;
 LOG("Sprite coarse tested=%u,%u rejected=%u,%u preparation_builds=%u shared_reuses=%u",spriteCoarseTested[0],spriteCoarseTested[1],spriteCoarseRejected[0],spriteCoarseRejected[1],spritePreparationBuilt,spritePreparationShared);
 spriteCoarseTested[0]=spriteCoarseTested[1]=spriteCoarseRejected[0]=spriteCoarseRejected[1]=spritePreparationBuilt=spritePreparationShared=0;
 LOG("Reflection union repairs=%u added=%u",reflectionUnionRepairs,reflectionUnionAdded);reflectionUnionRepairs=reflectionUnionAdded=0;
 LOG("Reflection shared stream frames=%u union_sprites=%u leaf_tests=%u selected=%u index_ms_frame=%.3f select_ms_frame=%.3f vertex_upload_ms_frame=%.3f element_upload_ms_frame=%.3f element_uploads=%llu element_bytes=%llu element_reuses=%llu",reflectionCandidateFrames,reflectionUnionUploaded,reflectionCandidateTests,reflectionCandidateSelected,reflectionIndexCPU*1000/std::max(1u,frames),reflectionSelectCPU*1000/std::max(1u,frames),reflectionVertexUploadCPU*1000/std::max(1u,frames),reflectionElementUploadCPU*1000/std::max(1u,frames),(unsigned long long)streamIndexUploads,(unsigned long long)streamIndexBytes,(unsigned long long)streamIndexReuses);
 reflectionCandidateFrames=reflectionUnionUploaded=reflectionCandidateTests=reflectionCandidateSelected=0;reflectionIndexCPU=reflectionSelectCPU=reflectionVertexUploadCPU=reflectionElementUploadCPU=0;streamIndexUploads=streamIndexBytes=streamIndexReuses=0;
}
static void drawPreparedSprites(const XrVector3f&center,bool reflected,const Mat&mvp){
 if(reflected){drawReflectionSprites(center);return;}
 SpriteWaitScope phase(reflected,8);prepareSprites(center,reflected);phase.next(9);auto&cache=spriteView(reflected);
 if(!reflected&&cache.uploaded){++stereoUploadReuses;phase.next(11);cache.stream.draw(false);return;}
 rendercull::StereoFrustum views;if(!reflected)views.set(lightingEyeMatrices);
 cache.visible.clear();
 for(size_t i=0;i<cache.sprites.size();i++){if(reflected){reflectObjectsTested++;if(!reflectionCull.visible(cache.sprites[i].bounds))continue;reflectObjectsDrawn++;}else{if(diagnosticEnabled)normalSpriteTested++;if(!views.visible(cache.sprites[i].bounds))continue;if(diagnosticEnabled)normalSpriteDrawn++;}cache.visible.push_back(i);}
 if(cache.uploaded&&cache.visible==cache.previousVisible){phase.next(11);cache.stream.draw(reflected);return;}
 double batchStart=diagnosticEnabled?seconds():0;cache.previousVisible=cache.visible;
 cache.inputs.clear();
 bool sampled=reflected&&spriteProfile.sample();if(sampled)spriteProfile.passes++;
 for(auto index:cache.visible){auto&p=cache.sprites[index];SpriteScope profile(sampled);profile.next(1);
  profile.next(2);if(!p.colored)p.colored=coloredTexture(objects[p.object],p.texture);
  profile.next(3);cache.inputs.push_back({p.colored,spriteLightingOwner(objects[p.object],p.object),p.vertices.data(),6,&p.lightBounds});
 }
 if(diagnosticEnabled){spriteBatchCPU[reflected]+=seconds()-batchStart;spriteBatchPasses[reflected]++;}double began=sampled?seconds():0;phase.next(10);cache.stream.uploadReferences(cache.inputs);phase.next(11);cache.stream.draw(reflected);cache.uploaded=true;
 if(sampled)spriteProfile.ms[3]+=(seconds()-began)*1000;
}
