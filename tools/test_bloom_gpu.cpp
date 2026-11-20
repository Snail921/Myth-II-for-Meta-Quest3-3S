#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <cassert>
#include <cmath>
#include <chrono>
#include <vector>
#include <cstdio>
#include "../src/vr/bloom.h"
static GLuint fbo=0,tex=0,rb=0;
static void makeTarget(int w,int h,GLenum fmt,int samples){
 glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);
 if(samples>1){glGenRenderbuffers(1,&rb);glBindRenderbuffer(GL_RENDERBUFFER,rb);glRenderbufferStorageMultisample(GL_RENDERBUFFER,samples,fmt,w,h);glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_RENDERBUFFER,rb);}
 else{glGenTextures(1,&tex);glBindTexture(GL_TEXTURE_2D,tex);glTexImage2D(GL_TEXTURE_2D,0,fmt,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,tex,0);}
 assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);glViewport(0,0,w,h);
}
static void drop(){glDeleteFramebuffers(1,&fbo);glDeleteTextures(1,&tex);glDeleteRenderbuffers(1,&rb);fbo=tex=rb=0;}
static void paint(bool sky,int size){glDisable(GL_SCISSOR_TEST);glColorMask(1,1,1,1);glClearColor(0,0,0,0);glClear(GL_COLOR_BUFFER_BIT);glEnable(GL_SCISSOR_TEST);glScissor(size/2-16,size/2-16,32,32);glClearColor(1,1,1,sky?0:1);glClear(GL_COLOR_BUFFER_BIT);glDisable(GL_SCISSOR_TEST);}
static std::vector<unsigned char> read(int size,GLenum fmt,int samples){
 GLuint out=0,fb=0;if(samples>1){bloom::target(out,fb,size,size,fmt);glBindFramebuffer(GL_READ_FRAMEBUFFER,fbo);glBindFramebuffer(GL_DRAW_FRAMEBUFFER,fb);glBlitFramebuffer(0,0,size,size,0,0,size,size,GL_COLOR_BUFFER_BIT,GL_NEAREST);glBindFramebuffer(GL_FRAMEBUFFER,fb);}
 std::vector<unsigned char> v(size*size*4);glReadPixels(0,0,size,size,GL_RGBA,GL_UNSIGNED_BYTE,v.data());if(fb){glDeleteFramebuffers(1,&fb);glDeleteTextures(1,&out);}glBindFramebuffer(GL_FRAMEBUFFER,fbo);return v;
}
int main(){auto d=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(d,0,0));EGLint a[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};EGLConfig cfg;EGLint n;assert(eglChooseConfig(d,a,&cfg,1,&n)&&n);EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE},pa[]={EGL_WIDTH,16,EGL_HEIGHT,16,EGL_NONE};auto ctx=eglCreateContext(d,cfg,EGL_NO_CONTEXT,ca);auto surface=eglCreatePbufferSurface(d,cfg,pa);assert(eglMakeCurrent(d,surface,surface,ctx));glDisable(GL_DITHER);
 bloom::enabled=false;bloom::render();assert(!bloom::snapshot&&!bloom::extractProgram);assert(glGetError()==GL_NO_ERROR);
 for(GLenum fmt:{GLenum(GL_RGBA8),GLenum(GL_SRGB8_ALPHA8)})for(int samples:{1,2})for(int resolution=0;resolution<6;resolution++)for(int iterations:{1,4}){
  const int size=256;makeTarget(size,size,fmt,samples);bloom::enabled=true;bloom::resolution=resolution;bloom::iterations=iterations;bloom::strength=.15f;
  for(bool sky:{true,false}){
   paint(sky,size);
   // Adversarial caller state must survive postprocessing exactly.
   glEnable(GL_SCISSOR_TEST);glScissor(1,2,3,4);glEnable(GL_DEPTH_TEST);glEnable(GL_CULL_FACE);glEnable(GL_BLEND);glDepthMask(GL_TRUE);glBlendFuncSeparate(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA,GL_ZERO,GL_ONE);glColorMask(GL_FALSE,GL_TRUE,GL_FALSE,GL_TRUE);glActiveTexture(GL_TEXTURE3);
   bloom::render();assert(glGetError()==GL_NO_ERROR);GLint v;glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&v);assert(v==int(fbo));glGetIntegerv(GL_ACTIVE_TEXTURE,&v);assert(v==GL_TEXTURE3);assert(glIsEnabled(GL_SCISSOR_TEST)&&glIsEnabled(GL_DEPTH_TEST)&&glIsEnabled(GL_CULL_FACE)&&glIsEnabled(GL_BLEND));GLboolean mask[4];glGetBooleanv(GL_COLOR_WRITEMASK,mask);assert(!mask[0]&&mask[1]&&!mask[2]&&mask[3]);glGetIntegerv(GL_BLEND_SRC_ALPHA,&v);assert(v==GL_ZERO);
   glDisable(GL_SCISSOR_TEST);auto pixels=read(size,fmt,samples);int halo=0;for(int y=0;y<size;y++)for(int x=0;x<size;x++){int i=(y*size+x)*4;assert(pixels[i+3]==255);if(x<size/2-16||x>=size/2+16||y<size/2-16||y>=size/2+16)halo+=pixels[i];}
   if(sky)assert(halo==0);else assert(halo>0);
   glDisable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);glDisable(GL_BLEND);glColorMask(1,1,1,1);glActiveTexture(GL_TEXTURE0);
  }
  drop();printf("PASS format=%x samples=%d divisor=%d iterations=%d sky/no-sky halo + state\n",fmt,samples,bloom::divisors[resolution],iterations);
 }
 // Reuse across eyes must not retain the previous eye glow.
 makeTarget(256,256,GL_SRGB8_ALPHA8,1);paint(false,256);bloom::render();glDisable(GL_SCISSOR_TEST);glClearColor(0,0,0,0);glClear(GL_COLOR_BUFFER_BIT);bloom::render();auto black=read(256,GL_SRGB8_ALPHA8,1);for(size_t i=0;i<black.size();i+=4)assert(black[i]==0&&black[i+1]==0&&black[i+2]==0);drop();bloom::release();
 // Synthetic 1536-square benchmark: includes snapshot, extraction, blur and composite.
 // glFinish isolates submission+GPU latency; this is NOT an in-game frame-time claim.
 makeTarget(1536,1536,GL_SRGB8_ALPHA8,1);for(int r:{0,2,3})for(int iterations:{1,4}){bloom::resolution=r;bloom::iterations=iterations;paint(false,1536);bloom::render();glFinish();auto start=std::chrono::steady_clock::now();for(int i=0;i<12;i++){paint(false,1536);bloom::render();glFinish();}double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()/12;printf("SYNTHETIC 1536x1536 divisor=%d iterations=%d total_ms=%.3f\n",bloom::divisors[r],iterations,ms);}
 drop();bloom::release();bloom::enabled=false;bloom::render();assert(!bloom::snapshot&&!bloom::extractProgram);assert(glGetError()==GL_NO_ERROR);puts("PASS Bloom all tests");
}
