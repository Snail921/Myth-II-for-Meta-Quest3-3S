// Actual production reflection shaders, target allocation and compositing on Quest GLES.
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <vector>
#include <map>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <chrono>
#include <unistd.h>
#define LOG(...) do{printf(__VA_ARGS__);puts("");}while(0)
using Mat=std::array<float,16>;
struct Vertex{float x,y,z,u,v;};
struct Activity{const char*internalDataPath;};struct android_app{Activity*activity;};
static const char* waterFile="/data/local/tmp/reflection-water.pack";
static std::vector<unsigned char> asset(android_app*,const char*){FILE*f=fopen(waterFile,"rb");assert(f);fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);std::vector<unsigned char>b(n);assert(fread(b.data(),1,n,f)==(size_t)n);fclose(f);return b;}
static unsigned u32(const void*p){unsigned n;memcpy(&n,p,4);return n;}
static float mythWorldX(float x,float origin,float scale){return (origin-x)*scale;}
static unsigned levelWidth=224,levelHeight=192;
static const float originX=187.2f,originY=156.3f,originZ=-1.82f,scale=1;
static double seconds(){return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();}
static GLuint program;
static GLuint shader(GLenum kind,const char*src){GLuint s=glCreateShader(kind);glShaderSource(s,1,&src,nullptr);glCompileShader(s);GLint ok;glGetShaderiv(s,GL_COMPILE_STATUS,&ok);if(!ok){char log[2048];glGetShaderInfoLog(s,2048,nullptr,log);puts(log);}assert(ok);return s;}
static void attributes(GLuint b){glBindBuffer(GL_ARRAY_BUFFER,b);glEnableVertexAttribArray(0);glEnableVertexAttribArray(1);glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),0);glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)12);}
#include "../src/vr/color_pipeline.h"
#include "../src/vr/render_culling.h"
#include "../src/vr/reflection_render.h"
static bool diagnosticEnabled=true;
#include "../src/vr/render_profile.h"
#include "../src/vr/sprite_stream.h"
#include "../src/vr/particle_stream.h"
#include "../src/vr/scene_fade.h"
int main(int argc,char**argv){
 EGLDisplay display=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(display,0,0));EGLint aa[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_DEPTH_SIZE,24,EGL_NONE};EGLConfig cfg;EGLint count;assert(eglChooseConfig(display,aa,&cfg,1,&count)&&count);
 EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE},pa[]={EGL_WIDTH,64,EGL_HEIGHT,64,EGL_NONE};auto context=eglCreateContext(display,cfg,EGL_NO_CONTEXT,ca);auto surface=eglCreatePbufferSurface(display,cfg,pa);assert(eglMakeCurrent(display,surface,surface,context));
 GLuint vao,vbo;glGenVertexArrays(1,&vao);glBindVertexArray(vao);glGenBuffers(1,&vbo);
 program=glCreateProgram();glAttachShader(program,shader(GL_VERTEX_SHADER,mythVertexShader));glAttachShader(program,shader(GL_FRAGMENT_SHADER,mythFragmentShader));glLinkProgram(program);GLint ok;glGetProgramiv(program,GL_LINK_STATUS,&ok);assert(ok);
 if(argc==2&&!strcmp(argv[1],"--export-normal")){
  initWaterNormal();GLuint exportFBO;glGenFramebuffers(1,&exportFBO);glBindFramebuffer(GL_FRAMEBUFFER,exportFBO);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,waterNormalTexture,0);assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);
  std::vector<unsigned char> pixels(128*128*4);glReadPixels(0,0,128,128,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());assert(glGetError()==GL_NO_ERROR);FILE*f=fopen("/data/local/tmp/water-normal.rgba","wb");assert(f);assert(fwrite(pixels.data(),1,pixels.size(),f)==pixels.size());fclose(f);puts("Exported production RG8 normal texture: 128x128 RGBA readback, first row is texture v=0");return 0;
 }
 Activity act{"/data/local/tmp/myth-reflection-test-settings"};android_app app{&act};loadReflections(&app);assert(reflectionWater.size()==1764*3);
 snprintf(reflectionSettingPath,sizeof(reflectionSettingPath),"/data/local/tmp/myth-reflection-setting-test");
 for(char setting: {'0','1'}){FILE*f=fopen(reflectionSettingPath,"wb");assert(f);fputc(setting,f);fclose(f);reflectionPoll=0;pollReflectionSetting();assert(reflectionEnabled==(setting=='1'));}
 for(int flags=0;flags<8;flags++){FILE*f=fopen(reflectionSettingPath,"wb");assert(f);char settings[]={(flags&1)?'1':'0',(flags&2)?'1':'0',(flags&4)?'1':'0'};fwrite(settings,1,3,f);fclose(f);reflectionPoll=0;pollReflectionSetting();assert(reflectionEnabled==bool(flags&1)&&reflectionFull==bool(flags&2)&&reflectionHigh==bool(flags&4));}
 {FILE*f=fopen(reflectionSettingPath,"wb");fputs("10x",f);fclose(f);reflectionPoll=0;pollReflectionSetting();assert(reflectionEnabled&&reflectionFull&&reflectionHigh);}
 unlink(reflectionSettingPath);reflectionEnabled=true;reflectionFull=reflectionHigh=false;
 Mat identity{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};auto mirror=reflectionMatrix();float y=reflectionLevel+2;float reflected=mirror[5]*y+mirror[13];assert(fabsf(reflected-(reflectionLevel-2))<1e-5f);assert(fabsf(mirror[5]*reflected+mirror[13]-y)<1e-5f);
 reflectionBounds={{{-.5f,-.5f,0},{.5f,.5f,0}}};assert(visibleReflectionWater(identity));Mat away=identity;away[12]=4;assert(!visibleReflectionWater(away));
 assert(prepareReflectionTarget(256,256)&&reflectionW==64&&reflectionH==64);GLuint saved=reflectionTexture;assert(prepareReflectionTarget(256,256)&&saved==reflectionTexture);
 glViewport(0,0,64,64);glClearColor(0,0,0,0);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);glDisable(GL_DEPTH_TEST);glDisable(GL_BLEND);
 glUseProgram(program);glUniformMatrix4fv(glGetUniformLocation(program,"mvp"),1,GL_FALSE,identity.data());glUniform4f(glGetUniformLocation(program,"tint"),1,1,1,1);glUniform1f(glGetUniformLocation(program,"alphaCutoff"),.5f);glUniform1f(glGetUniformLocation(program,"reflectionClip"),1);glUniform1f(glGetUniformLocation(program,"reflectionHeight"),0);
 GLuint tx;glGenTextures(1,&tx);glBindTexture(GL_TEXTURE_2D,tx);unsigned char red[]={255,0,0,255};glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,red);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
 Vertex all[]={{-1,-1,0,0,0},{1,-1,0,1,0},{-1,1,0,0,1},{1,-1,0,1,0},{1,1,0,1,1},{-1,1,0,0,1}};attributes(vbo);glBufferData(GL_ARRAY_BUFFER,sizeof(all),all,GL_STATIC_DRAW);glDrawArrays(GL_TRIANGLES,0,6);
 glBindFramebuffer(GL_FRAMEBUFFER,0);glViewport(0,0,64,64);glClearColor(0,0,1,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);reflectionReady=true;
 Vertex left[]={{-1,-1,0,0,0},{0,-1,0,1,0},{-1,1,0,0,1},{0,-1,0,1,0},{0,1,0,1,1},{-1,1,0,0,1}};reflectionWater.resize(6);glBindBuffer(GL_ARRAY_BUFFER,reflectionWaterVBO);glBufferData(GL_ARRAY_BUFFER,sizeof(left),left,GL_STATIC_DRAW);compositeReflections(identity);
 unsigned char pixel[4];glReadPixels(8,48,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);assert(abs(pixel[0]-97)<3&&abs(pixel[2]-158)<3);glReadPixels(8,8,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);assert(pixel[0]==0&&pixel[2]==255);glReadPixels(48,48,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);assert(pixel[0]==0&&pixel[2]==255);
 // Stencil rejects fragments outside the water even inside the screen rectangle.
 assert(prepareReflectionTarget(256,256));glViewport(0,0,64,64);glClearColor(0,0,0,0);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
 maskReflectionWater(identity);glDisable(GL_DEPTH_TEST);glUniform1f(glGetUniformLocation(program,"reflectionClip"),0);glBindTexture(GL_TEXTURE_2D,tx);attributes(vbo);glDrawArrays(GL_TRIANGLES,0,6);
 glReadPixels(8,48,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);assert(pixel[0]==255);glReadPixels(48,48,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);assert(pixel[0]==0&&pixel[3]==0);glDisable(GL_STENCIL_TEST);glUniform1f(glGetUniformLocation(program,"reflectionClip"),1);
 puts("PASS stencil water mask rejects non-water fragments");
 // Two overlapping half-alpha FX over transparent reflection background.
 assert(prepareReflectionTarget(256,256));glViewport(0,0,64,64);glClearColor(0,0,0,0);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);glDisable(GL_DEPTH_TEST);glUseProgram(program);glUniform1f(glGetUniformLocation(program,"alphaCutoff"),.003f);
 red[3]=128;glBindTexture(GL_TEXTURE_2D,tx);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,red);attributes(vbo);glEnable(GL_BLEND);glBlendFuncSeparate(GL_ONE,GL_ONE_MINUS_SRC_ALPHA,GL_ONE,GL_ONE_MINUS_SRC_ALPHA);glDrawArrays(GL_TRIANGLES,0,6);glDrawArrays(GL_TRIANGLES,0,6);glDisable(GL_BLEND);
 glBindFramebuffer(GL_FRAMEBUFFER,0);glClearColor(0,0,1,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);compositeReflections(identity);glReadPixels(8,48,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);assert(abs(pixel[0]-73)<3&&abs(pixel[2]-182)<3);
 reflectionHigh=true;assert(prepareReflectionTarget(1872,2016)&&reflectionW==1872&&reflectionH==2016);reflectionHigh=false;assert(prepareReflectionTarget(256,256)&&reflectionW==64&&reflectionH==64);

 // Compare streamed texture batches with legacy submission, including ring wrap,
 // empty passes, capacity growth and a second eye after another buffer was bound.
 SpriteStream stream;std::map<GLuint,std::vector<Vertex>> batches;
 glBindFramebuffer(GL_FRAMEBUFFER,0);glViewport(0,0,64,64);glDisable(GL_DEPTH_TEST);glDisable(GL_BLEND);glUseProgram(program);
 glUniformMatrix4fv(glGetUniformLocation(program,"mvp"),1,GL_FALSE,identity.data());glUniform1f(glGetUniformLocation(program,"reflectionClip"),0);glUniform4f(glGetUniformLocation(program,"tint"),1,1,1,1);
 GLuint green;glGenTextures(1,&green);glBindTexture(GL_TEXTURE_2D,green);unsigned char gp[]={0,255,0,255};glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,gp);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
 std::vector<unsigned char> expected(64*64*4),actual(expected.size());
 for(int round=0;round<10;round++){
  batches.clear();if(round!=3){batches[tx]=std::vector<Vertex>(all,all+6);batches[green]=std::vector<Vertex>(left,left+6);if(round==6)for(int j=0;j<1000;j++)batches[green].insert(batches[green].end(),left,left+6);}
  glClearColor(0,0,1,1);glClear(GL_COLOR_BUFFER_BIT);attributes(vbo);
  for(auto&b:batches){glBindTexture(GL_TEXTURE_2D,b.first);glBufferData(GL_ARRAY_BUFFER,b.second.size()*sizeof(Vertex),b.second.data(),GL_STREAM_DRAW);glDrawArrays(GL_TRIANGLES,0,b.second.size());}
  glReadPixels(0,0,64,64,GL_RGBA,GL_UNSIGNED_BYTE,expected.data());stream.upload(batches);
  for(int eye=0;eye<2;eye++){attributes(vbo);glClear(GL_COLOR_BUFFER_BIT);stream.draw();glReadPixels(0,0,64,64,GL_RGBA,GL_UNSIGNED_BYTE,actual.data());assert(actual==expected);}
 }
 puts("PASS sprite stream pixel equivalence: two eyes, ring wrap, empty pass, growth and other VBO bindings");
 // Production ordered FX stream must match individual draws, including alpha and A/B/A ordering.
 ParticleStream fxStream;glBindFramebuffer(GL_FRAMEBUFFER,0);glViewport(0,0,64,64);glDisable(GL_DEPTH_TEST);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
 auto tint=glGetUniformLocation(program,"tint");
 for(int round=0;round<8;round++){
 fxStream.clear();glClearColor(0,0,0,0);glClear(GL_COLOR_BUFFER_BIT);
 if(round!=3)for(unsigned i=0;i<9;i++){GLuint texture=i%3==1?green:tx;float alpha=(i%4+1)*.2f;auto vertices=i%2?left:all;glBindTexture(GL_TEXTURE_2D,texture);glUniform4f(tint,1,1,1,alpha);attributes(vbo);glBufferData(GL_ARRAY_BUFFER,sizeof(all),vertices,GL_STREAM_DRAW);glDrawArrays(GL_TRIANGLES,0,6);fxStream.append(texture,alpha,vertices,6);}
 glReadPixels(0,0,64,64,GL_RGBA,GL_UNSIGNED_BYTE,expected.data());fxStream.upload();
 for(int eye=0;eye<2;eye++){attributes(vbo);glClear(GL_COLOR_BUFFER_BIT);fxStream.draw(tint);glReadPixels(0,0,64,64,GL_RGBA,GL_UNSIGNED_BYTE,actual.data());assert(actual==expected);}}
 glDisable(GL_BLEND);glUniform4f(tint,1,1,1,1);puts("PASS ordered FX stream: alpha/order pixel equivalence, empty pass, ring wrap and both eyes");
 // Final fullscreen fade affects both eyes equally, including clear/sky areas.
 initSceneFade();glBindFramebuffer(GL_FRAMEBUFFER,0);glViewport(0,0,64,64);
 for(float value:{0.f,.5f,1.f}){sceneFade=value;glClearColor(1,1,1,1);glClear(GL_COLOR_BUFFER_BIT);drawSceneFade();glReadPixels(32,32,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);assert(abs(int(pixel[0])-int(value*255))<=2);}
 sceneFadeStart=-1;updateSceneFade();assert(sceneFade<.01f);sceneFadeStart=seconds()-1;updateSceneFade();assert(sceneFade==1);
 puts("PASS fade black/midpoint/full pixels and startup timer");
 // Exercise real nonblocking elapsed queries for both pass types.
 gpuProfile.init();
 if(gpuProfile.supported){
  for(int kind=0;kind<2;kind++)for(int i=0;i<17;i++){gpuProfile.start(kind);glClear(GL_COLOR_BUFFER_BIT);gpuProfile.stop();glFlush();}
  for(int i=0;i<200&&(!gpuProfile.samples[0]||!gpuProfile.samples[1]);i++){usleep(1000);gpuProfile.poll();}
  assert(gpuProfile.samples[0]&&gpuProfile.samples[1]);gpuProfile.report();puts("PASS asynchronous GPU timers for normal and reflection passes");
 }else puts("GPU timer unavailable; CPU profiling remains usable");
 // Reload the second map and ensure each elevation retains its own target and state.
 waterFile="/data/local/tmp/reflection-salvation.pack";levelWidth=160;levelHeight=224;loadReflections(&app);assert(reflectionPlanes.size()==3);
 GLuint targets[3];float elevations[3];size_t vertices[3];
 for(unsigned i=0;i<3;i++){activateReflectionPlane(i);elevations[i]=reflectionLevel;vertices[i]=reflectionWater.size();assert(vertices[i]>0);assert(prepareReflectionTarget(256,256));targets[i]=reflectionTexture;reflectionReady=true;}
 assert(targets[0]!=targets[1]&&targets[0]!=targets[2]&&targets[1]!=targets[2]);
 for(unsigned i:{0u,2u,1u,0u}){activateReflectionPlane(i);assert(reflectionTexture==targets[i]&&reflectionLevel==elevations[i]&&reflectionWater.size()==vertices[i]&&reflectionReady);assert(prepareReflectionTarget(256,256)&&reflectionTexture==targets[i]);}
 puts("PASS Salvation three elevations: independent reusable targets, geometry and ready state");
 waterFile="/data/local/tmp/reflection-gonen.pack";levelWidth=256;levelHeight=192;loadReflections(&app);assert(reflectionPlanes.size()==4);
 size_t total=0;bool retainedSlope=false;for(unsigned i=0;i<reflectionPlanes.size();i++){activateReflectionPlane(i);total+=reflectionWater.size();for(auto&v:reflectionWater)if(fabsf(v.y-(reflectionLevel+.004f))>.001f)retainedSlope=true;assert(prepareReflectionTarget(128,128));}
 assert(total>0&&retainedSlope);puts("PASS sloping river: four reflection proxies with actual varying water geometry retained");
 waterFile="/data/local/tmp/reflection-cloudspine.pack";levelWidth=224;levelHeight=224;loadReflections(&app);assert(reflectionPlanes.size()==2);
 for(unsigned plane=0;plane<2;plane++){activateReflectionPlane(plane);size_t count=0;for(auto&b:reflectionBounds){assert(b.first==count&&b.count%3==0);count+=b.count;}assert(count==reflectionWater.size());
 for(float offset:{-2.f,-.5f,0.f,.5f,2.f}){Mat m{};m[0]=.02f;m[9]=.02f;m[6]=.001f;m[12]=offset;m[15]=1;auto exact=rendercull::projectedTriangles(m,reflectionWater.data(),reflectionWater.size());auto tiled=projectReflectionWater(m);assert(exact.valid()==tiled.valid());if(exact.valid())assert(fabsf(exact.l-tiled.l)<1e-5f&&fabsf(exact.r-tiled.r)<1e-5f&&fabsf(exact.b-tiled.b)<1e-5f&&fabsf(exact.t-tiled.t)<1e-5f);}}
 reportReflectionWater();puts("PASS Cloudspine two planes: tiled projection equals all-triangle reference across five views");

 // Test the supplied artist texture through the production upload path.
 {FILE*f=fopen("/data/local/tmp/water-normal.pack","rb");assert(f);fseek(f,0,SEEK_END);size_t length=ftell(f);rewind(f);std::vector<unsigned char> data(length);assert(fread(data.data(),1,length,f)==length);fclose(f);assert(!uploadWaterNormal(data.data(),15));assert(!uploadWaterNormal(data.data(),length-1));assert(uploadWaterNormal(data.data(),length));assert(u32(data.data()+8)==512);}
 // Surface shader: old appearance when disabled, animated distortion and safe alpha.
 assert(prepareReflectionTarget(256,256));
 glBindFramebuffer(GL_FRAMEBUFFER,0);glViewport(0,0,64,64);glDisable(GL_DEPTH_TEST);glDisable(GL_STENCIL_TEST);glDisable(GL_SCISSOR_TEST);
 reflectionReady=true;reflectionWater.resize(6);Vertex water[]={{-1,0,-1,4,0},{1,0,-1,4,0},{-1,0,1,4,0},{1,0,-1,4,0},{1,0,1,4,0},{-1,0,1,4,0}};
 attributes(reflectionWaterVBO);glBufferData(GL_ARRAY_BUFFER,sizeof(water),water,GL_STATIC_DRAW);
 Mat waterM{};waterM[0]=waterM[9]=waterM[6]=waterM[15]=1;
 std::vector<unsigned char> gradient(64*64*4);for(int y=0;y<64;y++)for(int x=0;x<64;x++){auto i=(y*64+x)*4;gradient[i]=(x%8<4)?255:0;gradient[i+1]=y*4;gradient[i+2]=80;gradient[i+3]=255;}
 glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,reflectionTexture);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,64,64,0,GL_RGBA,GL_UNSIGNED_BYTE,gradient.data());
 auto renderWater=[&](bool enabled,float time){waterWavesEnabled=enabled;waterFrameTime=time;glClearColor(0,0,0,0);glClear(GL_COLOR_BUFFER_BIT);compositeReflections(waterM);std::vector<unsigned char> image(64*64*4);glReadPixels(0,0,64,64,GL_RGBA,GL_UNSIGNED_BYTE,image.data());return image;};
 waterSkyTexture=0;waterEye[0]=0;waterEye[1]=2;waterEye[2]=2;
 auto off=renderWater(false,0);auto wave0=renderWater(true,0);auto wave1=renderWater(true,8);assert(off!=wave0&&wave0!=wave1);for(size_t i=3;i<off.size();i+=4)assert(off[i]==wave0[i]&&off[i]==wave1[i]);assert(renderWater(false,8)==off);
 std::array<float,12> currents{};glBindBuffer(GL_ARRAY_BUFFER,reflectionFlowVBO);glBufferData(GL_ARRAY_BUFFER,sizeof(currents),currents.data(),GL_STATIC_DRAW);
 auto standing=renderWater(true,1);for(int i=0;i<6;i++)currents[i*2]=1;glBindBuffer(GL_ARRAY_BUFFER,reflectionFlowVBO);glBufferData(GL_ARRAY_BUFFER,sizeof(currents),currents.data(),GL_STATIC_DRAW);assert(renderWater(true,1)!=standing);assert(renderWater(false,1)==off);puts("PASS directed current affects production shader; OFF unchanged");
 // A shoreline vertex field of zero must exactly suppress distortion.
 for(auto&v:water)v.u=0;attributes(reflectionWaterVBO);glBufferData(GL_ARRAY_BUFFER,sizeof(water),water,GL_STATIC_DRAW);assert(renderWater(true,8)==off);
 for(auto&v:water)v.u=4;attributes(reflectionWaterVBO);glBufferData(GL_ARRAY_BUFFER,sizeof(water),water,GL_STATIC_DRAW);
 GLuint sunTexture;glGenTextures(1,&sunTexture);glBindTexture(GL_TEXTURE_2D,sunTexture);unsigned char white[]={255,255,255,255};glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,white);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);waterSkyTexture=sunTexture;waterSun[0]=0;waterSun[1]=1;waterSun[2]=-1;
 auto lit=renderWater(true,8);unsigned brighter=0;for(size_t i=0;i<lit.size();i+=4){brighter+=lit[i]>wave1[i];assert(lit[i+3]==wave1[i+3]);}assert(brighter>0);waterSkyTexture=0;
 puts("PASS water waves: disabled equivalence, temporal animation, shoreline fallback, specular and unchanged alpha");
 assert(glGetError()==GL_NO_ERROR);puts("PASS 8 independent mode combinations, malformed settings rejected, half-alpha FX composition, native-resolution target and return to low resolution");puts("PASS original water mask loaded; mirror involution; frustum rejection; reusable target; source-plane clipping; linear alpha composite confined to mask; GL_NO_ERROR");
}
