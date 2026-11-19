#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <cassert>
#include <cstdio>
#include <vector>
#include <random>
#include "../src/vr/render_culling.h"
#include "../src/vr/water_visibility.h"
#include "../src/vr/weather_billboard.h"
struct V{float x,y,z;};
static GLuint shader(GLenum t,const char*s){GLuint h=glCreateShader(t);glShaderSource(h,1,&s,nullptr);glCompileShader(h);GLint ok;glGetShaderiv(h,GL_COMPILE_STATUS,&ok);assert(ok);return h;}
int main(){
 std::mt19937 rng(42);std::uniform_real_distribution<float> random(-5,5);
 std::array<float,16> identity{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},perspective{1,0,0,0,0,1,0,0,0,0,-1.02f,-1,0,0,-.202f,0};
 for(auto&m:{identity,perspective})for(int i=0;i<5000;i++){V v[3];rendercull::Bounds b;for(auto&p:v){p={random(rng),random(rng),random(rng)};b.add(p.x,p.y,p.z);}auto exact=rendercull::projectedTriangles(m,v,3),coarse=rendercull::projectedBounds(m,b);if(exact.valid()){assert(coarse.valid());assert(coarse.l<=exact.l+.0001f&&coarse.b<=exact.b+.0001f&&coarse.r>=exact.r-.0001f&&coarse.t>=exact.t-.0001f);}}
 for(V toward:{V{1,2,3},V{-3,-4,1},V{0,5,0},V{0,-5,0},V{0,0,2}}){V right,up;assert(snowBillboard(toward,right,up));auto dot=[](V a,V b){return a.x*b.x+a.y*b.y+a.z*b.z;};assert(fabs(dot(toward,right))<1e-5&&fabs(dot(toward,up))<1e-5&&fabs(dot(right,up))<1e-5);assert(fabs(dot(right,right)-1)<1e-5&&fabs(dot(up,up)-1)<1e-5);}V right,up;assert(!snowBillboard(V{0,0,0},right,up));
 auto d=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(d,0,0));EGLint a[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_DEPTH_SIZE,24,EGL_NONE};EGLConfig cfg;EGLint n;assert(eglChooseConfig(d,a,&cfg,1,&n)&&n);EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE},pa[]={EGL_WIDTH,32,EGL_HEIGHT,32,EGL_NONE};auto ctx=eglCreateContext(d,cfg,EGL_NO_CONTEXT,ca);auto surf=eglCreatePbufferSurface(d,cfg,pa);assert(eglMakeCurrent(d,surf,surf,ctx));
 GLuint program=glCreateProgram();glAttachShader(program,shader(GL_VERTEX_SHADER,"#version 300 es\nlayout(location=0)in vec2 pos;uniform float z;void main(){gl_Position=vec4(pos,z,1);}"));glAttachShader(program,shader(GL_FRAGMENT_SHADER,"#version 300 es\nprecision highp float;out vec4 c;void main(){c=vec4(1,0,0,1);}"));glLinkProgram(program);GLint ok;glGetProgramiv(program,GL_LINK_STATUS,&ok);assert(ok);glUseProgram(program);auto z=glGetUniformLocation(program,"z");GLuint vao,buffer;glGenVertexArrays(1,&vao);glBindVertexArray(vao);glGenBuffers(1,&buffer);glBindBuffer(GL_ARRAY_BUFFER,buffer);float vertices[]={-1,-1,1,-1,-1,1,-1,1,1,-1,1,1};glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STATIC_DRAW);glEnableVertexAttribArray(0);glVertexAttribPointer(0,2,GL_FLOAT,0,8,0);glViewport(0,0,32,32);glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LEQUAL);glClearColor(0,0,0,1);
 WaterVisibility q;q.poll(1);assert(q.render);
 auto issue=[&](unsigned eye,uint64_t frame,bool hidden){glDepthMask(GL_TRUE);glColorMask(1,1,1,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);if(hidden){glUniform1f(z,-.5f);glDrawArrays(GL_TRIANGLES,0,6);}glUniform1f(z,0);glDepthMask(GL_FALSE);glColorMask(0,0,0,0);assert(q.begin(eye,frame));glDrawArrays(GL_TRIANGLES,0,6);q.end();};
 issue(0,1,true);issue(1,1,true);glFinish();q.poll(2);assert(!q.render);
 issue(0,2,false);issue(1,2,true);glFinish();q.poll(3);assert(q.render); // Either eye visible keeps both reflections.
 issue(0,3,true);issue(1,3,true);glFinish();q.poll(4);assert(!q.render);
 issue(0,4,false);issue(1,4,false);glFinish();q.poll(5);assert(q.render); // Queries continue while hidden.
 issue(0,5,true);issue(0,6,true);issue(0,7,true);assert(!q.begin(0,8)&&q.busy==1);glFinish();q.poll(9);assert(q.eyes[0].resultFrame==7&&!q.eyes[0].visible);
 q.poll(100);assert(q.render); // Old results fail open.
 q.release();assert(!q.eyes[0].known&&q.render);assert(glGetError()==GL_NO_ERROR);
 puts("PASS 10000 conservative projection cases, near clipping, snow camera-facing basis, hidden/revealed water, stereo union, full query ring, freshness and cleanup");
}
