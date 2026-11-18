#pragma once
struct PreparedSprite {size_t object;unsigned texture;GLuint colored=0;std::array<Vertex,6> vertices;rendercull::Bounds bounds;};
struct SpriteViewCache {
 bool ready=false,uploaded=false;std::vector<PreparedSprite> sprites;
 std::vector<Vertex> selectionFrames,friendlyHover,enemyHover,itemHover,healthBars,manaBars;
 std::map<std::pair<GLuint,unsigned>,std::vector<Vertex>> batches;
 SpriteStream stream;
 std::vector<size_t> visible,previousVisible;
 void reset(){ready=uploaded=false;visible.clear();previousVisible.clear();sprites.clear();selectionFrames.clear();friendlyHover.clear();enemyHover.clear();itemHover.clear();healthBars.clear();manaBars.clear();for(auto it=batches.begin();it!=batches.end();){if(it->second.empty())it=batches.erase(it);else{it->second.clear();++it;}}}
};
static SpriteViewCache spriteViews[2];
static std::array<SpriteViewCache,16> reflectionSpriteViews;
static SpriteViewCache& spriteView(bool reflected){return reflected?reflectionSpriteViews.at(activeReflectionPlane):spriteViews[0];}
static unsigned normalSpriteTested=0,normalSpriteDrawn=0;
static double spriteBatchCPU[2]{};static unsigned spriteBatchPasses[2]{};
static double spritePrepareCPU[2]{};static unsigned spriteBuilds[2]{},spriteReuse[2]{};
static void beginSpriteFrame(){hd::begin();waterFrameTime=seconds();for(auto&c:spriteViews)c.reset();for(auto&c:reflectionSpriteViews)c.reset();trimColors(coloredSprites);}
static void prepareSprites(const XrVector3f&center,bool reflected){
 auto&cache=spriteView(reflected);if(cache.ready){spriteReuse[reflected]++;return;}cache.ready=true;double began=seconds();spriteBuilds[reflected]++;
 auto&selectionFrames=cache.selectionFrames;auto&friendlyHover=cache.friendlyHover;auto&enemyHover=cache.enemyHover;auto&itemHover=cache.itemHover;
 for(size_t oi=0;oi<objects.size();oi++){const auto&o=objects[oi];if(!objectVisible(o))continue;
if(reflected&&!reflectionFull&&(o.objectType==3||o.objectType==4))continue;auto it=quads.find(key(o.col,o.seq,o.frame,0));if(it==quads.end())continue;Quad q=it->second;if(o.unitID)fixedUnitFrame(o,q);float px=mythWorldX(o.x,originX,scale),py=(o.z-originZ)*scale,pz=(o.y-originY)*scale;float dx=center.x-px,dz=center.z-pz,len=sqrtf(dx*dx+dz*dz);if(len<.001f)continue;float rx=dz/len,rz=-dx/len;unsigned objectFlips=(o.rawFlags>>17)&3;unsigned dir=myth_object_sprite_direction(dx,dz,o.yaw,q.views,objectFlips);if(!(q.seqflags&4)){auto jt=quads.find(key(o.col,o.seq,o.frame,dir));if(jt!=quads.end())q=jt->second;}
 GLuint highTexture=hdSprite(o,q);
 float ps=q.scale*(10.f/512.f)*scale*o.scale;float left=-q.rx*ps,right=(q.w-q.rx)*ps,bottom=(q.ry-q.h)*ps,top=q.ry*ps;if(reflected&&((!reflectionFull&&!o.unitID&&top-bottom<.65f)||py+top<reflectionLevel))continue;unsigned flags=q.flags;if(objectFlips&1){float old=left;left=-right;right=-old;flags^=1;}if(objectFlips&2){float old=bottom;bottom=-top;top=-old;flags^=2;}float u0=(flags&1)?1:0,u1=1-u0,v0=(flags&2)?1:0,v1=1-v0;Vertex a{px+rx*left,py+top,pz+rz*left,u0,v0},b{px+rx*right,py+top,pz+rz*right,u1,v0},c{px+rx*left,py+bottom,pz+rz*left,u0,v1},d{px+rx*right,py+bottom,pz+rz*right,u1,v1};
 // Flight textures marked by projectile dependency metadata use axial billboards.
 if(!o.unitID&&(q.flags&0x40000000u)&&q.views==1){
  Vec3 pos{px,py,pz},toward{center.x-px,center.y-py,center.z-pz};
  Vec3 ar=arrowRight({-o.motionX,o.motionZ,o.motionY}),au=arrowUp(ar,toward);
  auto vertex=[&](float x,float y,float u,float v){auto p=add(pos,add(times(ar,x),times(au,y)));return Vertex{p.x,p.y,p.z,u,v};};
  a=vertex(left,top,u0,v0);b=vertex(right,top,u1,v0);c=vertex(left,bottom,u0,v1);d=vertex(right,bottom,u1,v1);
 }
 if(!reflected&&((!o.unitID&&pointerValid&&!selectionGesture.box&&o.objectKey==hoverItem&&pickupGreen())||(o.unitID&&((pointerValid&&!selectionGesture.box&&o.unitID==hoverID)||std::find(selectionPreview.begin(),selectionPreview.end(),o.unitID)!=selectionPreview.end())))){
 auto bounds=fixedUnitFrame(o,it->second);
 auto point=[&](float x,float y){return Vertex{px+rx*x+dx/len*.018f,py+y,pz+rz*x+dz/len*.018f,0,0};};
 auto&h=!o.unitID?itemHover:(o.friendly?friendlyHover:enemyHover);
 auto strip=[&](float l,float bot,float r,float t){auto a=point(l,t),b=point(r,t),c=point(l,bot),d=point(r,bot);h.insert(h.end(),{a,b,c,b,d,c});};
 float l=bounds.left-.025f,r=bounds.right+.025f,bot=bounds.bottom-.025f,t=bounds.top+.025f,thickness=.022f;
 strip(l,bot,l+thickness,t);strip(r-thickness,bot,r,t);strip(l+thickness,bot,r-thickness,bot+thickness);strip(l+thickness,t-thickness,r-thickness,t);
 }
 if(!reflected&&o.selected&&o.unitID){
 // Four triangle strips share the sprite plane and stereo orientation. Offset toward
 // the viewer slightly to avoid coplanar depth flicker; keep terrain occlusion.
 auto bounds=fixedUnitFrame(o,it->second);
 float pad=.025f,thickness=.022f;
 auto point=[&](float x,float y){return Vertex{px+rx*x+dx/len*.012f,py+y,pz+rz*x+dz/len*.012f,0,0};};
 auto strip=[&](float l,float bot,float r,float t){auto a=point(l,t),b=point(r,t),c=point(l,bot),d=point(r,bot);selectionFrames.insert(selectionFrames.end(),{a,b,c,b,d,c});};
 float l=bounds.left-pad,r=bounds.right+pad,bot=bounds.bottom-pad,t=bounds.top+pad;
 strip(l,bot,l+thickness,t);strip(r-thickness,bot,r,t);strip(l+thickness,bot,r-thickness,bot+thickness);strip(l+thickness,t-thickness,r-thickness,t);
 }
 if(!reflected&&showHealthBar(o.unitID,o.health,o.selected,pointerValid&&!selectionGesture.box&&o.unitID==hoverID)){
 auto bounds=fixedUnitFrame(o,it->second);
 auto point=[&](float x,float y){return Vertex{px+rx*x+dx/len*.018f,py+y,pz+rz*x+dz/len*.018f,0,0};};
 appendHealthBar(cache.healthBars,bounds.right+.025f+.025f,bounds.bottom-.025f,bounds.top+.025f,.055f,o.health,point);
 }
 if(!reflected&&o.unitID&&o.selected&&o.mana>0){
 auto bounds=fixedUnitFrame(o,it->second);
 float l=bounds.left-.025f,r=l+(bounds.right-bounds.left+.05f)*o.mana,bot=bounds.top+.065f,t=bot+.055f;
 auto point=[&](float x,float y){return Vertex{px+rx*x+dx/len*.018f,py+y,pz+rz*x+dz/len*.018f,0,0};};
 auto a=point(l,t),b=point(r,t),c=point(l,bot),d=point(r,bot);cache.manaBars.insert(cache.manaBars.end(),{a,b,c,b,d,c});
 }
 // Object FX are also present in the live particle stream. That pass sorts
 // transparent sprites and retains their original alpha instead of clipping at .5.
 if(blendedSprite(q))continue;
 PreparedSprite prepared;prepared.object=oi;prepared.colored=highTexture;prepared.texture=q.texture;prepared.vertices={a,b,c,b,d,c};for(auto&v:prepared.vertices)prepared.bounds.add(v.x,v.y,v.z);cache.sprites.push_back(prepared);
 }
 spritePrepareCPU[reflected]+=seconds()-began;
}
static void drawPreparedSprites(const XrVector3f&center,bool reflected,const Mat&mvp){
 SpriteWaitScope phase(reflected,8);prepareSprites(center,reflected);phase.next(9);auto&cache=spriteView(reflected);
 rendercull::Frustum viewCull;if(!reflected)viewCull.set(mvp,{-1,-1,1,1},-INFINITY);
 cache.visible.clear();
 for(size_t i=0;i<cache.sprites.size();i++){if(reflected){reflectObjectsTested++;if(!reflectionCull.visible(cache.sprites[i].bounds))continue;reflectObjectsDrawn++;}else{if(diagnosticEnabled)normalSpriteTested++;if(!viewCull.visible(cache.sprites[i].bounds))continue;if(diagnosticEnabled)normalSpriteDrawn++;}cache.visible.push_back(i);}
 if(cache.uploaded&&cache.visible==cache.previousVisible){phase.next(11);cache.stream.draw(reflected);return;}
 double batchStart=diagnosticEnabled?seconds():0;cache.previousVisible=cache.visible;
 for(auto&b:cache.batches)b.second.clear();
 bool sampled=reflected&&spriteProfile.sample();if(sampled)spriteProfile.passes++;
 for(auto index:cache.visible){auto&p=cache.sprites[index];SpriteScope profile(sampled);profile.next(1);
  profile.next(2);if(!p.colored)p.colored=coloredTexture(objects[p.object],p.texture);
  profile.next(3);auto&batch=cache.batches[{p.colored,spriteLightingOwner(objects[p.object],p.object)}];batch.insert(batch.end(),p.vertices.begin(),p.vertices.end());
 }
 if(diagnosticEnabled){spriteBatchCPU[reflected]+=seconds()-batchStart;spriteBatchPasses[reflected]++;}double began=sampled?seconds():0;phase.next(10);cache.stream.upload(cache.batches);phase.next(11);cache.stream.draw(reflected);cache.uploaded=true;
 if(sampled)spriteProfile.ms[3]+=(seconds()-began)*1000;
}
