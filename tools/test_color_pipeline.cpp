#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <initializer_list>
#include "../src/vr/color_pipeline.h"
static void require(bool v,const char*m){if(!v){std::fprintf(stderr,"FAIL %s EGL=%x GL=%x\n",m,eglGetError(),glGetError());std::exit(1);}}
static GLuint compile(GLenum type,const char*s){GLuint h=glCreateShader(type);glShaderSource(h,1,&s,nullptr);glCompileShader(h);GLint ok;glGetShaderiv(h,GL_COMPILE_STATUS,&ok);require(ok,"shader compile");return h;}
int main(){
 EGLDisplay d=eglGetDisplay(EGL_DEFAULT_DISPLAY);require(eglInitialize(d,nullptr,nullptr),"initialize");
 EGLint attrs[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_NONE};EGLConfig cfg;EGLint n;require(eglChooseConfig(d,attrs,&cfg,1,&n)&&n,"config");
 EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE},sa[]={EGL_WIDTH,256,EGL_HEIGHT,1,EGL_NONE};
 EGLContext c=eglCreateContext(d,cfg,EGL_NO_CONTEXT,ca);EGLSurface surface=eglCreatePbufferSurface(d,cfg,sa);require(eglMakeCurrent(d,surface,surface,c),"current");
 GLuint program=glCreateProgram();glAttachShader(program,compile(GL_VERTEX_SHADER,"#version 300 es\nflat out float vertexAlpha;out vec2 tc;out float worldHeight;out vec3 fogPosition;flat out vec3 materialIds;out vec3 materialWeights;void main(){vertexAlpha=1.0;materialIds=vec3(1,2,0);materialWeights=vec3(.25,.75,0);vec2 p=vec2(float((gl_VertexID<<1)&2),float(gl_VertexID&2));tc=p;worldHeight=1.0;fogPosition=vec3(0,0,10);gl_Position=vec4(p*2.0-1.0,0,1);}"));glAttachShader(program,compile(GL_FRAGMENT_SHADER,mythFragmentShader));glLinkProgram(program);GLint ok;glGetProgramiv(program,GL_LINK_STATUS,&ok);require(ok,"link");glUseProgram(program);glUniform1i(glGetUniformLocation(program,"tex"),0);
 glUniform1i(glGetUniformLocation(program,"detailTex"),1);GLuint detail;glGenTextures(1,&detail);glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D_ARRAY,detail);
 unsigned char detailPixels[]={128,128,128,255,64,192,255,255,192,64,128,255};glTexImage3D(GL_TEXTURE_2D_ARRAY,0,GL_RGBA8,1,1,3,0,GL_RGBA,GL_UNSIGNED_BYTE,detailPixels);glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MAG_FILTER,GL_NEAREST);glActiveTexture(GL_TEXTURE0);
 float scales[]={1,1,1,1,1,1};glUniform2fv(glGetUniformLocation(program,"detailScales"),3,scales);glUniform2f(glGetUniformLocation(program,"detailDimensions"),1,1);
 GLuint input,output,fbo,vao;glGenTextures(1,&input);glGenTextures(1,&output);glGenFramebuffers(1,&fbo);glGenVertexArrays(1,&vao);glBindVertexArray(vao);glBindFramebuffer(GL_FRAMEBUFFER,fbo);glViewport(0,0,256,1);glDisable(GL_DITHER);
 unsigned char pixels[1024],actual[1024];for(int i=0;i<256;i++){pixels[4*i]=i;pixels[4*i+1]=255-i;pixels[4*i+2]=(i*73)%256;pixels[4*i+3]=255;}
 glBindTexture(GL_TEXTURE_2D,input);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,256,1,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
 for(GLenum format:{GLenum(GL_SRGB8_ALPHA8),GLenum(GL_RGBA8)}){
  glBindTexture(GL_TEXTURE_2D,output);glTexImage2D(GL_TEXTURE_2D,0,format,256,1,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,output,0);require(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE,"framebuffer");glBindTexture(GL_TEXTURE_2D,input);
  for(int enabled:{0,1})for(float density:{0.f,.01536f,.069314718f,.1f,10.f})for(float alpha:{1.f,.35f}){
   glUniform1i(glGetUniformLocation(program,"detailEnabled"),enabled);
   glUniform4f(glGetUniformLocation(program,"fogColorDensity"),.8f,.8f,.8f,density);
   glUniform4f(glGetUniformLocation(program,"tint"),1,1,1,alpha);glDrawArrays(GL_TRIANGLES,0,3);glReadPixels(0,0,256,1,GL_RGBA,GL_UNSIGNED_BYTE,actual);require(glGetError()==GL_NO_ERROR,"draw/read");int maxError=0;
   for(int i=0;i<1024;i++){float fog=1-std::exp(-density*10);float base=pixels[i]/255.f;if(enabled&&i%4!=3)base*=1+(.25f*(detailPixels[4+i%4]/255.f*2-1)+.75f*(detailPixels[8+i%4]/255.f*2-1));float v=base+(.8f-base)*fog*fog;v=std::fmin(1.f,std::fmax(0.f,v));float expected=i%4==3?alpha:(format==GL_SRGB8_ALPHA8?v:(v<=.04045f?v/12.92f:std::pow((v+.055f)/1.055f,2.4f)));int error=std::abs(int(actual[i])-int(std::round(expected*255)));if(error>maxError)maxError=error;}
   std::printf("format=0x%x alpha=%.2f max_byte_error=%d\n",format,alpha,maxError);require(maxError<=1,"color/alpha round trip");
  }
 }
 std::puts("PASS actual GLES shader: all 256 values, RGB channels, sRGB and linear targets, alpha unchanged");
 eglMakeCurrent(d,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);eglDestroySurface(d,surface);eglDestroyContext(d,c);eglTerminate(d);
}
