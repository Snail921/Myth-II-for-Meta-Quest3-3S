#pragma once
#include <tuple>
#include "atlas_usage.h"
// Runtime collection IDs are scoped to this level. Source GL names are never
// persisted. Copies stay on the GPU; two duplicated edge texels preserve clamp.
namespace sprite_atlas {
static bool enabled=false;
static constexpr int side=2048,maxSide=2048,gutter=2;
static size_t gpuBytes=0;
static constexpr size_t budget=768u*1024u*1024u;
struct Rect{float sx=1,sy=1,ox=0,oy=0;};
struct Slot{GLuint texture=0;int x=0,y=0;Rect rect{};};
struct Source{unsigned w=0,h=0,col=0;bool high=false;Slot slot{};bool dirty=true,failed=false;GLuint group=0;};
struct Page{GLuint texture=0;unsigned col=0;bool normal=false;int x=0,y=0,row=0,size=side;GLuint group=0;};
static std::map<GLuint,Source> sources;
static std::map<GLuint,Source> normals;
static std::vector<Page> pages;
static GLuint readFbo=0;static unsigned generation=1;
static uint64_t copies=0,fallbacks=0,inputDraws=0,outputDraws=0;
static void load(const char*root){enabled=false;char p[512];snprintf(p,sizeof(p),"%s/sprite-atlas.enabled",root);if(FILE*f=fopen(p,"rb")){enabled=fgetc(f)=='1';fclose(f);}LOG("Sprite atlas unique_normals=1 shared_collections=1 enabled=%d page=%d budget_MiB=%zu",enabled,side,budget/1024/1024);}
static void registerTexture(GLuint tx,unsigned w,unsigned h,unsigned col,bool high=false){if(!enabled||!tx)return;auto it=sources.find(tx);if(it!=sources.end()&&it->second.w==w&&it->second.h==h&&it->second.col==col&&it->second.high==high)return;sources[tx]={w,h,col,high};sources[tx].group=high?1u:0u;}
static void changed(GLuint tx){auto i=sources.find(tx);if(i!=sources.end())i->second.dirty=true;}
static void forget(GLuint tx){sources.erase(tx);}
static atlas_usage::Values sessionPeaks{};
static void saveUsage(bool flush=true){size_t colors=0,normalPages=0;for(const auto&p:pages){if(p.normal)++normalPages;else ++colors;}atlas_usage::Values current{gpuBytes,pages.size(),colors,normalPages,sources.size(),normals.size(),copies,fallbacks};for(size_t i=0;i<current.size();i++)sessionPeaks[i]=std::max(sessionPeaks[i],current[i]);if(flush)atlas_usage::update(sessionPeaks);}
static void clear(){saveUsage();sessionPeaks={};++generation;for(auto&p:pages)glDeleteTextures(1,&p.texture);if(readFbo)glDeleteFramebuffers(1,&readFbo);readFbo=0;pages.clear();gpuBytes=0;sources.clear();normals.clear();copies=fallbacks=inputDraws=outputDraws=0;}
static Slot allocate(unsigned w,unsigned h,unsigned col,bool normal,GLuint group=0){
 int width=int(w)+2*gutter,height=int(h)+2*gutter;if(width>maxSide||height>maxSide)return {};int pageSide=side;while(pageSide<std::max(width,height))pageSide*=2;
 for(size_t i=0;i<=pages.size();i++){
  if(i==pages.size()){
   if(gpuBytes+size_t(pageSide)*pageSide*4>budget)return {};
   Page p;p.size=pageSide;p.col=col;p.normal=normal;p.group=group;glGenTextures(1,&p.texture);glBindTexture(GL_TEXTURE_2D,p.texture);glTexStorage2D(GL_TEXTURE_2D,1,GL_RGBA8,p.size,p.size);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);pages.push_back(p);gpuBytes+=size_t(p.size)*p.size*4;
  }
  auto&p=pages[i];if(p.group!=group||p.normal!=normal||width>p.size||height>p.size)continue;
  int x=p.x,y=p.y,row=p.row;if(x+width>p.size){x=0;y+=row;row=0;}if(y+height>p.size)continue;
  p.x=x+width;p.y=y;p.row=std::max(row,height);
  return {p.texture,x+gutter,y+gutter,{float(w)/p.size,float(h)/p.size,float(x+gutter)/p.size,float(y+gutter)/p.size}};
 }return {};
}
static Slot resident(GLuint tx,Source&s,bool normal){
 if(s.failed)return {};if(s.slot.texture&&!s.dirty)return s.slot;
 GLint oldRead=0,oldTexture=0;glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING,&oldRead);glGetIntegerv(GL_TEXTURE_BINDING_2D,&oldTexture);
 if(!readFbo)glGenFramebuffers(1,&readFbo);glBindFramebuffer(GL_READ_FRAMEBUFFER,readFbo);glFramebufferTexture2D(GL_READ_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,tx,0);glReadBuffer(GL_COLOR_ATTACHMENT0);
 bool ok=glCheckFramebufferStatus(GL_READ_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE;
 if(ok&&!s.slot.texture)s.slot=allocate(s.w,s.h,s.col,normal,s.group);
 if(ok&&s.slot.texture){
  auto&a=s.slot;glBindTexture(GL_TEXTURE_2D,a.texture);int w=int(s.w),h=int(s.h);
  glCopyTexSubImage2D(GL_TEXTURE_2D,0,a.x,a.y,0,0,w,h);
  for(int g=1;g<=gutter;g++){glCopyTexSubImage2D(GL_TEXTURE_2D,0,a.x-g,a.y,0,0,1,h);glCopyTexSubImage2D(GL_TEXTURE_2D,0,a.x+w-1+g,a.y,w-1,0,1,h);glCopyTexSubImage2D(GL_TEXTURE_2D,0,a.x,a.y-g,0,0,w,1);glCopyTexSubImage2D(GL_TEXTURE_2D,0,a.x,a.y+h-1+g,0,h-1,w,1);for(int k=1;k<=gutter;k++){glCopyTexSubImage2D(GL_TEXTURE_2D,0,a.x-g,a.y-k,0,0,1,1);glCopyTexSubImage2D(GL_TEXTURE_2D,0,a.x+w-1+g,a.y-k,w-1,0,1,1);glCopyTexSubImage2D(GL_TEXTURE_2D,0,a.x-g,a.y+h-1+k,0,h-1,1,1);glCopyTexSubImage2D(GL_TEXTURE_2D,0,a.x+w-1+g,a.y+h-1+k,w-1,h-1,1,1);}}
  ++copies;s.dirty=false;
 }else{s.failed=true;++fallbacks;}
 glFramebufferTexture2D(GL_READ_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,0,0);glBindFramebuffer(GL_READ_FRAMEBUFFER,oldRead);glBindTexture(GL_TEXTURE_2D,oldTexture);return s.failed?Slot{}:s.slot;
}
struct Attributes{Rect color{},normal{},mapping{};};
struct Resolved{GLuint texture=0; sprite_material::Binding material{};bool high=false,atlas=false;Attributes attributes{};};
static Resolved resolve(GLuint tx,bool high){
 Resolved r;r.texture=tx;r.high=high;r.material=enhancedSpritePath?sprite_material::get(tx):sprite_material::Binding{};
 if(!enabled)return r;auto i=sources.find(tx);if(i==sources.end())return r;auto&s=i->second;r.high=s.high;
 // Share immutable normal images across all color pages and collections.
 // Source GL names identify normals within this level; clear() resets residency.
 // Independent UV rectangles preserve crop/flip mappings without extra copies.
 auto c=resident(tx,s,false);if(!c.texture)return r;
 Slot n;if(r.material.texture){auto key=r.material.texture;auto it=normals.find(key);if(it==normals.end()){auto d=sprite_material::textureDimensions.find(r.material.texture);if(d==sprite_material::textureDimensions.end())return r;Source normal{d->second.first,d->second.second,s.col};it=normals.emplace(key,normal).first;}n=resident(r.material.texture,it->second,true);if(!n.texture)return r;}
 r.attributes={c.rect,n.rect,{r.material.sx,r.material.sy,r.material.ox,r.material.oy}};r.texture=c.texture;r.material={n.texture,1,1,0,0};r.atlas=true;return r;
}
static auto key(const Resolved&r){return std::make_tuple(r.atlas,r.texture,r.material.texture,r.high,r.material.sx,r.material.sy,r.material.ox,r.material.oy);}
struct AttributeStream{
 std::vector<Attributes> data;GLuint buffers[3]{};unsigned epoch=0;
 void append(const Resolved&r,size_t n){if(enabled)data.insert(data.end(),n,r.attributes);}
 void upload(unsigned cursor){if(epoch!=generation){for(auto&b:buffers)b=0;epoch=generation;}if(!enabled||data.empty())return;if(!buffers[cursor])glGenBuffers(1,&buffers[cursor]);glBindBuffer(GL_ARRAY_BUFFER,buffers[cursor]);glBufferData(GL_ARRAY_BUFFER,data.size()*sizeof(Attributes),data.data(),GL_STREAM_DRAW);}
 void bind(unsigned cursor){if(!enabled)return;glBindBuffer(GL_ARRAY_BUFFER,buffers[cursor]);for(int i=0;i<3;i++){glEnableVertexAttribArray(6+i);glVertexAttribPointer(6+i,4,GL_FLOAT,GL_FALSE,sizeof(Attributes),(void*)(size_t(i)*sizeof(Rect)));}}
 static void end(){if(!enabled)return;for(int i=6;i<9;i++)glDisableVertexAttribArray(i);glUniform1i(glGetUniformLocation(program,"spriteAtlas"),0);}
};
static void report(){if(!enabled)return;saveUsage(false);if(seconds()-atlas_usage::lastCheck>=30){saveUsage();atlas_usage::lastCheck=seconds();}size_t colorPages=0,normalPages=0;for(const auto&p:pages){if(p.normal)++normalPages;else ++colorPages;}LOG("Sprite atlas layout page_side=%d color_pages=%zu normal_pages=%zu",side,colorPages,normalPages);LOG("Sprite atlas pages=%zu MiB=%zu sources=%zu normal_images=%zu copies=%llu fallback=%llu source_ranges=%llu batched_ranges=%llu",pages.size(),gpuBytes/1024/1024,sources.size(),normals.size(),(unsigned long long)copies,(unsigned long long)fallbacks,(unsigned long long)inputDraws,(unsigned long long)outputDraws);inputDraws=outputDraws=0;}
}
