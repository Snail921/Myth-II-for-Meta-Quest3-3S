// Frozen pre-coarse-culling preparation, used only for regression comparisons.
static void prepareSpritesReference(const XrVector3f&center,bool reflected){
 auto&cache=spriteView(reflected);if(cache.ready){spriteReuse[reflected]++;if(reflected)++reflectionGeometryReuses;return;}cache.ready=true;double began=seconds();spriteBuilds[reflected]++;
 auto&selectionFrames=cache.selectionFrames;auto&friendlyHover=cache.friendlyHover;auto&enemyHover=cache.enemyHover;auto&itemHover=cache.itemHover;
 for(size_t oi=0;oi<objects.size();oi++){const auto&o=objects[oi];if(!objectVisible(o))continue;
if(reflected&&!reflectionFull&&(o.objectType==3||o.objectType==4))continue;

 auto it=quads.find(key(o.col,o.seq,o.frame,0));if(it==quads.end())continue;Quad q=it->second;if(o.unitID)fixedUnitFrame(o,q);float px=mythWorldX(o.x,originX,scale),py=(o.z-originZ)*scale,pz=(o.y-originY)*scale;float dx=center.x-px,dz=center.z-pz,len=sqrtf(dx*dx+dz*dz);if(len<.001f)continue;float rx=dz/len,rz=-dx/len;unsigned objectFlips=(o.rawFlags>>17)&3;unsigned dir=myth_object_sprite_direction(dx,dz,o.yaw,q.views,objectFlips);if(!(q.seqflags&4)){auto jt=quads.find(key(o.col,o.seq,o.frame,dir));if(jt!=quads.end())q=jt->second;}
 sprite_atlas::registerTexture(textures[q.texture],unsigned(q.w),unsigned(q.h),q.col);GLuint highTexture=hdSprite(o,q);
 float ps=q.scale*(10.f/512.f)*scale*o.scale;float left=-q.rx*ps,right=(q.w-q.rx)*ps,bottom=(q.ry-q.h)*ps,top=q.ry*ps;float clipTop=py+top,clipHeight=top-bottom;unsigned flags=q.flags;if(objectFlips&1){float old=left;left=-right;right=-old;flags^=1;}if(objectFlips&2){float old=bottom;bottom=-top;top=-old;flags^=2;}float u0=(flags&1)?1:0,u1=1-u0,v0=(flags&2)?1:0,v1=1-v0;Vertex a{px+rx*left,py+top,pz+rz*left,u0,v0},b{px+rx*right,py+top,pz+rz*right,u1,v0},c{px+rx*left,py+bottom,pz+rz*left,u0,v1},d{px+rx*right,py+bottom,pz+rz*right,u1,v1};
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
 float l=bounds.left-.025f,r=bounds.right+.025f,bot=bounds.bottom-.025f,t=bounds.top+.025f,thickness=.022f*(((!o.unitID&&o.objectKey==hoverItem)||(o.unitID&&o.unitID==hoverID))?pointer_style::scale:1.f);
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
