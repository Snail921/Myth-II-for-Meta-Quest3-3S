#include "surface_compositor.h"
static surface::Compositor groundEffects;
static GLuint waterShadowTexture=0;
static std::map<uint64_t,unsigned> groundFrames;
static uint64_t surfaceRevision=~uint64_t(0);
static unsigned surfaceUpdates=0;
static uint64_t appliedSurfaceEpoch=0;static std::vector<uint8_t> initialSurfaceIndices,initialSurfaceShade;
#include "detail_shadows.h"
static void loadSurfaceEffects(android_app*a){
 loadDetailShadowSetting(a);
 auto b=asset(a,"surface-effects.pack");size_t p=0;
 auto take=[&](size_t n){if(n>b.size()-p)throw std::runtime_error("surface bounds");auto r=b.data()+p;p+=n;return r;};
 if(memcmp(take(8),"MYTHSF01",8))throw std::runtime_error("surface magic");auto h=take(16);
 unsigned w=u32(h),height=u32(h+4),nm=u32(h+8),nf=u32(h+12);
 if(w!=terrain.width*8||height!=terrain.height*8||w>4096||height>4096||nm>20000||nf>1000000)throw std::runtime_error("surface sizes");
 groundEffects={};groundFrames.clear();auto&g=groundEffects;g.w=w;g.h=height;
 auto ix=take(size_t(w)*height);g.indices.assign(ix,ix+size_t(w)*height);
 auto sh=take(size_t(w)*height);g.base.assign(sh,sh+size_t(w)*height);g.shade=g.base;g.initShadow();
 glActiveTexture(GL_TEXTURE0);glGenTextures(1,&waterShadowTexture);glBindTexture(GL_TEXTURE_2D,waterShadowTexture);glPixelStorei(GL_UNPACK_ALIGNMENT,1);glTexImage2D(GL_TEXTURE_2D,0,GL_R8,w,height,0,GL_RED,GL_UNSIGNED_BYTE,g.shadow.data());glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
 initialSurfaceIndices=g.indices;initialSurfaceShade=g.base;appliedSurfaceEpoch=0;
 auto lut=take(256*256*4);g.lut.assign(lut,lut+256*256*4);
 ui_contrast::analyze(w,height,g.indices.data(),g.base.data(),g.lut.data());LOG("Auto UI contrast enabled=%d orange=%d samples=%u pale=%u yellow=%u level=%s",ui_contrast::enabled,ui_contrast::active,ui_contrast::samples,ui_contrast::pale,ui_contrast::yellow,levelName.c_str());
 for(unsigned i=0;i<nm;i++){auto h=take(24);unsigned id=u32(h),kind=u32(h+4),mw=u32(h+16),mh=u32(h+20);
  if(kind>2||!mw||!mh||mw>1024||mh>1024)throw std::runtime_error("surface mask size");
  surface::Mask m{kind,i32(h+8),i32(h+12),(int)mw,(int)mh,{}};auto pixels=take(size_t(mw)*mh);m.pixels.assign(pixels,pixels+size_t(mw)*mh);if(!g.masks.emplace(id,std::move(m)).second)throw std::runtime_error("duplicate surface mask");
 }
 for(unsigned i=0;i<nf;i++){auto f=take(16);unsigned col=u32(f),seq=u32(f+4),frame=u32(f+8),mask=u32(f+12);if(col>=256||seq>=256||frame>=256||!g.masks.count(mask))throw std::runtime_error("surface frame");groundFrames[(uint64_t(col)<<32)|(seq<<16)|frame]=mask;}
 if(p!=b.size())throw std::runtime_error("surface trailing bytes");surfaceRevision=~uint64_t(0);surfaceUpdates=0;
 LOG("Surface assets %ux%u masks=%u frames=%u bytes=%zu",w,height,nm,nf,b.size());
}
static void updateSurfaceEffects(){
 if(surfaceRevision==snapshotRevision||groundEffects.w==0)return;surfaceRevision=snapshotRevision;
 detailShadowBuilds=0;detailShadowPixels=0;double start=seconds();std::vector<surface::Stamp> stamps;unsigned modelCount=0,objectCount=0,lightCount=0,particleLights=0;
 // Original lists prepend model stamps after object collection: model minima first.
 if(modelPosesValid)for(auto it=modelPoses.rbegin();it!=modelPoses.rend();++it){const auto&p=*it;if(p.owner==65535)continue;auto mi=groundEffects.masks.find(p.geometry);if(mi==groundEffects.masks.end()||mi->second.kind!=2)continue;auto&m=mi->second;
  stamps.push_back({(uint64_t(1)<<63)|p.owner,p.geometry,p.pos[0]+m.ox,p.pos[1]+m.oy,65535});modelCount++;
 }
 for(auto it=objects.rbegin();it!=objects.rend();++it){const auto&o=*it;if(o.rawFlags&0x200000)continue;
  auto fi=groundFrames.find((uint64_t(o.col)<<32)|(o.seq<<16)|o.frame);if(fi==groundFrames.end())continue;unsigned maskID=fi->second;if(groundEffects.masks.at(maskID).kind!=0)continue;unsigned detailed=detailShadow(o);if(detailed)maskID=detailed;auto&m=groundEffects.masks.at(maskID);
  if(m.kind!=0)continue; // Lights arrive with their original runtime masks in the v6 light stream.
  float wx=mythWorldX(o.x,originX,scale),wz=(o.y-originY)*scale;
  if(!terrain.contains(wx,wz))continue;
  int height=(int)std::lround((o.z-(terrain.elevation(wx,wz)/scale+originZ))*512.f),offset=0;
  int strength=surface::place(height,m.kind,o.transfer,o.transferPhase,offset);if(strength<=0)continue;
  stamps.push_back({o.objectKey,maskID,(int)std::lround(o.x*512)+m.ox+offset,(int)std::lround(o.y*512)+m.oy+offset,strength});objectCount++;
 }
 auto&g=groundEffects;
 // Original 0x4bca50 consumes shadow list +a4, then additive light list +a8.
 for(auto&l:liveLights){float wx=mythWorldX(l.x/512.f,originX,scale),wz=(l.y/512.f-originY)*scale;if(!terrain.contains(wx,wz))continue;
  int height=(int)std::lround(l.z-(terrain.elevation(wx,wz)/scale+originZ)*512.f),offset=0;if(l.heightMode==1)height=0;else if(l.heightMode==2&&height<0)continue;
  int strength=surface::place(height,1,l.transfer,l.phase,offset,l.alpha);if(strength<=0)continue;
  // Use the original 1.0x intensity, including synchronized ground-fire lights.
  unsigned key=0x80000000u|l.key;auto found=g.masks.find(key);
  if(found==g.masks.end())g.masks.emplace(key,surface::Mask{1,l.ox,l.oy,(int)l.w,(int)l.h,l.pixels});
  else if(found->second.pixels!=l.pixels||found->second.w!=(int)l.w||found->second.h!=(int)l.h){found->second={1,l.ox,l.oy,(int)l.w,(int)l.h,l.pixels};g.externalDirty.assign(((g.w+63)/64)*((g.h+63)/64),1);}
  stamps.push_back({(uint64_t(l.domain)<<56)|l.source,key,l.x+l.ox,l.y+l.oy,strength});lightCount++;particleLights+=l.domain>=2;
 }
 static unsigned lastLightCount=0;if(lightCount!=lastLightCount){LOG("Ground light transition tick=%u count=%u particles=%u received=%zu",tick,lightCount,particleLights,liveLights.size());lastLightCount=lightCount;}
 if(surfaceUpdatesQueue.epoch&&appliedSurfaceEpoch!=surfaceUpdatesQueue.epoch){
  if(appliedSurfaceEpoch){g.indices=initialSurfaceIndices;g.base=initialSurfaceShade;g.externalDirty.assign(((g.w+63)/64)*((g.h+63)/64),1);}
  appliedSurfaceEpoch=surfaceUpdatesQueue.epoch;LOG("Persistent ground epoch=%llu",(unsigned long long)appliedSurfaceEpoch);
 }
 unsigned persistentTiles=surfaceUpdatesQueue.pending.size(),changedColor=0,changedShade=0;
 for(auto&item:surfaceUpdatesQueue.pending){unsigned id=item.first;auto&p=item.second;int x=(id%(g.w/256))*256,y=(id/(g.w/256))*256;unsigned colorDelta=0,shadeDelta=0;
  for(int j=0;j<256;j++)for(int i=0;i<256;i++){int src=j*256+i,dst=(y+j)*g.w+x+i;colorDelta+=g.indices[dst]!=p[src];shadeDelta+=g.base[dst]!=p[65536+src];}
  changedColor+=colorDelta;changedShade+=shadeDelta;
  if(colorDelta||shadeDelta)g.replaceTile(id,p.data(),p.data()+65536);
 }
 surfaceUpdatesQueue.pending.clear();
 if(persistentTiles)LOG("Persistent ground tick=%u tiles=%u changed_color=%u changed_shade=%u",tick,persistentTiles,changedColor,changedShade);
 g.compose(stamps);double composed=seconds();
 glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,textures[0]);glPixelStorei(GL_UNPACK_ALIGNMENT,1);std::vector<uint8_t> rgba;unsigned bytes=0;
 for(auto tile:g.tiles){g.rgba(tile,rgba);int x=(tile%((g.w+63)/64))*64,y=(tile/((g.w+63)/64))*64;glTexSubImage2D(GL_TEXTURE_2D,0,x,y,std::min(64,g.w-x),std::min(64,g.h-y),GL_RGBA,GL_UNSIGNED_BYTE,rgba.data());bytes+=rgba.size();}
 glBindTexture(GL_TEXTURE_2D,waterShadowTexture);glPixelStorei(GL_UNPACK_ROW_LENGTH,g.w);
 for(auto tile:g.tiles){int x=(tile%((g.w+63)/64))*64,y=(tile/((g.w+63)/64))*64;glTexSubImage2D(GL_TEXTURE_2D,0,x,y,std::min(64,g.w-x),std::min(64,g.h-y),GL_RED,GL_UNSIGNED_BYTE,g.shadow.data()+y*g.w+x);}
 glPixelStorei(GL_UNPACK_ROW_LENGTH,0);glBindTexture(GL_TEXTURE_2D,textures[0]);
 if(surfaceUpdates++%120==0)LOG("Ground shadows tick=%u object=%u model=%u lights=%u particles=%u dirty=%zu upload=%u compose_ms=%.3f total_ms=%.3f",tick,objectCount,modelCount,lightCount,particleLights,g.tiles.size(),bytes,(composed-start)*1000,(seconds()-start)*1000);
}
