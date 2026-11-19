#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <cstdio>
#include <cassert>
#define LOG(...) do {printf(__VA_ARGS__);puts("");}while(0)
static bool enhancedSpritePath=true;
static void pumpLoadingEvents(){}
struct App {bool destroyRequested=false;};static App*loadingApp=nullptr;
#include "../src/vr/sprite_material.h"
int main(int argc,char**argv){assert(argc==2);std::string root=argv[1];
 std::ifstream vectors(root+"/hash-vectors.bin",std::ios::binary);unsigned w,h,n=0;
 while(vectors.read((char*)&w,4)){vectors.read((char*)&h,4);std::vector<unsigned char> p(w*h*4);material_hash::Digest expected;assert(vectors.read((char*)p.data(),p.size()));assert(vectors.read((char*)expected.data(),32));assert(material_hash::image(w,h,p.data())==expected);n++;}assert(n==10);
 EGLDisplay d=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(d,0,0));EGLint a[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_NONE};EGLConfig cfg;EGLint count;assert(eglChooseConfig(d,a,&cfg,1,&count)&&count);EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE},pa[]={EGL_WIDTH,16,EGL_HEIGHT,16,EGL_NONE};auto c=eglCreateContext(d,cfg,EGL_NO_CONTEXT,ca);auto surf=eglCreatePbufferSurface(d,cfg,pa);assert(eglMakeCurrent(d,surf,surf,c));
 sprite_material::load(root.c_str());assert(sprite_material::entries.size()==2&&sprite_material::hashIndex.size()==1&&sprite_material::normalGpuBytes==0);
 unsigned char pixels[80]={0};for(int i=64;i<80;i+=4){pixels[i]=11;pixels[i+1]=22;pixels[i+2]=33;pixels[i+3]=255;}
 auto first=sprite_material::match(20,1,pixels);assert(first.texture&&sprite_material::normalGpuBytes==80&&sprite_material::timing.legacyReads==0);
 assert(sprite_material::match(20,1,pixels).texture==first.texture&&sprite_material::normalGpuBytes==80);
 pixels[79]^=1;assert(!sprite_material::match(20,1,pixels).texture); // Same first64, different image.
 GLuint fbo;glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,first.texture,0);assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);unsigned char color[4];glReadPixels(0,0,1,1,GL_RGBA,GL_UNSIGNED_BYTE,color);assert(color[0]==130&&color[1]==140&&color[2]==250&&color[3]==255);glDeleteFramebuffers(1,&fbo);
 unsigned char old[]={1,2,3,255};assert(sprite_material::match(1,1,old).texture);assert(sprite_material::timing.legacyReads==1);
 sprite_material::load(root.c_str());assert(!glIsTexture(first.texture)&&sprite_material::normalGpuBytes==0);
 enhancedSpritePath=false;sprite_material::load(root.c_str());assert(sprite_material::hashIndex.empty()&&sprite_material::entries.empty());assert(glGetError()==GL_NO_ERROR);
 puts("PASS Python SHA256 vectors, hash lookup, same-prefix mismatch, exact GPU pixels, lazy upload/reuse, legacy compatibility, level release and OFF bypass");
}
