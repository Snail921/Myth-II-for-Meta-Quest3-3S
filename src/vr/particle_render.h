#pragma once
#include "particle_stream.h"
struct ParticleTexture{unsigned w,h;std::vector<uint8_t> pixels,indices;GLuint texture=0;sprite_material::Binding material{};};
static std::vector<ParticleTexture> particleTextures;
static sprite_material::Binding particleMaterial(unsigned i){return i<particleTextures.size()?particleTextures[i].material:sprite_material::Binding{};}
static std::map<std::array<unsigned,4>,Quad> particleQuads;
static std::map<uint64_t,ColoredSprite> particleColors;
static rendercull::Frustum particleViewCull;
static unsigned particleGeneration=0;
static unsigned particleUploads=0;static bool particleCanonical=false;
static unsigned particleCollectionKey(const LiveParticle&p){return particleCanonical?p[13]:p[1];}
static void loadParticles(android_app*a){
 particleGeneration++;
 for(auto&t:particleTextures)if(t.texture)glDeleteTextures(1,&t.texture);particleTextures.clear();particleQuads.clear();for(auto&v:particleColors)glDeleteTextures(1,&v.second.texture);particleColors.clear();
 auto b=asset(a,"particles.pack");size_t pos=0;auto take=[&](size_t n){if(n>b.size()-pos)throw std::runtime_error("particle assets bounds");auto p=b.data()+pos;pos+=n;return p;};
 auto magic=take(8);particleCanonical=!memcmp(magic,"MYTHFX03",8);if(!particleCanonical&&memcmp(magic,"MYTHFX02",8))throw std::runtime_error("particle assets magic");auto h=take(8);unsigned nt=u32(h),nq=u32(h+4);if(nt>20000||nq>100000)throw std::runtime_error("particle assets count");
 for(unsigned i=0;i<nt;i++){auto h=take(8);unsigned w=u32(h),height=u32(h+4);if(!w||!height||w>4096||height>4096)throw std::runtime_error("particle bitmap");auto p=take(size_t(w)*height*4);auto ix=take(size_t(w)*height);particleTextures.push_back({w,height,std::vector<uint8_t>(p,p+size_t(w)*height*4),std::vector<uint8_t>(ix,ix+size_t(w)*height),0,sprite_material::match(w,height,p)});}
 for(unsigned i=0;i<nq;i++){Quad q;static_assert(sizeof(q)==52);memcpy(&q,take(52),52);if(q.texture>=nt||q.seq>255||q.frame>255||q.dir>=q.views||q.views>64||!std::isfinite(q.scale)||q.scale<=0)throw std::runtime_error("particle quad");particleQuads[{q.col,q.seq,q.frame,q.dir}]=q;}
 if(pos!=b.size())throw std::runtime_error("particle trailing bytes");LOG("Particle assets textures=%u quads=%u bytes=%zu",nt,nq,b.size());
}
static GLuint particleTexture(unsigned index,unsigned palette,unsigned collection){auto&t=particleTextures.at(index);
 auto pal=livePalettes.find(palette);if(pal!=livePalettes.end()){
  uint64_t key=(uint64_t(palette)<<32)|index;auto found=particleColors.find(key);if(found!=particleColors.end()&&found->second.palette==pal->second){found->second.used=++colorUse;return found->second.texture;}
  GLuint tx;if(found==particleColors.end())glGenTextures(1,&tx);else tx=found->second.texture;auto rgba=t.pixels;for(size_t i=0;i<t.indices.size();i++)memcpy(rgba.data()+i*4,pal->second.data()+t.indices[i]*4,3);
  glBindTexture(GL_TEXTURE_2D,tx);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,t.w,t.h,0,GL_RGBA,GL_UNSIGNED_BYTE,rgba.data());particleUploads++;glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);sprite_material::assign(tx,t.material);atlasTextureChanged(tx,t.w,t.h,collection);particleColors[key]={tx,pal->second,++colorUse};return tx;
 }
 if(!t.texture){glGenTextures(1,&t.texture);glBindTexture(GL_TEXTURE_2D,t.texture);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,t.w,t.h,0,GL_RGBA,GL_UNSIGNED_BYTE,t.pixels.data());glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);}sprite_material::assign(t.texture,t.material);sprite_atlas::registerTexture(t.texture,t.w,t.h,collection);return t.texture;}
