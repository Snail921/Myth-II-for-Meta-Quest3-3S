#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <cstdio>
#include <cassert>
#include <array>
#include "../src/vr/color_pipeline.h"
#include "test_sprite_material_pipeline.h"
#include "../src/vr/scene_correction.h"
GLuint compile(GLenum type,const char*s){GLuint x=glCreateShader(type);glShaderSource(x,1,&s,0);glCompileShader(x);GLint ok;glGetShaderiv(x,GL_COMPILE_STATUS,&ok);if(!ok){char b[8192];glGetShaderInfoLog(x,sizeof(b),0,b);puts(b);}assert(ok);return x;}
int main(){auto d=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(d,0,0));EGLint a[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};EGLConfig cfg;EGLint n;assert(eglChooseConfig(d,a,&cfg,1,&n)&&n);EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE},pa[]={EGL_WIDTH,16,EGL_HEIGHT,16,EGL_NONE};auto ctx=eglCreateContext(d,cfg,EGL_NO_CONTEXT,ca);auto surface=eglCreatePbufferSurface(d,cfg,pa);assert(eglMakeCurrent(d,surface,surface,ctx));
testMaterialPipeline();
for(bool correction:{false,true})for(bool pbr:{false,true})for(bool atlas:{false,true})for(bool shadows:{false,true})for(bool lighting:{false,true}){GLuint p=glCreateProgram();auto vs=spriteShaderSource(mythVertexShader,shadows,lighting,atlas,pbr),fs=scene_correction::fragment(spriteShaderSource(mythFragmentShader,shadows,lighting,atlas,pbr),correction);glAttachShader(p,compile(GL_VERTEX_SHADER,vs.c_str()));glAttachShader(p,compile(GL_FRAGMENT_SHADER,fs.c_str()));glLinkProgram(p);GLint ok;glGetProgramiv(p,GL_LINK_STATUS,&ok);assert(ok);glUseProgram(p);auto u=[&](const char*s){return glGetUniformLocation(p,s);};if(!shadows){assert(u("spriteStaticDepth")==-1);assert(u("spriteLiveDepth")==-1);assert(u("spriteLiveOwner")==-1);assert(u("spriteShadows")==-1);}if(!pbr){assert(u("spriteSkyMap")==-1);assert(u("spritePbrStrength")==-1);assert(u("spriteSunRadiance")==-1);}if(!lighting)assert(u("spriteLightCount")==-1);if(!shadows&&!lighting&&!pbr)assert(u("spriteNormalEnabled")==-1);printf("variant shadows=%d lighting=%d pbr=%d atlas=%d\n",shadows,lighting,pbr,atlas);GLint count;glGetProgramiv(p,GL_ACTIVE_UNIFORMS,&count);int unit=1;for(int i=0;i<count;i++){char name[128];GLint size;GLenum type;glGetActiveUniform(p,i,128,0,&size,&type,name);if(type==GL_SAMPLER_2D||type==GL_SAMPLER_2D_ARRAY)glUniform1i(u(name),strcmp(name,"tex")==0?0:unit++);}
float ident[]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};glUniformMatrix4fv(u("mvp"),1,GL_FALSE,ident);glUniform4f(u("tint"),1,1,1,1);glUniform1f(u("reflectionHeight"),-100);GLuint vao,vbo,tx;glGenVertexArrays(1,&vao);glBindVertexArray(vao);glVertexAttrib4f(9,0,0,0,1);glGenBuffers(1,&vbo);glBindBuffer(GL_ARRAY_BUFFER,vbo);float verts[]={-1,-1,0,0,0, 1,-1,0,1,0, -1,1,0,0,1, -1,1,0,0,1, 1,-1,0,1,0, 1,1,0,1,1};glBufferData(GL_ARRAY_BUFFER,sizeof(verts),verts,GL_STATIC_DRAW);glEnableVertexAttribArray(0);glVertexAttribPointer(0,3,GL_FLOAT,0,20,0);glEnableVertexAttribArray(1);glVertexAttribPointer(1,2,GL_FLOAT,0,20,(void*)12);glGenTextures(1,&tx);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,tx);unsigned char pixels[]={255,255,255,64,255,255,255,192};glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,2,1,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);glViewport(0,0,16,16);
for(int reflection=0;reflection<2;reflection++){glUniform1f(u("reflectionClip"),reflection);for(int mode=0;mode<3;mode++){glClearColor(0,0,0,0);glClear(GL_COLOR_BUFFER_BIT);glUniform1i(u("spriteCutout"),mode==0);glUniform1i(u("softOnly"),mode==2);glUniform1f(u("alphaCutoff"),mode==0?.5f:mode==1?.995f:.003f);glDrawArrays(GL_TRIANGLES,0,6);unsigned char lo[4],hi[4];glReadPixels(4,8,1,1,GL_RGBA,GL_UNSIGNED_BYTE,lo);glReadPixels(12,8,1,1,GL_RGBA,GL_UNSIGNED_BYTE,hi);assert(glGetError()==GL_NO_ERROR);if(mode==0){assert(lo[3]==0&&hi[3]==255&&hi[0]>250);}if(mode==1)assert(lo[3]==0&&hi[3]==0);if(mode==2)assert(lo[3]>=63&&lo[3]<=65&&hi[3]>=191&&hi[3]<=193);}}
// Same ordered overlapping geometry: three uniform-alpha draws versus one
// draw with a flat alpha attribute. Exercise normal and premultiplied reflection.
float all[90],alpha[72];for(int i=0;i<3;i++){memcpy(all+i*30,verts,sizeof(verts));for(int j=0;j<6;j++){for(int k=0;k<3;k++)alpha[(i*6+j)*4+k]=0;alpha[(i*6+j)*4+3]=(.2f+i*.27f);}}
glBindBuffer(GL_ARRAY_BUFFER,vbo);glBufferData(GL_ARRAY_BUFFER,sizeof(all),all,GL_STATIC_DRAW);
GLuint ab;glGenBuffers(1,&ab);glBindBuffer(GL_ARRAY_BUFFER,ab);glBufferData(GL_ARRAY_BUFFER,sizeof(alpha),alpha,GL_STATIC_DRAW);glVertexAttribPointer(9,4,GL_FLOAT,GL_FALSE,16,0);
glUniform1i(u("spriteCutout"),0);glUniform1i(u("softOnly"),0);glUniform1f(u("alphaCutoff"),.003f);glEnable(GL_BLEND);
for(int reflected=0;reflected<2;reflected++){
 glUniform1f(u("reflectionClip"),reflected);if(reflected)glBlendFuncSeparate(GL_ONE,GL_ONE_MINUS_SRC_ALPHA,GL_ONE,GL_ONE_MINUS_SRC_ALPHA);else glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
 unsigned char reference[16*16*4],batched[16*16*4];glClear(GL_COLOR_BUFFER_BIT);glDisableVertexAttribArray(9);glVertexAttrib4f(9,0,0,0,1);
 for(int i=0;i<3;i++){glUniform4f(u("tint"),1,1,1,alpha[i*24+3]);glDrawArrays(GL_TRIANGLES,i*6,6);}glReadPixels(0,0,16,16,GL_RGBA,GL_UNSIGNED_BYTE,reference);
 glClear(GL_COLOR_BUFFER_BIT);glUniform4f(u("tint"),1,1,1,1);glEnableVertexAttribArray(9);glDrawArrays(GL_TRIANGLES,0,18);glReadPixels(0,0,16,16,GL_RGBA,GL_UNSIGNED_BYTE,batched);assert(memcmp(reference,batched,sizeof(reference))==0);assert(glGetError()==GL_NO_ERROR);
}
glDisable(GL_BLEND);glDisableVertexAttribArray(9);glVertexAttrib4f(9,0,0,0,1);glDeleteBuffers(1,&ab);

