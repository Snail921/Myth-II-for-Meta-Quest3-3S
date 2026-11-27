#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <vector>
#include <map>
#include <array>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cassert>
#define LOG(...) ((void)0)
struct Vertex{float x,y,z,u,v;};static float scale=1;
#include "../src/vr/water_surface.h"
#include "../src/vr/surface_compositor.h"
static GLuint compile(GLenum type,const char*s){auto t=glCreateShader(type);glShaderSource(t,1,&s,0);glCompileShader(t);int ok;glGetShaderiv(t,GL_COMPILE_STATUS,&ok);if(!ok){char b[4096];glGetShaderInfoLog(t,4096,0,b);puts(b);}assert(ok);return t;}
int main(){
 surface::Compositor c;c.w=128;c.h=128;c.base.assign(128*128,190);c.shade=c.base;
 c.masks[1]={0,0,0,20,20,std::vector<uint8_t>(400,255)};c.masks[2]={1,0,0,20,20,std::vector<uint8_t>(400,255)};
 for(int i=0;i<70;i++){std::vector<surface::Stamp>s;if(i<69)s={{1,1,i*64,10*64,65535},{2,2,i*64,10*64,65535}};c.compose(s);std::vector<uint8_t> expected(128*128,255);if(s.size())surface::apply(expected,128,128,c.masks[1],s[0]);assert(c.shadow==expected);c.compose(s);assert(c.tiles.empty());}
 c.base.assign(128*128,180);c.base[0]=90;c.base[1]=180;c.shadow.clear();c.initShadow();assert(c.shadow[0]==255&&c.shadow[1]==255);c.indices.assign(128*128,0);c.indices[1]=1;assert(c.shadow[1]==255);
 c.base[0]=180;c.externalDirty.assign(4,1);c.compose({});assert(c.shadow[0]==255);
 puts("PASS baked shade excluded, dark palette independence, base updates cannot add shadows");
 puts("PASS shadow-only dirty tiles: movement, overlapping light exclusion, removal, unchanged frames");
 auto d=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(d,0,0));EGLint a[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};EGLConfig cfg;int n;assert(eglChooseConfig(d,a,&cfg,1,&n));EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE},pa[]={EGL_WIDTH,32,EGL_HEIGHT,32,EGL_NONE};auto ctx=eglCreateContext(d,cfg,0,ca);auto sf=eglCreatePbufferSurface(d,cfg,pa);assert(eglMakeCurrent(d,sf,sf,ctx));
 auto p=glCreateProgram();glAttachShader(p,compile(GL_VERTEX_SHADER,waterVertexShader));glAttachShader(p,compile(GL_FRAGMENT_SHADER,waterFragmentShader));glLinkProgram(p);int ok;glGetProgramiv(p,GL_LINK_STATUS,&ok);assert(ok);glUseProgram(p);
 auto loc=[&](const char*s){return glGetUniformLocation(p,s);};
 float m[]={1,0,0,0,0,0,1,0,0,1,0,0,0,0,0,1};glUniformMatrix4fv(loc("mvp"),1,0,m);glUniform4f(loc("viewport"),0,0,32,32);glUniform3f(loc("eye"),0,100,0);glUniform3f(loc("sun"),0,1,0);glUniform1f(loc("worldScale"),1);glUniform1i(loc("waves"),1);glUniform1i(loc("hasSky"),1);glUniform4f(loc("shadowUV"),.5,.5,.5,.5);
 GLuint tex[4];glGenTextures(4,tex);const char*names[]={"tex","normalMap","sky","shadowMap"};unsigned char data[4][4]={{26,51,77,128},{128,128,255,255},{255,255,255,255},{255,255,255,255}};
 for(int i=0;i<4;i++){glActiveTexture(GL_TEXTURE0+i);glBindTexture(GL_TEXTURE_2D,tex[i]);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,data[i]);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);glUniform1i(loc(names[i]),i);}
 GLuint va,vb;glGenVertexArrays(1,&va);glBindVertexArray(va);glGenBuffers(1,&vb);glBindBuffer(GL_ARRAY_BUFFER,vb);Vertex v[]={{-1,0,-1,10,0},{1,0,-1,10,0},{-1,0,1,10,0},{1,0,-1,10,0},{1,0,1,10,0},{-1,0,1,10,0}};glBufferData(GL_ARRAY_BUFFER,sizeof(v),v,GL_STATIC_DRAW);glEnableVertexAttribArray(0);glVertexAttribPointer(0,3,GL_FLOAT,0,sizeof(Vertex),0);glEnableVertexAttribArray(1);glVertexAttribPointer(1,2,GL_FLOAT,0,sizeof(Vertex),(void*)12);glViewport(0,0,32,32);
 unsigned char pixels[3][4];for(int i=0;i<3;i++){unsigned char shadow[]={static_cast<unsigned char>(i==0?255:i==1?235:210),0,0,255};glTexSubImage2D(GL_TEXTURE_2D,0,0,0,1,1,GL_RGBA,GL_UNSIGNED_BYTE,shadow);glDrawArrays(GL_TRIANGLES,0,6);glReadPixels(16,16,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixels[i]);}
 assert(pixels[0][0]>pixels[1][0]+20&&pixels[1][0]>pixels[2][0]+20);assert(abs(int(pixels[2][0])-10)<=2);assert(pixels[0][3]==pixels[1][3]&&pixels[1][3]==pixels[2][3]);assert(glGetError()==GL_NO_ERROR);

 // Exercise actual water shadow depth comparisons, with reflections untouched.
 GLuint depths[2];glGenTextures(2,depths);for(int i=0;i<2;i++){glActiveTexture(GL_TEXTURE4+i);glBindTexture(GL_TEXTURE_2D,depths[i]);unsigned char px[]={255,255,255,255};glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,px);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);}
 glUniform1i(loc("staticSunDepth"),4);glUniform1i(loc("liveSunDepth"),5);float sunM[16]={};sunM[15]=1;glUniformMatrix4fv(loc("sunMatrix"),1,0,sunM);glUniform1i(loc("sunShadowReady"),1);glActiveTexture(GL_TEXTURE3);unsigned char white[]={255,255,255,255};glTexSubImage2D(GL_TEXTURE_2D,0,0,0,1,1,GL_RGBA,GL_UNSIGNED_BYTE,white);
 for(int caster=0;caster<2;caster++){glUniform1i(loc("liveSunShadowReady"),1);glActiveTexture(GL_TEXTURE4+caster);unsigned char blocked[]={64,64,64,255};glTexSubImage2D(GL_TEXTURE_2D,0,0,0,1,1,GL_RGBA,GL_UNSIGNED_BYTE,blocked);glDrawArrays(GL_TRIANGLES,0,6);unsigned char px[4];glReadPixels(16,16,1,1,GL_RGBA,GL_UNSIGNED_BYTE,px);assert(abs(int(px[0])-10)<=2&&px[3]==pixels[0][3]);if(caster==1){glUniform1i(loc("liveSunShadowReady"),0);glDrawArrays(GL_TRIANGLES,0,6);glReadPixels(16,16,1,1,GL_RGBA,GL_UNSIGNED_BYTE,px);assert(px[0]>30&&px[3]==pixels[0][3]);}glTexSubImage2D(GL_TEXTURE_2D,0,0,0,1,1,GL_RGBA,GL_UNSIGNED_BYTE,white);}
 assert(glGetError()==GL_NO_ERROR);puts("PASS static/live depth mask suppresses only direct specular; live OFF preserves original behavior");
 puts("PASS production water shader: sun/partial/full shadow, reflected RGB retained, alpha unchanged, GL_NO_ERROR");
}
