#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <cstdio>
#include <cassert>
#include "../src/vr/vertex_lod_profile.h"
#include "../src/vr/color_pipeline.h"
GLuint compile(GLenum type,const char*s){GLuint x=glCreateShader(type);glShaderSource(x,1,&s,0);glCompileShader(x);GLint ok;glGetShaderiv(x,GL_COMPILE_STATUS,&ok);if(!ok){char b[8192];glGetShaderInfoLog(x,sizeof(b),0,b);puts(b);}assert(ok);return x;}
int main(){auto d=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(d,0,0));EGLint a[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};EGLConfig cfg;EGLint n;assert(eglChooseConfig(d,a,&cfg,1,&n)&&n);EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE},pa[]={EGL_WIDTH,16,EGL_HEIGHT,16,EGL_NONE};auto ctx=eglCreateContext(d,cfg,EGL_NO_CONTEXT,ca);auto surface=eglCreatePbufferSurface(d,cfg,pa);assert(eglMakeCurrent(d,surface,surface,ctx));

auto vs=spriteShaderSource(mythVertexShader,true,true),fs=spriteShaderSource(mythFragmentShader,true,true);
GLuint p=glCreateProgram();glAttachShader(p,compile(GL_VERTEX_SHADER,vs.c_str()));glAttachShader(p,compile(GL_FRAGMENT_SHADER,fs.c_str()));
const char* outputs[]={"vertexShadeWeight","vertexVisibility","vertexLocalLight"};glTransformFeedbackVaryings(p,3,outputs,GL_INTERLEAVED_ATTRIBS);glLinkProgram(p);GLint ok;glGetProgramiv(p,GL_LINK_STATUS,&ok);assert(ok);glUseProgram(p);
auto u=[&](const char*s){return glGetUniformLocation(p,s);};
float identity[]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
glUniformMatrix4fv(u("mvp"),1,GL_FALSE,identity);glUniformMatrix4fv(u("spriteSunMatrix"),1,GL_FALSE,identity);
glUniform1i(u("spriteLightCull"),1);glUniform1f(u("spriteVertexLodDistance"),20);glUniform1i(u("spriteShadowSamples"),1);glUniform1f(u("spriteShadowAmount"),.7);glUniform3f(u("spriteSunDirection"),0,0,1);
glUniform1i(u("spriteObjectLightLimit"),8);glUniform1i(u("spriteLightCount"),1);glUniform4f(u("spriteLights[0]"),0,0,1,.01);glUniform4f(u("spriteLightColors[0]"),1,.2,.1,.5);
glUniform4f(u("tint"),1,1,1,1);glUniform4f(u("spriteNormalUV"),1,1,0,0);
GLuint vao;glGenVertexArrays(1,&vao);glBindVertexArray(vao);glVertexAttrib1f(4,1);glVertexAttrib4f(5,0,0,0,.1);
// Assign all sampler types distinct units and valid 1x1 depth/normal/color textures.
const char* samplers[]={"tex","spriteStaticDepth","spriteLiveDepth","spriteLiveOwner","spriteNormalMap"};
GLuint textures[5];glGenTextures(5,textures);for(int i=0;i<5;i++){glActiveTexture(GL_TEXTURE0+i);glBindTexture(GL_TEXTURE_2D,textures[i]);unsigned char pixel[]={255,255,255,255};if(i==1)pixel[0]=0;if(i==3)pixel[0]=pixel[1]=pixel[2]=0;if(i==4){pixel[0]=255;pixel[1]=128;pixel[2]=128;}
glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,pixel);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);glUniform1i(u(samplers[i]),i);}glUniform1i(u("detailTex"),6);
GLuint buffer;glGenBuffers(1,&buffer);glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER,buffer);glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER,24,nullptr,GL_STREAM_READ);glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER,0,buffer);
auto check=[&](float distance,float radius,int expected,float expectedVisibility=.3f){glUniform3f(u("spriteLightingEye"),0,0,distance);glUniform3f(u("fogEye"),0,0,distance);glVertexAttrib4f(5,0,0,0,radius);glEnable(GL_RASTERIZER_DISCARD);glBeginTransformFeedback(GL_POINTS);glDrawArrays(GL_POINTS,0,1);glEndTransformFeedback();auto*v=(float*)glMapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER,0,24,GL_MAP_READ_BIT);assert(v);if(expected!=0)assert(fabsf(v[0]-vertexlod::weight(distance,radius))<1e-5f);printf("d=%.1f radius=%.1f weight=%.3f visibility=%.3f light=%.3f\n",distance,radius,v[0],v[1],v[5]);if(expected==0)assert(v[0]==0);else if(expected==1){assert(v[0]==1);assert(v[5]>.4);if(true){assert(v[1]>expectedVisibility-.001&&v[1]<expectedVisibility+.001);}}else assert(v[0]>0&&v[0]<1);glUnmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);glDisable(GL_RASTERIZER_DISCARD);assert(glGetError()==GL_NO_ERROR);};
glUniform1i(u("spriteShadows"),1);check(5,.1,0);check(50,.1,1);check(50,10,0);check(22,.1,2);check(50,1.2,2);glVertexAttrib1f(4,0);check(50,.1,0);glVertexAttrib1f(4,1);glUniform1i(u("spriteLightCull"),0);check(50,.1,0);glUniform1i(u("spriteLightCull"),1);check(50,.1,1);
// Dynamic owner exclusion must also work in the vertex path.
auto pixel=[&](int unit,unsigned char r,unsigned char g,unsigned char b){glActiveTexture(GL_TEXTURE0+unit);glBindTexture(GL_TEXTURE_2D,textures[unit]);unsigned char v[]={r,g,b,255};glTexSubImage2D(GL_TEXTURE_2D,0,0,0,1,1,GL_RGBA,GL_UNSIGNED_BYTE,v);};
pixel(1,255,255,255);pixel(2,0,0,0);pixel(3,2,0,0);check(50,.1,1);pixel(3,1,0,0);check(50,.1,1,1);pixel(1,0,0,0);
// Rasterized far shading must ignore a deliberately extreme normal texture.
float vertices[]={-.05,-.05,0,0,0, .05,-.05,0,1,0, -.05,.05,0,0,1, -.05,.05,0,0,1, .05,-.05,0,1,0, .05,.05,0,1,1};
GLuint vb;glGenBuffers(1,&vb);glBindBuffer(GL_ARRAY_BUFFER,vb);glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STATIC_DRAW);glEnableVertexAttribArray(0);glVertexAttribPointer(0,3,GL_FLOAT,0,20,0);glEnableVertexAttribArray(1);glVertexAttribPointer(1,2,GL_FLOAT,0,20,(void*)12);identity[0]=identity[5]=20;glUniformMatrix4fv(u("mvp"),1,GL_FALSE,identity);glViewport(0,0,16,16);
unsigned char pixels[2][4];for(int i=0;i<2;i++){glUniform1i(u("spriteNormalEnabled"),i);glClear(GL_COLOR_BUFFER_BIT);glDrawArrays(GL_TRIANGLES,0,6);glReadPixels(8,8,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixels[i]);}assert(!memcmp(pixels[0],pixels[1],4));assert(pixels[0][3]==255&&pixels[0][0]>0);assert(glGetError()==GL_NO_ERROR);
glUniform1f(u("reflectionClip"),1);glUniform1f(u("reflectionHeight"),-1);glClear(GL_COLOR_BUFFER_BIT);glDrawArrays(GL_TRIANGLES,0,6);unsigned char reflected[4];glReadPixels(8,8,1,1,GL_RGBA,GL_UNSIGNED_BYTE,reflected);assert(!memcmp(pixels[1],reflected,4));glUniform1f(u("reflectionHeight"),1);glClear(GL_COLOR_BUFFER_BIT);glDrawArrays(GL_TRIANGLES,0,6);glReadPixels(8,8,1,1,GL_RGBA,GL_UNSIGNED_BYTE,reflected);assert(reflected[3]==0);assert(glGetError()==GL_NO_ERROR);
vertexlod::Counts counts;counts.add(5,.1,true);counts.add(50,10,true);counts.add(22,.1,true);counts.add(50,.1,true);counts.add(50,.1,false);assert(counts.total==5&&counts.detail==2&&counts.near==1&&counts.large==1&&counts.transition==1&&counts.vertex==1&&counts.nonreceiver==1);
puts("PASS diagnostic counts and GPU/CPU LOD agreement");
puts("PASS dynamic owner exclusion, reflection clip");
puts("PASS near detail, far vertex shadow/light, large-object protection, smooth transition, nonreceivers, far ignores normal map");
}
