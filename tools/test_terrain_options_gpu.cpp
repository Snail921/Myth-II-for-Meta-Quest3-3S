#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <cstdio>
#include <cassert>
#include <cmath>
#include <vector>
#define LOG(...) ((void)0)
#include "../src/vr/terrain_options.h"
#include "../src/vr/color_pipeline.h"
GLuint compile(GLenum type,const std::string&s){GLuint x=glCreateShader(type);const char*p=s.c_str();glShaderSource(x,1,&p,0);glCompileShader(x);GLint ok;glGetShaderiv(x,GL_COMPILE_STATUS,&ok);if(!ok){char b[8192];glGetShaderInfoLog(x,sizeof(b),0,b);puts(b);}assert(ok);return x;}
int main(){
 auto d=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(d,0,0));EGLint a[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};EGLConfig cfg;EGLint n;assert(eglChooseConfig(d,a,&cfg,1,&n)&&n);EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE},pa[]={EGL_WIDTH,32,EGL_HEIGHT,32,EGL_NONE};auto ctx=eglCreateContext(d,cfg,EGL_NO_CONTEXT,ca);auto surf=eglCreatePbufferSurface(d,cfg,pa);assert(eglMakeCurrent(d,surf,surf,ctx));
 GLuint p=glCreateProgram();glAttachShader(p,compile(GL_VERTEX_SHADER,spriteShaderSource(mythVertexShader,false,false,false)));glAttachShader(p,compile(GL_FRAGMENT_SHADER,spriteShaderSource(mythFragmentShader,false,false,false)));glLinkProgram(p);GLint ok;glGetProgramiv(p,GL_LINK_STATUS,&ok);assert(ok);glUseProgram(p);auto u=[&](const char*s){return glGetUniformLocation(p,s);};
 GLuint vao,vbo,tx,detail;glGenVertexArrays(1,&vao);glBindVertexArray(vao);glGenBuffers(1,&vbo);glBindBuffer(GL_ARRAY_BUFFER,vbo);
 // Same winding as the generated heightfield after X reversal, two diagonals.
 float v[]={.8,0,-.8,0,0, -.8,0,-.8,1,0, .8,0,.8,0,1, -.8,0,-.8,1,0, -.8,0,.8,1,1, .8,0,.8,0,1};
 glBufferData(GL_ARRAY_BUFFER,sizeof(v),v,GL_STATIC_DRAW);glEnableVertexAttribArray(0);glVertexAttribPointer(0,3,GL_FLOAT,0,20,0);glEnableVertexAttribArray(1);glVertexAttribPointer(1,2,GL_FLOAT,0,20,(void*)12);
 glVertexAttrib3f(2,1,2,3);glVertexAttrib3f(3,.2,.3,.5);
 glGenTextures(1,&tx);glBindTexture(GL_TEXTURE_2D,tx);unsigned char base[]={128,128,128,255};glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,base);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
 glGenTextures(1,&detail);glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D_ARRAY,detail);unsigned char colors[]={128,128,128,255, 255,128,128,255, 128,255,128,255, 128,128,255,255};glTexImage3D(GL_TEXTURE_2D_ARRAY,0,GL_RGBA8,1,1,4,0,GL_RGBA,GL_UNSIGNED_BYTE,colors);glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MAG_FILTER,GL_NEAREST);glActiveTexture(GL_TEXTURE0);
 glUniform1i(u("tex"),0);glUniform1i(u("detailTex"),1);glUniform1i(u("detailEnabled"),1);glUniform2f(u("detailDimensions"),1,1);float scales[]={1,1,1,1,1,1,1,1};glUniform2fv(u("detailScales"),4,scales);glUniform4f(u("tint"),1,1,1,1);glViewport(0,0,32,32);glClearColor(0,0,0,0);
 auto draw=[&](bool lod,float distance,bool below=false,bool reflected=false){float m[]={1,0,0,0, 0,0,-1,0, 0,below?1.f:-1.f,0,0, 0,0,0,1};if(reflected)m[6]*=-1;glUniformMatrix4fv(u("mvp"),1,0,m);glUniform3f(u("fogEye"),0,distance,0);glUniform2f(u("detailLodRange"),lod?terrainDetailNear:0,lod?terrainDetailFar:0);glClear(GL_COLOR_BUFFER_BIT);{TerrainCullScope c(reflected);glDrawArrays(GL_TRIANGLES,0,6);}assert(!glIsEnabled(GL_CULL_FACE));GLint winding;glGetIntegerv(GL_FRONT_FACE,&winding);assert(winding==GL_CCW);std::vector<unsigned char>out(4);glReadPixels(16,16,1,1,GL_RGBA,GL_UNSIGNED_BYTE,out.data());assert(glGetError()==GL_NO_ERROR);return out;};
 auto off=draw(false,10),near=draw(true,10),middle=draw(true,30),far=draw(true,50),offFar=draw(false,50);assert(off==near&&off==offFar);assert(off[3]==255&&far[3]==255);for(int i=0;i<3;i++){assert(off[i]>middle[i]&&middle[i]>far[i]);assert(abs(int(far[i])-55)<=1);}
 assert(draw(false,10,true,false)[3]==0);assert(draw(false,10,false,true)[3]==0);assert(draw(false,10,true,true)[3]==255);
 glUniform1i(u("detailEnabled"),0);assert(draw(true,10)==far);
 puts("PASS terrain: full detail OFF/near equality, smooth fade, far/base equality, three-material blend, top/bottom cull, reflected winding and two-sided state restoration");
}
