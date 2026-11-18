#include "../src/vr/main.cpp"
#include <cassert>
#include <sys/stat.h>
int main(){
 EGLDisplay d=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(d,0,0));EGLint a[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_NONE};EGLConfig cfg;EGLint n;assert(eglChooseConfig(d,a,&cfg,1,&n)&&n);EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE},pa[]={EGL_WIDTH,16,EGL_HEIGHT,16,EGL_NONE};auto ctx=eglCreateContext(d,cfg,EGL_NO_CONTEXT,ca);auto surf=eglCreatePbufferSurface(d,cfg,pa);assert(eglMakeCurrent(d,surf,surf,ctx));
 const char*root="/data/local/tmp/lazy-normal-test";mkdir(root,0700);std::string folder=std::string(root)+"/SpriteMaterials";mkdir(folder.c_str(),0700);
 for(int pack=0;pack<3;pack++){std::ofstream f(folder+"/"+std::to_string(pack)+".mvrnm",std::ios::binary);f.write("MVRNORM1",8);auto word=[&](unsigned v){f.write((char*)&v,4);};word(pack?3:4);for(int j=0;j<3;j++){word(1024);word(1024);f.seekp(1024*1024*8-1,std::ios::cur);f.put(0);}if(!pack){word(1);word(1);unsigned char pixels[]={1,2,3,255,128,128,255,255};f.write((char*)pixels,8);}}
 sprite_material::load(root);assert(sprite_material::entries.size()==10);assert(sprite_material::totalBytes>64*1024*1024);assert(sprite_material::normalGpuBytes==0);for(auto&e:sprite_material::entries)assert(!e.texture);
 unsigned char source[]={1,2,3,255},wrong[]={1,2,4,255};auto binding=sprite_material::match(1,1,source);assert(binding.texture&&sprite_material::normalGpuBytes==4);assert(!sprite_material::match(1,1,wrong).texture);assert(sprite_material::match(1,1,source).texture==binding.texture);assert(glIsTexture(binding.texture));sprite_material::assign(123,binding);
 // A new level always discards all previous GPU normals and bindings.
 sprite_material::load(root);assert(!glIsTexture(binding.texture));assert(sprite_material::bindings.empty()&&sprite_material::normalGpuBytes==0);assert(sprite_material::entries.size()==10);
 enhancedSpritePath=false;sprite_material::load(root);assert(sprite_material::entries.empty()&&sprite_material::packs.empty());assert(!sprite_material::match(1,1,source).texture);
 for(bool enabled:{true,false}){auto vs=spriteShaderSource(mythVertexShader,enabled),fs=spriteShaderSource(mythFragmentShader,enabled);GLuint p=glCreateProgram();glAttachShader(p,shader(GL_VERTEX_SHADER,vs.c_str()));glAttachShader(p,shader(GL_FRAGMENT_SHADER,fs.c_str()));glLinkProgram(p);GLint ok;glGetProgramiv(p,GL_LINK_STATUS,&ok);if(!ok){char log[4096];glGetProgramInfoLog(p,4096,0,log);puts(log);}assert(ok);glDeleteProgram(p);}
 assert(glGetError()==GL_NO_ERROR);for(int i=0;i<3;i++)unlink((folder+"/"+std::to_string(i)+".mvrnm").c_str());rmdir(folder.c_str());rmdir(root);
 puts("PASS 72 MiB library: metadata only; 4 bytes selected GPU normal; exact match; next-level GPU/binding release; OFF skips index; full/bypass shader link");
}
