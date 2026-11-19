#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <cstdio>
#include <cassert>
#include <cmath>
#include <map>
#include <set>
#include <vector>
#include <array>
#include <algorithm>
#include <stdexcept>
static bool enhancedSpritePath=true,diagnosticEnabled=false,shadowDebugEnabled=false;
static void pumpLoadingEvents(){}
struct AppStub{bool destroyRequested=false;};static AppStub*loadingApp=nullptr;
struct Vertex{float x,y,z,u,v;};static GLuint program;
struct Camera{float x=0,y=0,z=5;}camera;
static double seconds(){return 0;}
#define LOG(...) do{printf(__VA_ARGS__);puts("");}while(0)
#include "../src/vr/sprite_material.h"
#include "../src/vr/sprite_atlas.h"
#include "../src/vr/color_pipeline.h"
#include "../src/vr/render_culling.h"
namespace hd{static std::set<GLuint> textures;}
struct Profile{bool sample(int){return false;}template<class F>void measure(bool,int,const char*,GLuint,size_t,F f){f();}}spriteApi,spriteDetail;
struct SpriteDetailScope{SpriteDetailScope(bool,int,int){}void next(int){}};
static void attributes(GLuint b){glBindBuffer(GL_ARRAY_BUFFER,b);glEnableVertexAttribArray(0);glVertexAttribPointer(0,3,GL_FLOAT,0,sizeof(Vertex),nullptr);glEnableVertexAttribArray(1);glVertexAttribPointer(1,2,GL_FLOAT,0,sizeof(Vertex),(void*)12);}
#include "../src/vr/sprite_stream.h"
#include "../src/vr/particle_stream.h"
#include "../src/vr/weather_stream.h"
static GLuint compile(GLenum type,const std::string&s){GLuint x=glCreateShader(type);const char*p=s.c_str();glShaderSource(x,1,&p,0);glCompileShader(x);GLint ok;glGetShaderiv(x,GL_COMPILE_STATUS,&ok);if(!ok){char b[8192];glGetShaderInfoLog(x,sizeof(b),0,b);puts(b);}assert(ok);return x;}
static GLuint texture(int w,int h,int seed,bool normal=false){std::vector<unsigned char>p(w*h*4);for(int y=0;y<h;y++)for(int x=0;x<w;x++){int i=(y*w+x)*4;p[i]=normal?128+x*80/w:40+(x*21+seed)%180;p[i+1]=normal?128+y*60/h:40+(y*31+seed)%180;p[i+2]=normal?230:120;p[i+3]=(x==0||y==0)?160:255;}GLuint tx;glGenTextures(1,&tx);glBindTexture(GL_TEXTURE_2D,tx);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,p.data());glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);return tx;}
static std::vector<Vertex> quad(float l,float r,bool flip=false){float u=flip?1:0,v=1-u;return {{l,-1,0,u,0},{r,-1,0,v,0},{l,1,0,u,1},{l,1,0,u,1},{r,-1,0,v,0},{r,1,0,v,1}};}
static std::vector<unsigned char> pixels(){std::vector<unsigned char>p(64*64*4);glReadPixels(0,0,64,64,GL_RGBA,GL_UNSIGNED_BYTE,p.data());return p;}
static void same(const std::vector<unsigned char>&a,const std::vector<unsigned char>&b){int max=0;for(size_t i=0;i<a.size();i++)max=std::max(max,abs(int(a[i])-int(b[i])));printf("pixel difference max=%d\n",max);assert(max<=2);}
int main(int argc,char**argv){
 auto d=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(d,0,0));EGLint a[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};EGLConfig cfg;EGLint n;assert(eglChooseConfig(d,a,&cfg,1,&n)&&n);EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE},pa[]={EGL_WIDTH,64,EGL_HEIGHT,64,EGL_NONE};auto ctx=eglCreateContext(d,cfg,EGL_NO_CONTEXT,ca);auto surface=eglCreatePbufferSurface(d,cfg,pa);assert(eglMakeCurrent(d,surface,surface,ctx));GLuint vao;glGenVertexArrays(1,&vao);glBindVertexArray(vao);glViewport(0,0,64,64);glClearColor(0,0,0,0);
 for(bool shadows:{false,true})for(bool lighting:{false,true}){GLuint p=glCreateProgram();glAttachShader(p,compile(GL_VERTEX_SHADER,spriteShaderSource(mythVertexShader,shadows,lighting,false)));glAttachShader(p,compile(GL_FRAGMENT_SHADER,spriteShaderSource(mythFragmentShader,shadows,lighting,false)));glLinkProgram(p);GLint ok;glGetProgramiv(p,GL_LINK_STATUS,&ok);assert(ok);assert(glGetUniformLocation(p,"spriteAtlas")==-1&&glGetAttribLocation(p,"atlasColorRect")==-1);glDeleteProgram(p);}
 GLuint color[2]={texture(17,23,0),texture(21,19,85)},normal[2]={texture(11,13,0,true),texture(9,15,0,true)};
 for(int i=0;i<2;i++){sprite_material::assign(color[i],{normal[i],i?-.8f:1.2f,.9f,i?.9f:-.1f,.05f});sprite_material::textureDimensions[normal[i]]={i?9u:11u,i?15u:13u};hd::textures.insert(color[i]);}
 for(bool shadows:{false,true})for(bool lighting:{false,true}){
  enhancedSpritePath=shadows||lighting;program=glCreateProgram();glAttachShader(program,compile(GL_VERTEX_SHADER,spriteShaderSource(mythVertexShader,shadows,lighting)));glAttachShader(program,compile(GL_FRAGMENT_SHADER,spriteShaderSource(mythFragmentShader,shadows,lighting)));glLinkProgram(program);GLint ok;glGetProgramiv(program,GL_LINK_STATUS,&ok);if(!ok){char b[8192];glGetProgramInfoLog(program,sizeof(b),0,b);puts(b);}assert(ok);glUseProgram(program);sprite_material::uniforms={};auto u=[](const char*s){return glGetUniformLocation(program,s);};
  float ident[]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};glUniformMatrix4fv(u("mvp"),1,0,ident);glUniformMatrix4fv(u("spriteSunMatrix"),1,0,ident);glUniform4f(u("tint"),1,1,1,1);glUniform3f(u("fogEye"),0,0,5);glUniform3f(u("spriteLightingEye"),0,0,5);glUniform3f(u("spriteSunDirection"),.5,.7,.5);glUniform1i(u("spriteShadows"),shadows);glUniform1i(u("spriteSkipShadowLookup"),1);glUniform1f(u("spriteShadowAmount"),.7);glUniform1i(u("spriteLightCount"),lighting?1:0);glUniform4f(u("spriteLights[0]"),.5,1,2,.04);glUniform4f(u("spriteLightColors[0]"),1,.3,.1,.9);glUniform1i(u("spriteObjectLightLimit"),8);glUniform1i(u("detailTex"),1);glUniform1i(u("tex"),0);glUniform1i(u("spriteNormalMap"),11);glUniform1f(u("reflectionHeight"),-10);
  for(int reflected=0;reflected<2;reflected++)for(bool smooth:{false,true}){
   glUniform1f(u("reflectionClip"),reflected);spriteCutoutOnePass=!smooth;
   std::vector<unsigned char>reference;for(bool atlas:{false,true}){sprite_atlas::enabled=atlas;for(int i=0;i<2;i++)sprite_atlas::registerTexture(color[i],i?21:17,i?19:23,7+i,true);
    SpriteStream stream;std::map<std::pair<GLuint,unsigned>,std::vector<Vertex>> batches;batches[{color[0],1}]=quad(-1,0);batches[{color[1],2}]=quad(0,1,true);stream.upload(batches);assert(stream.ranges.size()==(atlas?1u:2u));glClear(GL_COLOR_BUFFER_BIT);stream.draw(reflected);auto p=pixels();if(atlas)same(reference,p);else reference=p;assert(glGetError()==GL_NO_ERROR);
   }
  }
  // Weather must retain original textures/UVs even when registered in an atlas.
  if(!shadows&&!lighting){
   std::map<GLuint,std::vector<Vertex>> rain;rain[color[0]]=quad(-1,0);rain[color[1]]=quad(0,1,true);
   sprite_atlas::enabled=false;glUniform1i(u("spriteAtlas"),0);glUniform1i(u("spriteCutout"),0);glUniform1i(u("softOnly"),0);glUniform1f(u("alphaCutoff"),.5f);glUniform1f(u("reflectionClip"),0);glUniform4f(u("tint"),1,1,1,1);glClear(GL_COLOR_BUFFER_BIT);GLuint raw;glGenBuffers(1,&raw);for(auto&b:rain){attributes(raw);glBufferData(GL_ARRAY_BUFFER,b.second.size()*sizeof(Vertex),b.second.data(),GL_STREAM_DRAW);glBindTexture(GL_TEXTURE_2D,b.first);glDrawArrays(GL_TRIANGLES,0,b.second.size());}auto expected=pixels();glDeleteBuffers(1,&raw);
   sprite_atlas::enabled=true;auto pagesBefore=sprite_atlas::pages.size(),copiesBefore=sprite_atlas::copies;
   WeatherStream weather;weather.upload(rain);assert(weather.ranges.size()==2&&weather.packed.size()==12);assert(weather.ranges[0].texture==rain.begin()->first);
   assert(sprite_atlas::pages.size()==pagesBefore&&sprite_atlas::copies==copiesBefore);
   glUniform1i(u("spriteAtlas"),0);glUniform1f(u("alphaCutoff"),.5f);glClear(GL_COLOR_BUFFER_BIT);weather.draw();same(expected,pixels());auto cursor=weather.cursor;glClear(GL_COLOR_BUFFER_BIT);weather.draw();same(expected,pixels());assert(weather.cursor==cursor);assert(glGetError()==GL_NO_ERROR);
   puts("PASS weather original texture/UV isolation with atlas ON, flip and shared upload");
  }
  // Preserve blended ordering; different owners/alpha remain distinct draws.
  std::vector<unsigned char>reference;for(bool atlas:{false,true}){sprite_atlas::enabled=atlas;ParticleStream stream;auto q=quad(-1,1,true);stream.append(color[0],.6f,q.data(),q.size(),1);stream.append(color[1],.6f,q.data(),q.size(),1);assert(stream.ranges.size()==(atlas?1u:2u));stream.append(color[0],.3f,q.data(),q.size(),2);assert(stream.ranges.size()==(atlas?2u:3u));stream.upload();glUniform1f(u("alphaCutoff"),.003);glClear(GL_COLOR_BUFFER_BIT);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);stream.draw(u("tint"));glDisable(GL_BLEND);auto p=pixels();if(atlas)same(reference,p);else reference=p;assert(glGetError()==GL_NO_ERROR);}
  // Production reference path and binocular union: compare each eye against
  // its independently culled/uploaded baseline, including edge-only sprites.
  sprite_atlas::enabled=true;spriteCutoutOnePass=true;glUniform1f(u("reflectionClip"),0);glUniform4f(u("tint"),1,1,1,1);
  std::array<std::array<float,16>,2> matrices;for(auto&m:matrices)std::copy(ident,ident+16,m.begin());matrices[0][12]=.3f;matrices[1][12]=-.3f;
  rendercull::StereoFrustum stereo;stereo.set(matrices);
  std::vector<std::vector<Vertex>> geometry{quad(-1.4,-1.1),quad(-.3,.3,true),quad(1.1,1.4),quad(3,4)};
  std::vector<std::array<float,4>> bounds;std::vector<rendercull::Bounds> boxes;
  for(auto&v:geometry){bounds.push_back(SpriteLightBoundsStream::calculate(v.data(),v.size()));rendercull::Bounds box;for(auto&p:v)box.add(p.x,p.y,p.z);boxes.push_back(box);}
  std::vector<SpriteStream::Input> refs;for(size_t i=0;i<geometry.size();i++)if(stereo.visible(boxes[i]))refs.push_back({color[i%2],unsigned(i+1),geometry[i].data(),6,&bounds[i]});assert(refs.size()==3);
  SpriteStream unionStream;unionStream.uploadReferences(refs);auto cursor=unionStream.cursor;
  for(int eye=0;eye<2;eye++){std::map<std::pair<GLuint,unsigned>,std::vector<Vertex>> separate;for(size_t i=0;i<geometry.size();i++)if(stereo.eyes[eye].visible(boxes[i]))separate[{color[i%2],unsigned(i+1)}]=geometry[i];assert(separate.size()==2);SpriteStream reference;reference.upload(separate);glUniformMatrix4fv(u("mvp"),1,0,matrices[eye].data());glClear(GL_COLOR_BUFFER_BIT);reference.draw(false);auto expected=pixels();glClear(GL_COLOR_BUFFER_BIT);unionStream.draw(false);same(expected,pixels());assert(unionStream.cursor==cursor);}
  assert(glGetError()==GL_NO_ERROR);
  glDeleteProgram(program);
 }
 sprite_atlas::enabled=true;enhancedSpritePath=false;
 auto old=sprite_atlas::resolve(color[0],true);GLuint oldPage=old.texture;auto count=sprite_atlas::copies;assert(sprite_atlas::resolve(color[0],true).texture==oldPage&&sprite_atlas::copies==count);
 glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,color[0]);unsigned char changed[]={200,30,50,255};glTexSubImage2D(GL_TEXTURE_2D,0,0,0,1,1,GL_RGBA,GL_UNSIGNED_BYTE,changed);sprite_atlas::changed(color[0]);auto updated=sprite_atlas::resolve(color[0],true);assert(updated.texture==oldPage&&sprite_atlas::copies==count+1);
 GLuint fb;glGenFramebuffers(1,&fb);glBindFramebuffer(GL_FRAMEBUFFER,fb);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,oldPage,0);unsigned char px[4];glReadPixels(sprite_atlas::sources[color[0]].slot.x-2,sprite_atlas::sources[color[0]].slot.y-2,1,1,GL_RGBA,GL_UNSIGNED_BYTE,px);assert(!memcmp(px,changed,4));glBindFramebuffer(GL_FRAMEBUFFER,0);glDeleteFramebuffers(1,&fb);
 // Cross-collection sharing, full-page rollover, oversize fallback, reused GL name.
 GLuint other=texture(4,4,1);sprite_atlas::registerTexture(other,4,4,8,true);assert(sprite_atlas::resolve(other,false).texture==oldPage);
 GLuint big=texture(1020,1020,1);sprite_atlas::registerTexture(big,1020,1020,7);assert(sprite_atlas::resolve(big,false).texture!=oldPage);
 // Force a true rollover in the same color pool; ensure all pages use 2K.
 GLuint full=texture(2044,2044,1);sprite_atlas::registerTexture(full,2044,2044,9,true);
 auto fullPage=sprite_atlas::resolve(full,true);assert(fullPage.atlas&&fullPage.texture!=oldPage);
 for(const auto&page:sprite_atlas::pages)assert(page.size==2048);
 // One immutable normal must be resident once even across color page pools.
 enhancedSpritePath=true;
 auto sharedNormal=sprite_atlas::resolve(color[0],true);
 auto normalCount=sprite_atlas::normals.size();auto beforeCopies=sprite_atlas::copies;
 sprite_material::assign(big,{normal[0],-.8f,.9f,.9f,.05f});
 auto sharedBig=sprite_atlas::resolve(big,false);
 assert(sharedBig.atlas&&sharedBig.texture!=sharedNormal.texture);
 assert(sharedBig.material.texture==sharedNormal.material.texture);
 assert(sprite_atlas::normals.size()==normalCount&&sprite_atlas::copies==beforeCopies);
 assert(!memcmp(&sharedBig.attributes.normal,&sharedNormal.attributes.normal,sizeof(sprite_atlas::Rect)));
 assert(sharedBig.attributes.mapping.sx==-.8f&&sharedBig.attributes.mapping.ox==.9f);
 // Repeated resolves and another collection must not copy the shared normal.
 sprite_material::assign(other,{normal[0],1,1,0,0});
 for(int repeat=0;repeat<100;repeat++){assert(sprite_atlas::resolve(other,true).material.texture==sharedNormal.material.texture);assert(sprite_atlas::resolve(big,false).atlas);}
 assert(sprite_atlas::normals.size()==normalCount&&sprite_atlas::copies==beforeCopies);
 puts("PASS unique normal residency across color pages, collections and UV mappings");
 enhancedSpritePath=false;
 GLuint oversized=texture(2048,2,1);sprite_atlas::registerTexture(oversized,2048,2,7);assert(!sprite_atlas::resolve(oversized,false).atlas);
 sprite_atlas::forget(color[0]);sprite_atlas::registerTexture(color[0],17,23,7);assert(sprite_atlas::resolve(color[0],true).atlas);
 assert(glGetError()==GL_NO_ERROR);sprite_atlas::report();sprite_atlas::clear();assert(!glIsTexture(oldPage)&&sprite_atlas::sources.empty()&&sprite_atlas::normals.empty()&&sprite_atlas::gpuBytes==0);
 if(argc==2){
  std::ifstream in(argv[1],std::ios::binary);char magic[8];assert(in.read(magic,8));bool fx=!memcmp(magic,"MYTHFX03",8)||!memcmp(magic,"MYTHFX02",8);assert(fx||!memcmp(magic,"MYTHVR01",8));unsigned w=0,h=0,nt,nq;if(!fx){in.read((char*)&w,4);in.read((char*)&h,4);}in.read((char*)&nt,4);in.read((char*)&nq,4);if(!fx)in.seekg(size_t(w)*h*2,std::ios::cur);
  struct Image{GLuint tx;unsigned w,h,col=~0u;};std::vector<Image> images;
  for(unsigned i=0;i<nt;i++){unsigned iw,ih;in.read((char*)&iw,4);in.read((char*)&ih,4);std::vector<unsigned char>rgba(size_t(iw)*ih*4);assert(in.read((char*)rgba.data(),rgba.size()));if(fx)in.seekg(size_t(iw)*ih,std::ios::cur);GLuint tx;glGenTextures(1,&tx);glBindTexture(GL_TEXTURE_2D,tx);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,iw,ih,0,GL_RGBA,GL_UNSIGNED_BYTE,rgba.data());images.push_back({tx,iw,ih});}
  for(unsigned i=0;i<nq;i++){unsigned q[13];assert(in.read((char*)q,52));assert(q[7]<nt);images[q[7]].col=q[0];}
  std::set<unsigned> collections;std::map<std::pair<GLuint,unsigned>,std::vector<Vertex>> batches;unsigned covered=0;
  for(auto&i:images)if(i.col!=~0u){collections.insert(i.col);sprite_atlas::registerTexture(i.tx,i.w,i.h,i.col);auto r=sprite_atlas::resolve(i.tx,false);if(r.atlas)covered++;batches[{i.tx,1}]=quad(-1,1);}
  SpriteStream stream;stream.upload(batches);printf("Willow Creek pack collections=%zu images=%zu atlas_images=%u ranges_before=%zu ranges_after=%zu\n",collections.size(),batches.size(),covered,batches.size(),stream.ranges.size());sprite_atlas::report();assert(covered==batches.size());assert(stream.ranges.size()<batches.size()/2);assert(glGetError()==GL_NO_ERROR);sprite_atlas::clear();for(auto&i:images)glDeleteTextures(1,&i.tx);
 }
 puts("PASS actual sprite/particle streams: batching, 4 shader combinations, HD edges, normal crop/flip, reflection, alpha order, gutters, palette refresh, reuse, page rollover, fallback and level release");
}
