#pragma once
#include <dirent.h>
#include <fstream>
#include <chrono>
#include "material_hash.h"
// V3 stores SHA-256 + normal XY / thickness / M&S. V2 stores normals only. V1 exact-pixel packs remain readable.
// Runtime collection/texture numbers and team palettes are deliberately not identities.
namespace sprite_material {
struct Entry{unsigned w,h,pack;size_t offset;uint64_t prefix=0;bool hashed=false;bool packed=false;unsigned effects=0,priority=0;material_hash::Digest digest{};GLuint texture=0;};
static std::vector<std::string> packs;
static size_t normalGpuBytes=0;
static uint64_t bindingRevision=1;
static uint64_t prefixHash(const unsigned char*p,size_t n){uint64_t h=1469598103934665603ull;for(size_t i=0;i<std::min(n,size_t(64));i++)h=(h^p[i])*1099511628211ull;return h;}
struct Binding{GLuint texture=0;float sx=1,sy=1,ox=0,oy=0;bool packed=false;unsigned effects=0;};
static std::vector<Entry> entries;
static std::map<GLuint,Binding> bindings;
static std::map<GLuint,std::pair<unsigned,unsigned>> textureDimensions;
static std::map<std::pair<unsigned,unsigned>,std::vector<size_t>> dimensions;
static size_t totalBytes=0;
static std::map<material_hash::Digest,size_t> hashIndex;
static std::map<std::pair<unsigned,unsigned>,bool> hashDimensions;
struct Timing {unsigned matches=0,hits=0,legacyReads=0;double hash=0,lookup=0,read=0,upload=0;} ;
static Timing timing;
// Counts submitted draw calls (both eyes and alpha passes), not visible pixels.
struct DrawStats{uint64_t pbr=0,bypass=0,vertices=0;};
static DrawStats drawStats[2];
static std::vector<unsigned> packedUploads;
static void recordDraw(bool pbr,bool reflected,unsigned vertices){if(!diagnosticEnabled)return;auto&d=drawStats[reflected?1:0];if(pbr){++d.pbr;d.vertices+=vertices;}else ++d.bypass;}
static void reportDraws(){
 for(int i=0;i<2;++i){auto&d=drawStats[i];LOG("Sprite PBR draw pass=%s pbr_calls=%llu bypass_calls=%llu pbr_vertices=%llu sky=%d reflection=%.2f specular=%.2f sss=%.2f",i?"reflection":"normal",(unsigned long long)d.pbr,(unsigned long long)d.bypass,(unsigned long long)d.vertices,sprite_pbr::sky!=0,sprite_pbr::reflection,sprite_pbr::specular,sprite_pbr::sss);d={};}
 for(size_t i=0;i<packedUploads.size();++i)if(packedUploads[i])LOG("Sprite PBR resident pack=%s uploaded_images=%u",packs[i].c_str(),packedUploads[i]);
}

static double materialTime(){return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();}
static void reportTiming(){LOG("Sprite material stages requests=%u hits=%u legacyReads=%u hash_ms=%.3f lookup_ms=%.3f read_ms=%.3f upload_submit_ms=%.3f gpu_bytes=%zu",timing.matches,timing.hits,timing.legacyReads,timing.hash*1000,timing.lookup*1000,timing.read*1000,timing.upload*1000,normalGpuBytes);}

struct Uniforms{GLuint program=0;bool valid=false;GLint enabled=-1,map=-1,uv=-1,packed=-1;};static Uniforms uniforms;
static void locate(GLuint program){if(uniforms.valid&&uniforms.program==program)return;uniforms={program,true,glGetUniformLocation(program,"spriteNormalEnabled"),glGetUniformLocation(program,"spriteNormalMap"),glGetUniformLocation(program,"spriteNormalUV"),glGetUniformLocation(program,"spritePackedMaterial")};glUniform1i(uniforms.map,11);}

static void clear(){if(timing.matches)reportTiming();timing={};drawStats[0]={};drawStats[1]={};packedUploads.clear();hashIndex.clear();hashDimensions.clear();++bindingRevision;uniforms={};for(auto&e:entries)if(e.texture)glDeleteTextures(1,&e.texture);entries.clear();textureDimensions.clear();packs.clear();bindings.clear();dimensions.clear();totalBytes=normalGpuBytes=0;}
static void load(const char*root){
 double indexStart=materialTime();clear();if(!enhancedSpritePath)return;std::string folder=std::string(root)+"/SpriteMaterials";DIR*d=opendir(folder.c_str());if(!d)return;
 std::vector<std::string> paths;while(auto*f=readdir(d)){std::string n=f->d_name;if(n.size()>6&&n.substr(n.size()-6)==".mvrnm")paths.push_back(folder+"/"+n);}closedir(d);std::sort(paths.begin(),paths.end());
 for(auto&path:paths){pumpLoadingEvents();try{
  std::ifstream in(path,std::ios::binary|std::ios::ate);auto length=in.tellg();if(length<12||length>64*1024*1024)throw std::runtime_error("material size limit");in.seekg(0);
  auto read=[&](void*p,size_t n){if(!in.read((char*)p,n))throw std::runtime_error("material truncated");};auto word=[&](){unsigned v;read(&v,4);return v;};char magic[8];read(magic,8);bool flagged=!memcmp(magic,"MVRMAT04",8);bool packed=!memcmp(magic,"MVRMAT03",8);bool hashed=flagged||packed||!memcmp(magic,"MVRNORM2",8);if(!hashed&&memcmp(magic,"MVRNORM1",8))throw std::runtime_error("material version");unsigned count=word();if(!count||count>10000)throw std::runtime_error("material count");std::vector<Entry> pending;
  size_t consumed=12;for(unsigned i=0;i<count;i++){pumpLoadingEvents();Entry e{};e.w=word();e.h=word();consumed+=8;if(!e.w||!e.h||e.w>2048||e.h>2048)throw std::runtime_error("material dimensions");size_t bytes=size_t(e.w)*e.h*4;if(consumed+(hashed?32+bytes:bytes*2)>size_t(length))throw std::runtime_error("material bounds");e.pack=packs.size();e.hashed=hashed;e.packed=packed;e.priority=flagged?4:packed?3:2;if(flagged){unsigned flags=word();consumed+=4;if(flags>1)throw std::runtime_error("material flags");e.packed=flags!=0;}if(hashed){read(e.digest.data(),32);e.offset=consumed+32;}else{e.offset=consumed;unsigned char prefix[64];size_t n=std::min(bytes,size_t(64));read(prefix,n);e.prefix=prefixHash(prefix,n);}consumed+=hashed?32+bytes:bytes*2;in.seekg(consumed);if(!in)throw std::runtime_error("material seek");pending.push_back(std::move(e));}
  if(consumed!=size_t(length))throw std::runtime_error("material trailing data");packs.push_back(path);for(auto&e:pending){if(e.hashed){auto old=hashIndex.find(e.digest);if(old==hashIndex.end()||e.priority>=entries[old->second].priority)hashIndex[e.digest]=entries.size();hashDimensions[{e.w,e.h}]=true;}else dimensions[{e.w,e.h}].push_back(entries.size());entries.push_back(std::move(e));}totalBytes+=size_t(length);
  LOG("Sprite material indexed %s frames=%u format=%s",path.c_str(),count,flagged?"MVRMAT04":packed?"MVRMAT03":(hashed?"MVRNORM2":"MVRNORM1"));
 }catch(const std::exception&e){if(loadingApp&&loadingApp->destroyRequested)throw;LOG("Sprite material skipped %s: %s",path.c_str(),e.what());}}
 LOG("Sprite material index packs=%zu entries=%zu hashed=%zu index_ms=%.3f",packs.size(),entries.size(),hashIndex.size(),(materialTime()-indexStart)*1000);
}
static Binding normalEntry(size_t index){
 auto&e=entries[index];if(e.texture)return {e.texture,1,1,0,0,e.packed,e.effects};
 double start=materialTime();size_t bytes=size_t(e.w)*e.h*4;std::ifstream in(packs[e.pack],std::ios::binary);in.seekg(e.offset+(e.hashed?0:bytes));std::vector<unsigned char> normal(bytes);
 if(!in.read((char*)normal.data(),bytes)){timing.read+=materialTime()-start;LOG("Sprite material read failed %s",packs[e.pack].c_str());return {};}
 if(e.packed){bool metal=false,thin=false,nonmetal=false;for(size_t i=0;i<bytes;i+=4){metal|=normal[i+3]>0;thin|=normal[i+2]<255;nonmetal|=normal[i+3]<255;}e.effects=(metal?5u:0u)|((thin&&nonmetal)?2u:0u);}
 timing.read+=materialTime()-start;start=materialTime();
 glGenTextures(1,&e.texture);glBindTexture(GL_TEXTURE_2D,e.texture);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,e.w,e.h,0,GL_RGBA,GL_UNSIGNED_BYTE,normal.data());glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);textureDimensions[e.texture]={e.w,e.h};normalGpuBytes+=bytes;timing.upload+=materialTime()-start;
 if(e.packed){packedUploads.resize(packs.size());++packedUploads[e.pack];}
 return {e.texture,1,1,0,0,e.packed,e.effects};
}
static Binding matchIdentity(unsigned w,unsigned h,const unsigned char*hash){
 material_hash::Digest digest{};memcpy(digest.data(),hash,32);auto it=hashIndex.find(digest);
 if(it==hashIndex.end())return {};auto&e=entries[it->second];
 return e.w==w&&e.h==h?normalEntry(it->second):Binding{};
}
static Binding match(unsigned w,unsigned h,const unsigned char*p){
 if(!enhancedSpritePath)return {};
 ++timing.matches;Binding result{};
 if(hashDimensions.count({w,h})){
  double start=materialTime();auto digest=material_hash::image(w,h,p);timing.hash+=materialTime()-start;start=materialTime();auto it=hashIndex.find(digest);timing.lookup+=materialTime()-start;
  if(it!=hashIndex.end()&&entries[it->second].w==w&&entries[it->second].h==h)result=normalEntry(it->second);
 }
 if(!result.texture){auto list=dimensions.find({w,h});if(list!=dimensions.end()){
  size_t bytes=size_t(w)*h*4;uint64_t prefix=prefixHash(p,bytes);
  for(auto index:list->second){pumpLoadingEvents();auto&e=entries[index];if(e.prefix!=prefix)continue;
   double start=materialTime();++timing.legacyReads;std::ifstream in(packs[e.pack],std::ios::binary);in.seekg(e.offset);unsigned char scratch[4096];size_t at=0;
   while(at<bytes){size_t n=std::min(bytes-at,sizeof(scratch));if(!in.read((char*)scratch,n)||memcmp(scratch,p+at,n))break;at+=n;}
   timing.read+=materialTime()-start;if(at==bytes){result=normalEntry(index);break;}
  }
 }}
 if(result.texture)++timing.hits;if(timing.matches==1||timing.matches%256==0)reportTiming();return result;
}
static void assign(GLuint texture,Binding b){++bindingRevision;if(b.texture)bindings[texture]=b;else bindings.erase(texture);}
static Binding get(GLuint texture){auto i=bindings.find(texture);return i==bindings.end()?Binding{}:i->second;}
static bool needsPbr(Binding b,bool receiver=true){
 if(!receiver||!b.texture||!b.packed)return false;
 if(sprite_pbr::debugSpecular||sprite_pbr::debugSss)return true;
 bool local=sprite_pbr::localLights>0&&((sprite_pbr::specular>0&&(b.effects&4))||(sprite_pbr::sss>0&&(b.effects&2)));
 bool sun=sprite_pbr::sun[0]>0||sprite_pbr::sun[1]>0||sprite_pbr::sun[2]>0;
 return local||(sprite_pbr::sky&&((sprite_pbr::reflection>0&&(b.effects&1))||(sun&&((sprite_pbr::specular>0&&(b.effects&4))||(sprite_pbr::sss>0&&(b.effects&2))))));
}
static void inherit(GLuint destination,GLuint source){assign(destination,get(source));}
static void bind(GLuint program,GLuint texture){if(!enhancedSpritePath)return;locate(program);auto b=get(texture);glUniform1i(uniforms.enabled,b.texture!=0);glUniform1i(uniforms.packed,b.packed);if(b.texture){glActiveTexture(GL_TEXTURE11);glBindTexture(GL_TEXTURE_2D,b.texture);glUniform4f(uniforms.uv,b.sx,b.sy,b.ox,b.oy);glActiveTexture(GL_TEXTURE0);}}
// Local to one uninterrupted stream draw: other render paths may mutate GL state.
struct StreamBinding {
 int enabled=-1,packed=-1;Binding last{};bool uvKnown=false;GLuint texture=0;bool textureKnown=false;
 void bind(GLuint program,GLuint source){
  bindResolved(program,get(source));
 }
 void bindResolved(GLuint program,Binding b){
  if(!enhancedSpritePath)return;locate(program);int on=b.texture!=0;
  if(enabled!=on){glUniform1i(uniforms.enabled,on);enabled=on;}
  if(packed!=int(b.packed)){glUniform1i(uniforms.packed,b.packed);packed=b.packed;}if(!on)return;
  if(!textureKnown||texture!=b.texture){glActiveTexture(GL_TEXTURE11);glBindTexture(GL_TEXTURE_2D,b.texture);glActiveTexture(GL_TEXTURE0);texture=b.texture;textureKnown=true;}
  if(!uvKnown||last.sx!=b.sx||last.sy!=b.sy||last.ox!=b.ox||last.oy!=b.oy){glUniform4f(uniforms.uv,b.sx,b.sy,b.ox,b.oy);last=b;uvKnown=true;}
 }
};
static void unbind(GLuint program){if(!enhancedSpritePath)return;locate(program);glUniform1i(uniforms.enabled,0);}
}
