#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <chrono>
#include <cassert>
#include <unistd.h>
#define LOG(...) do {printf(__VA_ARGS__);puts("");}while(0)
static bool diagnosticEnabled=true;
static double seconds(){return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();}
#include "../src/vr/render_profile.h"
int main(){auto d=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(d,0,0));EGLint a[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_NONE};EGLConfig cfg;EGLint n;assert(eglChooseConfig(d,a,&cfg,1,&n)&&n);EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE},pa[]={EGL_WIDTH,16,EGL_HEIGHT,16,EGL_NONE};
 for(int pass=0;pass<2;pass++){auto ctx=eglCreateContext(d,cfg,EGL_NO_CONTEXT,ca);auto surf=eglCreatePbufferSurface(d,cfg,pa);assert(eglMakeCurrent(d,surf,surf,ctx));stageProfile={};
 for(int j=0;j<40;j++){for(int k=0;k<16;k++){stageProfile.begin(k);glClear(GL_COLOR_BUFFER_BIT);stageProfile.end();}glFlush();usleep(2000);gpuProfile.poll();}
 for(int k=0;k<16;k++)assert(stageProfile.calls[k]==40);assert(gpuProfile.active==-1);assert(glGetError()==GL_NO_ERROR);stageProfile.report(40);gpuProfile.report();gpuProfile.release();assert(!gpuProfile.initialized);assert(glGetError()==GL_NO_ERROR);
 eglMakeCurrent(d,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);eglDestroySurface(d,surf);eglDestroyContext(d,ctx);}
 spriteDetail={};for(int k=0;k<2;k++)for(int i=0;i<34;i++){SpriteDetailScope scope(spriteDetail.sample(k),k,0);scope.next(2);scope.next(4);}for(int k=0;k<2;k++){assert(spriteDetail.passes[k]==2);assert(spriteDetail.ops[k][0]==2&&spriteDetail.ops[k][2]==2&&spriteDetail.ops[k][4]==2);}spriteDetail.report();assert(spriteDetail.passes[0]==0);diagnosticEnabled=false;assert(!spriteDetail.sample(0));stageProfile.begin(0);assert(stageProfile.active==-1);stageProfile.end();puts("PASS stages, non-nested GPU queries, two context lifetimes, diagnostics OFF");}