static void drawParticles(const XrVector3f&center,bool reflected=false){
 double fxStart=seconds();unsigned oldUploads=particleUploads;
 trimColors(particleColors);
 struct Item{const LiveParticle*p;float distance;};std::vector<Item> order;
 for(auto&p:liveParticles){if(p[15]==2)continue;if(p[15]==1){auto regular=quads.find(key(p[13],p[2],p[3],0));if(regular!=quads.end()&&!blendedSprite(regular->second)&&p[7]>=65535)continue;}float x=mythWorldX(int32_t(p[4])/512.f,originX,scale),y=(int32_t(p[6])/512.f-originZ)*scale,z=(int32_t(p[5])/512.f-originY)*scale;{
  if(reflected)reflectFXTested++;auto it=particleQuads.find({particleCollectionKey(p),p[2],p[3],0});if(it==particleQuads.end())continue;Quad q=it->second;
  float dx=center.x-x,dz=center.z-z;
  if(p[15]!=4&&!(q.seqflags&4)){auto jt=particleQuads.find({particleCollectionKey(p),p[2],p[3],myth_object_sprite_direction(dx,dz,p[12]/65536.f,q.views,p[15]==1?p[10]:0)});if(jt!=particleQuads.end())q=jt->second;}
  float ps=q.scale*(10.f/512.f)*scale;
  float radius=std::max(std::abs(float(q.rx)),std::abs(float(q.w)-q.rx))*ps*std::abs(p[8]/256.f);
  float height=std::max(std::abs(float(q.ry)),std::abs(float(q.ry)-q.h))*ps*std::abs(p[9]/256.f);
  if(p[15]==3){radius=(p[8]+p[9])/512.f*scale;height=.01f;}rendercull::Bounds bounds;bounds.add(x-radius,y-height,z-radius);bounds.add(x+radius,y+height,z+radius);
  if(p[15]==4){radius+=p[9]/512.f*scale;bounds.add(x-radius,y-radius,z-radius);bounds.add(x+radius,y+radius,z+radius);float ex=mythWorldX(int32_t(p[10])/512.f,originX,scale),ey=(int32_t(p[12])/512.f-originZ)*scale,ez=(int32_t(p[11])/512.f-originY)*scale;bounds.add(ex-radius,ey-radius,ez-radius);bounds.add(ex+radius,ey+radius,ez+radius);}
  if(!(reflected?reflectionCull:particleViewCull).visible(bounds))continue;if(reflected)reflectFXDrawn++;
 }order.push_back({&p,(x-center.x)*(x-center.x)+(y-center.y)*(y-center.y)+(z-center.z)*(z-center.z)});}
 std::stable_sort(order.begin(),order.end(),[](auto&a,auto&b){return a.distance>b.distance;});
 glEnable(GL_BLEND);if(reflected)glBlendFuncSeparate(GL_ONE,GL_ONE_MINUS_SRC_ALPHA,GL_ONE,GL_ONE_MINUS_SRC_ALPHA);else glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glDepthMask(GL_FALSE);glUniform1f(glGetUniformLocation(program,"alphaCutoff"),.003f);attributes(vbo);
 static ParticleStream stream;static unsigned streamGeneration=0;
 if(streamGeneration!=particleGeneration){stream=ParticleStream{};streamGeneration=particleGeneration;}
 stream.clear();
 double buildStart=seconds();
 unsigned drawn=0,missing=0,firstMissing=0,water=0,media=0,lightning=0;
 for(auto&item:order){auto&p=*item.p;auto it=particleQuads.find({particleCollectionKey(p),p[2],p[3],0});if(it==particleQuads.end()){missing++;firstMissing=p[1];continue;}Quad q=it->second;
  float x=mythWorldX(int32_t(p[4])/512.f,originX,scale),y=(int32_t(p[6])/512.f-originZ)*scale,z=(int32_t(p[5])/512.f-originY)*scale;
  if(p[15]==4){
   // Original bolt artwork stretched between world endpoints. The same captured
   // polyline feeds both eyes and reflection passes; never reroll geometry here.
   Vec3 a{x,y,z},b{mythWorldX(int32_t(p[10])/512.f,originX,scale),(int32_t(p[12])/512.f-originZ)*scale,(int32_t(p[11])/512.f-originY)*scale};
   Vec3 axis=sub(b,a);float length=sqrtf(dot(axis,axis));if(length<1e-5f)continue;axis=times(axis,1.f/length);
   Vec3 midpoint=times(add(a,b),.5f),side=arrowUp(axis,sub(Vec3{center.x,center.y,center.z},midpoint));
   float ps=q.scale*(10.f/512.f)*scale*p[8]/256.f*.5f;
   // ligh artwork runs along bitmap X; bitmap Y is transverse.
   float top=q.ry*ps,bottom=(q.ry-q.h)*ps,overlap=p[9]/512.f*scale;
   a=sub(a,times(axis,overlap*.5f));b=add(b,times(axis,overlap*.5f));
   auto vertex=[&](Vec3 pos,float across,float u,float v){pos=add(pos,times(side,across));return Vertex{pos.x,pos.y,pos.z,u,v};};
   float u0=(q.flags&1)?1:0,u1=1-u0,v0=(q.flags&2)?1:0,v1=1-v0;
   Vertex aa=vertex(a,top,u0,v0),bb=vertex(a,bottom,u0,v1),cc=vertex(b,top,u1,v0),dd=vertex(b,bottom,u1,v1),v[]={aa,bb,cc,bb,dd,cc};
   GLuint high=hdQuad(q,p[13]|(p[14]<<8),3);stream.append(high?high:particleTexture(q.texture,p[13]|(p[14]<<8),q.col),p[7]/65535.f,v,6);drawn++;lightning++;continue;
  }
  if(p[15]==3){
   // The original mesh-effect is a horizontal, expanding textured annulus.
   // Its position/height are engine state, not a billboard or a synthetic explosion.
   std::vector<Vertex> ring;ring.reserve(p[10]*6);float inner=p[8]/512.f*scale,outer=(p[8]+p[9])/512.f*scale;
   auto vertex=[&](unsigned i,float radius,float u,float v){float angle=i*6.283185307f/p[10];return Vertex{x+cosf(angle)*radius,y,z+sinf(angle)*radius,u,v};};
   for(unsigned i=0;i<p[10];i++){auto a=vertex(i,inner,0,0),b=vertex(i,outer,0,1),c=vertex(i+1,inner,1,0),d=vertex(i+1,outer,1,1);ring.insert(ring.end(),{a,b,c,b,d,c});}
   GLuint high=hdQuad(q,p[13]|(p[14]<<8),3);stream.append(high?high:particleTexture(q.texture,p[13]|(p[14]<<8),q.col),p[7]/65535.f,ring.data(),ring.size());drawn++;continue;
  }
  if(!(p[11]&(0x400000|0x2000000))&&terrain.contains(x,z)&&y<terrain.elevation(x,z)-.01f)continue;
  float dx=center.x-x,dz=center.z-z,len=sqrtf(dx*dx+dz*dz);if(len<.001f)continue;float rx=dz/len,rz=-dx/len;
  if(!(q.seqflags&4)){unsigned dir=myth_object_sprite_direction(dx,dz,p[12]/65536.f,q.views,p[15]==1?p[10]:0);auto jt=particleQuads.find({particleCollectionKey(p),p[2],p[3],dir});if(jt!=particleQuads.end())q=jt->second;}
  GLuint high=hdQuad(q,p[13]|(p[14]<<8),3);
  float ps=q.scale*(10.f/512.f)*scale,sx=p[8]/256.f,sy=p[9]/256.f;
  float l=-q.rx*ps*sx,r=(q.w-q.rx)*ps*sx,bot=(q.ry-q.h)*ps*sy,top=q.ry*ps*sy;
  unsigned flips=p[10];
  unsigned flags=q.flags;if(flips&1){float old=l;l=-r;r=-old;flags^=1;}if(flips&2){float old=bot;bot=-top;top=-old;flags^=2;}
  float u0=(flags&1)?1:0,u1=1-u0,v0=(flags&2)?1:0,v1=1-v0;
  Vertex a{x+rx*l,y+top,z+rz*l,u0,v0},b{x+rx*r,y+top,z+rz*r,u1,v0},c{x+rx*l,y+bot,z+rz*l,u0,v1},d{x+rx*r,y+bot,z+rz*r,u1,v1};Vertex v[]={a,b,c,b,d,c};
  unsigned owner=0;if(enhancedSpritePath&&p[15]==1)for(size_t i=0;i<objects.size();i++)if(physicalSprite(objects[i])&&objects[i].objectKey==p[0]){owner=spriteLightingOwner(objects[i],i);break;}
  stream.append(high?high:particleTexture(q.texture,p[13]|(p[14]<<8),q.col),p[7]/65535.f,v,6,owner);drawn++;water+=p[1]==0x5bf1af16;media+=(p[11]&0x2000000)!=0;
 }
 double buildEnd=seconds();
 stream.upload();double uploadEnd=seconds();stream.draw(glGetUniformLocation(program,"tint"),reflected);
 double submitEnd=seconds();attributes(vbo);
 glUniform4f(glGetUniformLocation(program,"tint"),1,1,1,1);glUniform1f(glGetUniformLocation(program,"alphaCutoff"),.5f);glDepthMask(GL_TRUE);glDisable(GL_BLEND);
 static uint64_t last=~uint64_t(0);if(!reflected&&last!=snapshotRevision&&tick%30==0){last=snapshotRevision;LOG("Particle FX tick=%u received=%zu drawn=%u missing=%u signature=%08x cpu_ms=%.3f uploads=%u water=%u media=%u lightningSegments=%u",tick,liveParticles.size(),drawn,missing,firstMissing,(seconds()-fxStart)*1000,particleUploads-oldUploads,water,media,lightning);LOG("Particle stages reflected=%d build_ms=%.3f upload_ms=%.3f submit_ms=%.3f draws=%zu vertices=%zu legacy_draws=%zu",reflected,(buildEnd-buildStart)*1000,(uploadEnd-buildEnd)*1000,(submitEnd-uploadEnd)*1000,stream.ranges.size(),stream.packed.size(),stream.legacyDraws);}
}
