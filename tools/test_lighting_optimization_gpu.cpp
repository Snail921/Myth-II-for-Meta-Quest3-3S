#include "../src/vr/main.cpp"
#define mythVertexShader baselineVS
#define mythFragmentShader baselineFS
#include "fixtures/lighting_before_culling.h"
#undef mythVertexShader
#undef mythFragmentShader
#include <cassert>
int main(){
 EGLDisplay display=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(display,0,0));EGLint configAttrib[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};EGLConfig cfg;EGLint n;assert(eglChooseConfig(display,configAttrib,&cfg,1,&n)&&n);EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE},pa[]={EGL_WIDTH,512,EGL_HEIGHT,512,EGL_NONE};auto ctx=eglCreateContext(display,cfg,EGL_NO_CONTEXT,ca);auto surface=eglCreatePbufferSurface(display,cfg,pa);assert(eglMakeCurrent(display,surface,surface,ctx));glGenVertexArrays(1,&vao);glBindVertexArray(vao);
 auto link=[&](const char*vs,const char*fs){auto p=glCreateProgram();glAttachShader(p,shader(GL_VERTEX_SHADER,vs));glAttachShader(p,shader(GL_FRAGMENT_SHADER,fs));glLinkProgram(p);GLint ok;glGetProgramiv(p,GL_LINK_STATUS,&ok);assert(ok);return p;};GLuint old=link(baselineVS,baselineFS),optimized=link(mythVertexShader,mythFragmentShader);
 GLuint tex;glGenTextures(1,&tex);glBindTexture(GL_TEXTURE_2D,tex);unsigned char gray[]={120,120,120,255};glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,gray);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
 std::map<std::pair<GLuint,unsigned>,std::vector<Vertex>> batches;
 Vertex v[]={{-.9f,.9f,0,0,0},{.9f,.9f,0,1,0},{-.9f,-.9f,0,0,1},{.9f,.9f,0,1,0},{.9f,-.9f,0,1,1},{-.9f,-.9f,0,0,1}};batches[{tex,1}]={v,v+6};SpriteStream stream;stream.upload(batches);assert(stream.lightBounds.bounds.size()==6);for(auto b:stream.lightBounds.bounds)assert(b[3]>1.27f);
 float positions[512]{},colors[512]{},axes[512]{};
 auto setup=[&](GLuint p,int count,bool reflected){program=p;glUseProgram(p);auto loc=[&](const char*s){return glGetUniformLocation(p,s);};Mat m{};m[0]=m[5]=m[10]=m[15]=1;glUniformMatrix4fv(loc("mvp"),1,GL_FALSE,m.data());glUniform1i(loc("tex"),0);glUniform1i(loc("detailTex"),1);glUniform4f(loc("tint"),1,1,1,1);glUniform3f(loc("fogEye"),0,0,2);glUniform1f(loc("reflectionClip"),reflected?1:0);glUniform1f(loc("reflectionHeight"),-2);glUniform1i(loc("spriteLightCount"),count);glUniform4fv(loc("spriteLights"),count,positions);glUniform4fv(loc("spriteLightColors"),count,colors);glUniform4fv(loc("spriteLightAxes"),count,axes);glViewport(0,0,512,512);};
 auto read=[&](){std::vector<unsigned char> pixels(512*512*4);glReadPixels(0,0,512,512,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());assert(glGetError()==GL_NO_ERROR);return pixels;};
 for(int c=0;c<6;c++){
  for(int i=0;i<128;i++){positions[i*4]=50+i;positions[i*4+1]=0;positions[i*4+2]=1;positions[i*4+3]=1;colors[i*4]=.3;colors[i*4+1]=.1;colors[i*4+2]=.7;colors[i*4+3]=.8;}
  // Exercise masks across all 32-bit boundaries and sphere/capsule edge hits.
  for(int i:{0,31,32,63,64,95,96,127}){positions[i*4]=c==0?50.f:(c==1?.9f:(c==2?2.f:-3.f));positions[i*4+2]=c==2?0.f:.5f;positions[i*4+3]=c==2?1.f:2.f;if(c>=3){axes[i*4]=6;axes[i*4+3]=1.f/36;}}
  setup(old,128,c==5);glClear(GL_COLOR_BUFFER_BIT);stream.draw();auto reference=read();setup(optimized,128,c==5);glClear(GL_COLOR_BUFFER_BIT);stream.draw();auto actual=read();int maxError=0;for(size_t i=0;i<actual.size();i++)maxError=std::max(maxError,abs(int(actual[i])-int(reference[i])));assert(maxError<=1);
  // Alpha stream has the same conservative bounds and lighting mask.
  ParticleStream ps;ps.append(tex,1,v,6,1);ps.upload();glClear(GL_COLOR_BUFFER_BIT);ps.draw(glGetUniformLocation(program,"tint"));auto particle=read();assert(particle==actual);
  printf("Equivalence case %d max channel error=%d (opaque/alpha/reflection)\n",c,maxError);
 }
 for(int i=0;i<128;i++){positions[i*4]=50+i;positions[i*4+2]=1;positions[i*4+3]=1;axes[i*4]=axes[i*4+3]=0;}positions[0]=.5f;positions[2]=.5f;
 for(int count:{8,128})for(GLuint p:{old,optimized}){setup(p,count,false);for(int i=0;i<10;i++)stream.draw();glFinish();double start=seconds();for(int i=0;i<80;i++){glClear(GL_COLOR_BUFFER_BIT);stream.draw();}glFinish();printf("512x512 lights=%d %s average_ms=%.3f\n",count,p==old?"baseline":"optimized",(seconds()-start)*1000/80);}
 program=optimized;glUseProgram(program);spriteLightBinding={};spriteLightSelections=spriteLightBindingReuses=0;liveLights.clear();liveParticles.clear();liveLightsValid=false;snapshotRevision++;enhancedLightingEnabled=true;enhancedLightingAmount=1;enhancedLightingLimit=8;
 for(int i=0;i<4;i++)bindSpriteLighting();assert(spriteLightSelections==1&&spriteLightBindingReuses==3);
 camera.x+=1;bindSpriteLighting();assert(spriteLightSelections==2);enhancedLightingLimit=32;bindSpriteLighting();assert(spriteLightSelections==3);enhancedLightingEnabled=false;bindSpriteLighting();bindSpriteLighting();assert(spriteLightSelections==4&&spriteLightBindingReuses==4);snapshotRevision++;bindSpriteLighting();assert(spriteLightSelections==5);assert(glGetError()==GL_NO_ERROR);
 printf("PASS stereo/reflection binding reuse; camera/settings/snapshot invalidation\n");
}