if(pbr){
 glBindBuffer(GL_ARRAY_BUFFER,vbo);glBufferData(GL_ARRAY_BUFFER,sizeof(verts),verts,GL_STATIC_DRAW);
 glVertexAttrib1f(4,1);glVertexAttrib4f(6,1,1,0,0);glVertexAttrib4f(7,1,1,0,0);glVertexAttrib4f(8,1,1,0,0);
 glUniform1i(u("spriteAtlas"),atlas);glUniform4f(u("spriteNormalUV"),1,1,0,0);
 glUniform1i(u("spritePbrEnabled"),1);glUniform1i(u("spriteNormalEnabled"),1);glUniform1i(u("spritePackedMaterial"),1);glUniform1i(u("spriteSkyAvailable"),1);
 glUniform1i(u("tex"),0);glUniform1i(u("detailTex"),7);glUniform1i(u("spriteNormalMap"),11);glUniform1i(u("spriteSkyMap"),12);
 glUniform1i(u("spriteStaticDepth"),8);glUniform1i(u("spriteLiveDepth"),9);glUniform1i(u("spriteLiveOwner"),10);
 glUniform1i(u("spriteShadows"),0);glUniform1f(u("reflectionClip"),0);glUniform1i(u("softOnly"),0);glUniform3f(u("fogEye"),0,0,2);
 glUniform3f(u("spriteSunRadiance"),.5,.5,.5);glUniform3f(u("spriteSunDirection"),0,0,1);
 GLuint maps[5];glGenTextures(5,maps);
 auto upload=[&](int unit,GLuint id,unsigned char r,unsigned char g,unsigned char b,unsigned char a){unsigned char px[]={r,g,b,a};glActiveTexture(GL_TEXTURE0+unit);glBindTexture(GL_TEXTURE_2D,id);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,px);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);};
 upload(0,tx,128,128,128,255);upload(11,maps[0],128,128,255,0);upload(12,maps[1],0,0,255,255);
 upload(8,maps[2],0,0,0,255);upload(9,maps[3],255,255,255,255);upload(10,maps[4],0,0,0,255);
 auto pixel=[&](){std::array<unsigned char,4> value{};glClear(GL_COLOR_BUFFER_BIT);glDrawArrays(GL_TRIANGLES,0,6);glReadPixels(8,8,1,1,GL_RGBA,GL_UNSIGNED_BYTE,value.data());assert(glGetError()==GL_NO_ERROR);return value;};
 glUniform3f(u("spritePbrStrength"),0,0,0);auto base=pixel();
 glUniform3f(u("spritePbrStrength"),0,4,0);auto zeroSpec=pixel();assert(zeroSpec==base);
 upload(11,maps[0],128,128,255,153);auto spec=pixel();assert(spec[0]>base[0]+10);
 glUniform1i(u("spriteSpecularDebug"),1);auto debugNoSky=pixel();glUniform3f(u("spritePbrStrength"),1,4,0);auto debugSky=pixel();assert(debugNoSky==debugSky);glUniform1i(u("spriteSpecularDebug"),0);
 upload(11,maps[0],128,128,255,255);glUniform3f(u("spritePbrStrength"),1,0,0);auto skyOnly=pixel();glUniform3f(u("spritePbrStrength"),1,.25,0);auto sunLow=pixel();glUniform3f(u("spritePbrStrength"),1,16,0);auto sunHigh=pixel();assert(sunLow[0]>skyOnly[0]&&sunHigh[0]>sunLow[0]);
 upload(11,maps[0],128,128,255,0);
 glUniform3f(u("spriteSunDirection"),0,0,-1);glUniform3f(u("spritePbrStrength"),0,0,1);
 auto thick=pixel();assert(thick==base);
 upload(11,maps[0],128,128,0,0);auto thin=pixel();assert(thin[0]>base[0]+15);
 upload(11,maps[0],128,128,0,255);auto metal=pixel();assert(metal==base);
 glUniform3f(u("spritePbrStrength"),1,0,0);auto reflected=pixel();assert(reflected[0]<3&&reflected[2]>=base[2]-1);
 if(shadows){
  upload(11,maps[0],128,128,0,0);glUniform3f(u("spritePbrStrength"),0,0,1);
  glUniform1i(u("spriteShadowSamples"),1);glUniform1i(u("spriteShadows"),1);glUniform1f(u("spriteShadowAmount"),1);glUniformMatrix4fv(u("spriteSunMatrix"),1,GL_FALSE,ident);
  auto blocked=pixel();assert(blocked[0]<3&&blocked[1]<3&&blocked[2]<3);
 }
 // Legacy alpha is validity, never metallic. PBR must leave it unchanged.
 glUniform1i(u("spriteShadows"),0);glUniform1i(u("spritePackedMaterial"),0);upload(11,maps[0],128,128,255,255);
 glUniform3f(u("spritePbrStrength"),1,4,4);assert(pixel()==base);
 glDeleteTextures(5,maps);printf("PASS PBR pixels: specular, thickness, metallic reflection, legacy%s\n",shadows?", received shadows":"");
}
glDeleteProgram(p);glDeleteBuffers(1,&vbo);glDeleteVertexArrays(1,&vao);glDeleteTextures(1,&tx);}
puts("PASS particle alpha batching pixel-identical in normal/reflection; all scene shader variants: cutout discard + opaque alpha, reflection, original opaque/soft alpha retained");}
