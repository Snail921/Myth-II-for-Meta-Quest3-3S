#pragma once
#include "sprite_light_bounds.h"
static bool spriteCutoutOnePass=true;
static void loadSpriteCutout(const char*root){spriteCutoutOnePass=true;char path[512];snprintf(path,sizeof(path),"%s/sprite-cutout-one-pass.enabled",root);if(FILE*f=fopen(path,"rb")){spriteCutoutOnePass=fgetc(f)=='1';fclose(f);}}
static uint64_t streamCandidateRanges=0,streamCandidateGroups6=0;
static uint64_t streamUploadBytes=0,streamUploadCalls=0,streamDrawCalls=0,streamVertices=0;static double streamUploadSeconds=0;
// One orphaned upload per eye pass, instead of one allocation/upload per texture.
// Three persistent buffer names rotate; orphaning avoids overwriting queued GPU reads.
struct SpriteStream {
 struct Range{GLuint texture;GLint first;GLsizei count;sprite_material::Binding material{};bool high=false,atlas=false;};
 uint64_t materialRevision=0;
 SpriteLightBoundsStream lightBounds;
 sprite_atlas::AttributeStream atlasAttributes;
 GLuint ownerBuffers[3]{};std::vector<float> owners;
 static GLuint textureKey(GLuint k){return k;}static unsigned ownerKey(GLuint){return 0;}
 static GLuint textureKey(std::pair<GLuint,unsigned> k){return k.first;}static unsigned ownerKey(std::pair<GLuint,unsigned> k){return k.second;}
 GLuint buffers[3]{};unsigned cursor=0;size_t capacity[3]{};
 std::vector<Vertex> packed;std::vector<Range> ranges;
 struct Input{GLuint texture;unsigned owner;const Vertex* vertices;size_t count;const std::array<float,4>* bounds=nullptr;};
 struct Batch{sprite_atlas::Resolved resolved;Input input;size_t order;};
 std::vector<Batch> ordered;std::vector<Input> legacyInputs;
 template<class K> void upload(const std::map<K,std::vector<Vertex>>&batches){
  legacyInputs.clear();for(auto&b:batches)if(!b.second.empty())legacyInputs.push_back({textureKey(b.first),ownerKey(b.first),b.second.data(),b.second.size()});uploadReferences(legacyInputs);
 }
 void uploadReferences(const std::vector<Input>&inputs){
  bool apiSample=spriteApi.sample(2);double profileStart=diagnosticEnabled?seconds():0;
  packed.clear();ranges.clear();owners.clear();atlasAttributes.data.clear();lightBounds.bounds.clear();ordered.clear();
  size_t total=0;for(auto&b:inputs)if(b.count){ordered.push_back({sprite_atlas::resolve(b.texture,hd::textures.count(b.texture)!=0),b,ordered.size()});total+=b.count;}
  // Reused reference storage; no intermediate vertex arrays or stable_sort scratch.
  std::sort(ordered.begin(),ordered.end(),[](const Batch&a,const Batch&b){auto ak=sprite_atlas::key(a.resolved),bk=sprite_atlas::key(b.resolved);if(ak!=bk)return ak<bk;return std::tie(a.input.texture,a.input.owner,a.order)<std::tie(b.input.texture,b.input.owner,b.order);});
  size_t reserve=(total+1023)&~size_t(1023);packed.reserve(reserve);if(enhancedSpritePath){owners.reserve(reserve);lightBounds.bounds.reserve(reserve);}if(sprite_atlas::enabled)atlasAttributes.data.reserve(reserve);
  GLuint previousSource=0;
  for(auto&b:ordered){auto&r=b.resolved;auto&v=b.input;if(v.texture!=previousSource){++sprite_atlas::inputDraws;previousSource=v.texture;}bool merge=false;if(!ranges.empty()){auto&last=ranges.back();merge=last.texture==r.texture&&last.atlas==r.atlas&&last.high==r.high&&last.material.texture==r.material.texture&&last.material.sx==r.material.sx&&last.material.sy==r.material.sy&&last.material.ox==r.material.ox&&last.material.oy==r.material.oy;}
   if(merge)ranges.back().count+=(GLsizei)v.count;else ranges.push_back({r.texture,(GLint)packed.size(),(GLsizei)v.count,r.material,r.high,r.atlas});
   packed.insert(packed.end(),v.vertices,v.vertices+v.count);atlasAttributes.append(r,v.count);
   if(enhancedSpritePath){owners.insert(owners.end(),v.count,float(v.owner));if(v.bounds)lightBounds.bounds.insert(lightBounds.bounds.end(),v.count,*v.bounds);else for(size_t i=0;i<v.count;i+=6)lightBounds.append(v.vertices+i,std::min(size_t(6),v.count-i));}}
  sprite_atlas::outputDraws+=ranges.size();materialRevision=sprite_material::bindingRevision;
  cursor=(cursor+1)%3;if(!buffers[cursor])glGenBuffers(1,&buffers[cursor]);
  size_t bytes=packed.size()*sizeof(Vertex);if(!bytes)return;
  if(capacity[cursor]<bytes)capacity[cursor]=(bytes+65535)&~size_t(65535);
  glBindBuffer(GL_ARRAY_BUFFER,buffers[cursor]);spriteApi.measure(apiSample,2,"MythVR/glBufferData-orphan",0,capacity[cursor],[&]{glBufferData(GL_ARRAY_BUFFER,capacity[cursor],nullptr,GL_STREAM_DRAW);});spriteApi.measure(apiSample,2,"MythVR/glBufferSubData",0,bytes,[&]{glBufferSubData(GL_ARRAY_BUFFER,0,bytes,packed.data());});
  if(enhancedSpritePath){if(!ownerBuffers[cursor])glGenBuffers(1,&ownerBuffers[cursor]);glBindBuffer(GL_ARRAY_BUFFER,ownerBuffers[cursor]);glBufferData(GL_ARRAY_BUFFER,owners.size()*sizeof(float),owners.data(),GL_STREAM_DRAW);lightBounds.upload(cursor);}
  atlasAttributes.upload(cursor);
  if(diagnosticEnabled){streamUploadBytes+=bytes+(enhancedSpritePath?owners.size()*sizeof(float)+lightBounds.bounds.size()*sizeof(lightBounds.bounds[0]):0)+(sprite_atlas::enabled?atlasAttributes.data.size()*sizeof(sprite_atlas::Attributes):0);streamUploadCalls++;streamUploadSeconds+=seconds()-profileStart;}
 }
 void draw(bool reflectedPass){
  if(diagnosticEnabled&&enhancedSpritePath){auto&c=vertexLodCounts[reflectedPass?1:0];if(!c.sampled){c.sampled=true;for(size_t i=0;i<owners.size();i+=6)lightBounds.count(c,i,6,owners[i]>0);}}
  if(ranges.empty())return;
  if(!sprite_atlas::enabled&&enhancedSpritePath&&materialRevision!=sprite_material::bindingRevision){for(auto&r:ranges)r.material=sprite_material::get(r.texture);materialRevision=sprite_material::bindingRevision;
   if(diagnosticEnabled){size_t used=0;Range*previous=nullptr;bool previousHD=false;for(auto&r:ranges){bool high=r.high;auto&a=r.material;bool same=previous&&previousHD==high&&previous->material.texture==a.texture&&previous->material.sx==a.sx&&previous->material.sy==a.sy&&previous->material.ox==a.ox&&previous->material.oy==a.oy;if(!same||used==6){streamCandidateGroups6++;used=0;}used++;streamCandidateRanges++;previous=&r;previousHD=high;}}
  }
bool apiSample=spriteApi.sample(reflectedPass);SpriteDetailScope detail(spriteDetail.sample(reflectedPass),reflectedPass,0);attributes(buffers[cursor]);if(enhancedSpritePath){glBindBuffer(GL_ARRAY_BUFFER,ownerBuffers[cursor]);glEnableVertexAttribArray(4);glVertexAttribPointer(4,1,GL_FLOAT,GL_FALSE,sizeof(float),nullptr);}auto cutoff=glGetUniformLocation(program,"alphaCutoff"),soft=glGetUniformLocation(program,"softOnly"),cutout=glGetUniformLocation(program,"spriteCutout");
  lightBounds.bind(cursor);atlasAttributes.bind(cursor);GLint atlasLocation=sprite_atlas::enabled?glGetUniformLocation(program,"spriteAtlas"):-1;int previousAtlas=-1;sprite_material::StreamBinding materialState;int previousCutout=-1;float previousCutoff=-1;bool hasHD=false;
  for(auto&r:ranges){detail.next(1);if(sprite_atlas::enabled&&previousAtlas!=int(r.atlas)){glUniform1i(atlasLocation,r.atlas);previousAtlas=r.atlas;}bool high=r.high;hasHD|=high;int desiredCutout=high&&spriteCutoutOnePass;float desiredCutoff=high&&!spriteCutoutOnePass?.995f:.5f;if(previousCutout!=desiredCutout){spriteApi.measure(apiSample,reflectedPass,"MythVR/glUniform-cutout",r.texture,1,[&]{glUniform1i(cutout,desiredCutout);});previousCutout=desiredCutout;}if(previousCutoff!=desiredCutoff){spriteApi.measure(apiSample,reflectedPass,"MythVR/glUniform-cutoff",r.texture,1,[&]{glUniform1f(cutoff,desiredCutoff);});previousCutoff=desiredCutoff;}detail.next(2);spriteApi.measure(apiSample,reflectedPass,"MythVR/normal-material",r.texture,r.count,[&]{materialState.bindResolved(program,r.material);});detail.next(3);spriteApi.measure(apiSample,reflectedPass,"MythVR/glBindTexture",r.texture,r.count,[&]{glBindTexture(GL_TEXTURE_2D,r.texture);});detail.next(4);if(diagnosticEnabled){streamDrawCalls++;streamVertices+=r.count;}spriteApi.measure(apiSample,reflectedPass,"MythVR/glDrawArrays",r.texture,r.count,[&]{glDrawArrays(GL_TRIANGLES,r.first,r.count);});}
  detail.next(5);if(hasHD&&!spriteCutoutOnePass){float reflected=0;glGetUniformfv(program,glGetUniformLocation(program,"reflectionClip"),&reflected);glEnable(GL_BLEND);glBlendFunc(reflected>.5f?GL_ONE:GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glDepthMask(GL_FALSE);glUniform1i(soft,1);glUniform1f(cutoff,.003f);for(auto&r:ranges)if(r.high){if(sprite_atlas::enabled&&previousAtlas!=int(r.atlas)){glUniform1i(atlasLocation,r.atlas);previousAtlas=r.atlas;}detail.next(2);spriteApi.measure(apiSample,reflectedPass,"MythVR/normal-material",r.texture,r.count,[&]{materialState.bindResolved(program,r.material);});detail.next(3);spriteApi.measure(apiSample,reflectedPass,"MythVR/glBindTexture",r.texture,r.count,[&]{glBindTexture(GL_TEXTURE_2D,r.texture);});detail.next(6);if(diagnosticEnabled){streamDrawCalls++;streamVertices+=r.count;}spriteApi.measure(apiSample,reflectedPass,"MythVR/glDrawArrays",r.texture,r.count,[&]{glDrawArrays(GL_TRIANGLES,r.first,r.count);});}glUniform1i(soft,0);glDepthMask(GL_TRUE);glDisable(GL_BLEND);}
  detail.next(7);sprite_atlas::AttributeStream::end();glUniform1i(cutout,0);SpriteLightBoundsStream::end();sprite_material::unbind(program);glUniform1f(cutoff,.5f);glDisableVertexAttribArray(4);glVertexAttrib1f(4,0);
 }
};
