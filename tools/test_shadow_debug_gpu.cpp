#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <cstdio>
#include <cassert>
#include "../src/vr/shadow_debug_shader.h"
#include <cstring>
GLuint compile(GLenum type,const char*s){GLuint x=glCreateShader(type);glShaderSource(x,1,&s,0);glCompileShader(x);GLint ok;glGetShaderiv(x,GL_COMPILE_STATUS,&ok);if(!ok){char b[8192];glGetShaderInfoLog(x,sizeof(b),0,b);puts(b);}assert(ok);return x;}
int main(){auto d=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(d,0,0));EGLint a[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};EGLConfig cfg;EGLint n;assert(eglChooseConfig(d,a,&cfg,1,&n)&&n);EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE},pa[]={EGL_WIDTH,16,EGL_HEIGHT,16,EGL_NONE};auto ctx=eglCreateContext(d,cfg,EGL_NO_CONTEXT,ca);auto surface=eglCreatePbufferSurface(d,cfg,pa);assert(eglMakeCurrent(d,surface,surface,ctx));

GLuint p=glCreateProgram();glAttachShader(p,compile(GL_VERTEX_SHADER,shadowDebugVS));glAttachShader(p,compile(GL_FRAGMENT_SHADER,shadowDebugFS));glLinkProgram(p);GLint ok;glGetProgramiv(p,GL_LINK_STATUS,&ok);assert(ok);glUseProgram(p);auto u=[&](const char*n){return glGetUniformLocation(p,n);};
float identity[]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};glUniformMatrix4fv(u("mvp"),1,GL_FALSE,identity);glUniformMatrix4fv(u("sunMatrix"),1,GL_FALSE,identity);glUniform1i(u("samples"),3);
GLuint vao,vbo;glGenVertexArrays(1,&vao);glBindVertexArray(vao);glGenBuffers(1,&vbo);glBindBuffer(GL_ARRAY_BUFFER,vbo);float vertices[]={-1,-1,0, 1,-1,0, -1,1,0, -1,1,0, 1,-1,0, 1,1,0};glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STATIC_DRAW);glEnableVertexAttribArray(0);glVertexAttribPointer(0,3,GL_FLOAT,0,12,0);
GLuint tx[3];glGenTextures(3,tx);const char*names[]={"staticDepth","liveDepth","liveOwner"};for(int i=0;i<3;i++){glActiveTexture(GL_TEXTURE0+i);glBindTexture(GL_TEXTURE_2D,tx[i]);unsigned char white[]={255,255,255,255};glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,white);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);glUniform1i(u(names[i]),i);}
auto set=[&](int i,unsigned v){unsigned char b[]={static_cast<unsigned char>(v),static_cast<unsigned char>(v>>8),static_cast<unsigned char>(v>>16),255};glActiveTexture(GL_TEXTURE0+i);glBindTexture(GL_TEXTURE_2D,tx[i]);glTexSubImage2D(GL_TEXTURE_2D,0,0,0,1,1,GL_RGBA,GL_UNSIGNED_BYTE,b);};
auto check=[&](const char*label,int r,int g,int b,bool hit){glClearColor(0,0,0,0);glClear(GL_COLOR_BUFFER_BIT);glDrawArrays(GL_TRIANGLES,0,6);unsigned char pix[4];glReadPixels(8,8,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pix);assert(pix[0]==r&&pix[1]==g&&pix[2]==b);assert(hit?pix[3]>0:pix[3]==0);assert(glGetError()==GL_NO_ERROR);puts(label);};glViewport(0,0,16,16);
set(0,255);set(1,255);set(2,0);check("lit terrain unchanged",0,0,0,false);
set(0,0);check("static cyan",0,255,255,true);
set(0,255);set(1,0);set(2,16777215);check("animated model magenta",255,0,255,true);
set(2,17);check("sprite yellow",255,255,0,true);
set(0,0);check("overlap white",255,255,255,true);
glUniform1f(u("bias"),.6);check("same depth bias",0,0,0,false);glUniform1f(u("bias"),0);identity[12]=4;glUniformMatrix4fv(u("sunMatrix"),1,GL_FALSE,identity);check("outside shadow map unchanged",0,0,0,false);
puts("PASS shadow overlay shader");}
