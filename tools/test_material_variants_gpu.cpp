#include "../src/vr/main.cpp"
#include <cassert>
int main(int argc,char**argv){
 assert(argc==2);setbuf(stdout,nullptr);
 EGLDisplay d=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(d,0,0));
 EGLint attrs[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};
 EGLConfig cfg;EGLint n;assert(eglChooseConfig(d,attrs,&cfg,1,&n)&&n);
 EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE},pa[]={EGL_WIDTH,2,EGL_HEIGHT,2,EGL_NONE};
 auto ctx=eglCreateContext(d,cfg,EGL_NO_CONTEXT,ca);auto surface=eglCreatePbufferSurface(d,cfg,pa);assert(eglMakeCurrent(d,surface,surface,ctx));
 enhancedSpritePath=true;
 for(bool enhanced:{false,true})for(bool unified:{false,true})for(bool pbr:{false,true}){
  enhancedShadowsEnabled=enhanced;unifiedShadowsEnabled=unified;sprite_pbr::meshes=pbr;
  sprite_material::load(argv[1]);
  for(int index=0;index<2;index++){
   std::vector<unsigned char> source;for(int j=0;j<4;j++)for(int v:{40+index*10,60+index*10,80+index*10,255})source.push_back(v);
   auto binding=sprite_material::match(2,2,source.data());assert(binding.texture);assert(binding.packed==bool(index));
   assert(bool(binding.albedoEntry)==(enhanced&&unified));
   auto pixels=source;bool applied=sprite_material::applyAlbedo(binding,2,2,pixels);assert(applied==(enhanced&&unified));
   assert(pixels[0]==40+index*10+(applied?40:0)&&pixels[3]==255);
   auto hash=material_hash::image(2,2,source.data());assert(sprite_material::matchIdentity(2,2,hash.data()).texture==binding.texture);
   GLuint tx,fb;glGenTextures(1,&tx);glBindTexture(GL_TEXTURE_2D,tx);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,2,2,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
   glGenFramebuffers(1,&fb);glBindFramebuffer(GL_FRAMEBUFFER,fb);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,tx,0);assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);
   unsigned char readback[16];glReadPixels(0,0,2,2,GL_RGBA,GL_UNSIGNED_BYTE,readback);assert(!memcmp(readback,pixels.data(),16));
   glDeleteFramebuffers(1,&fb);glDeleteTextures(1,&tx);
  }
  std::vector<unsigned char> missing(16,127);auto b=sprite_material::match(2,2,missing.data());assert(!b.texture&&!b.albedoEntry);auto before=missing;assert(!sprite_material::applyAlbedo(b,2,2,missing)&&before==missing);
  assert(glGetError()==GL_NO_ERROR);printf("PASS enhanced=%d unified=%d PBR=%d, both material flags, GPU pixels, missing fallback\n",enhanced,unified,pbr);
 }
 // Exercise the final mesh selector: a ready HD colour image must not replace
 // an edited standard image merely because it can inherit its PBR binding.
 enhancedShadowsEnabled=true;unifiedShadowsEnabled=true;
 GLuint sourceTx=501,hdTx=502;uint64_t id=(uint64_t(1)<<32)|6;
 sprite_material::Binding edited{};edited.texture=503;edited.albedoEntry=1;
 sprite_material::assign(sourceTx,edited);hd::modelKeys[sourceTx]=id;
 hd::entries[id]={};hd::cache[id]={hdTx,16,0};hd::materials.erase(id);
 assert(hdModel(sourceTx)==sourceTx);
 auto plain=edited;plain.albedoEntry=0;hd::materials[id]=plain;
 assert(hdModel(sourceTx)==sourceTx);
 hd::materials[id]=edited;assert(hdModel(sourceTx)==hdTx);
 hd::materials.erase(id);unifiedShadowsEnabled=false;assert(hdModel(sourceTx)==hdTx);
 unifiedShadowsEnabled=true;sprite_material::assign(sourceTx,plain);assert(hdModel(sourceTx)==hdTx);
 hd::cache.clear();hd::entries.clear();hd::materials.clear();hd::modelKeys.clear();
 puts("PASS final mesh albedo selection: edited HD > edited standard > original HD; unified OFF retains HD");
 sprite_material::clear();puts("PASS material variants");
}
