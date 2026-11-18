#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <vector>
#include <map>
#include <stdexcept>
#include <cstring>
#include <cstdio>
#include <cassert>
#include <fstream>
#include <iterator>
#define LOG(...) do {} while(0)
static unsigned u32(const unsigned char*p){unsigned v;memcpy(&v,p,4);return v;}
struct ModelPose{unsigned geometry;int32_t pos[3];unsigned owner=65535,permutation=0;};
static std::vector<ModelPose> modelPoses;static bool modelPosesValid=true;
namespace hd {static std::map<GLuint,uint64_t> modelKeys;}
#include "../src/vr/model_appearance.h"
int main(int argc,char**argv){assert(argc==2);EGLDisplay d=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(d,0,0));EGLint a[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_NONE};EGLConfig cfg;EGLint n;assert(eglChooseConfig(d,a,&cfg,1,&n)&&n);EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE},pa[]={EGL_WIDTH,16,EGL_HEIGHT,16,EGL_NONE};auto c=eglCreateContext(d,cfg,EGL_NO_CONTEXT,ca);auto surf=eglCreatePbufferSurface(d,cfg,pa);assert(eglMakeCurrent(d,surf,surf,c));
std::ifstream f(argv[1],std::ios::binary);std::vector<unsigned char>b((std::istreambuf_iterator<char>(f)),{});assert(!memcmp(b.data(),"MYTHMD02",8));unsigned nt=u32(b.data()+8),nc=u32(b.data()+12);size_t off=16;std::vector<AppearancePixels>pixels;std::vector<GLuint>tx(nt);
for(unsigned i=0;i<nt;i++){unsigned w=u32(b.data()+off),h=u32(b.data()+off+4);off+=8;pixels.push_back({w,h,{b.begin()+off,b.begin()+off+w*h*4}});off+=w*h*4;}
auto overrides=loadModelAppearances(b,off+nc*64,nc,tx,pixels);unsigned knots=0;for(auto&a:modelAppearances)if(a.geometry==0x776f6977){ModelPose p{};p.geometry=a.geometry;memcpy(p.pos,a.pos,12);p.permutation=7;modelPoses.push_back(p);knots++;}assert(knots==4);updateModelAppearances();GLuint fb;glGenFramebuffers(1,&fb);glBindFramebuffer(GL_FRAMEBUFFER,fb);unsigned changed=0;
for(auto&a:modelAppearances)if(a.geometry==0x776f6977){assert(a.current==7);for(size_t i=0;i<a.targets.size();i++){auto&p=appearancePixels.at(a.variants[7][i]);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,a.targets[i],0);assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);std::vector<unsigned char>read(p.rgba.size());glReadPixels(0,0,p.w,p.h,GL_RGBA,GL_UNSIGNED_BYTE,read.data());assert(read==p.rgba);changed+=a.variants[0][i]!=a.variants[7][i];}}
assert(changed>=4);for(auto&p:modelPoses)p.permutation=0;updateModelAppearances();for(auto&a:modelAppearances)if(a.geometry==0x776f6977)assert(a.current==0);assert(glGetError()==0);printf("PASS four knots: 0->7->0, %u changed materials, GPU image readback exact; %zu triangle bindings\n",changed,overrides.size());}
