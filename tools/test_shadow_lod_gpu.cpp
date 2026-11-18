#include "../src/vr/main.cpp"
#define mythVertexShader referenceVS
#define mythFragmentShader referenceFS
#include "fixtures/lighting_before_culling.h"
#undef mythVertexShader
#undef mythFragmentShader
#include <cassert>
int main(){
 EGLDisplay d=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(d,0,0));EGLint a[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_NONE};EGLConfig cfg;EGLint n;assert(eglChooseConfig(d,a,&cfg,1,&n)&&n);EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE},pa[]={EGL_WIDTH,64,EGL_HEIGHT,64,EGL_NONE};auto c=eglCreateContext(d,cfg,EGL_NO_CONTEXT,ca);auto surf=eglCreatePbufferSurface(d,cfg,pa);assert(eglMakeCurrent(d,surf,surf,c));
 glGenVertexArrays(1,&vao);glBindVertexArray(vao);glGenBuffers(1,&vbo);Vertex v[]={{-1,1,0,0,0},{1,1,0,1,0},{-1,-1,0,0,1},{1,1,0,1,0},{1,-1,0,1,1},{-1,-1,0,0,1}};attributes(vbo);glBufferData(GL_ARRAY_BUFFER,sizeof(v),v,GL_STATIC_DRAW);glVertexAttrib1f(4,1);glVertexAttrib4f(5,0,0,0,1);
 auto link=[&](const char*fs){auto p=glCreateProgram();glAttachShader(p,shader(GL_VERTEX_SHADER,fs==referenceFS?referenceVS:mythVertexShader));glAttachShader(p,shader(GL_FRAGMENT_SHADER,fs));glLinkProgram(p);GLint ok;glGetProgramiv(p,GL_LINK_STATUS,&ok);if(!ok){char log[4096];glGetProgramInfoLog(p,sizeof(log),nullptr,log);fprintf(stderr,"LINK: %s\n",log);}assert(ok);return p;};
 auto budget=link("#version 300 es\nprecision highp float;flat in float shadowSampleBudget;out vec4 c;void main(){c=vec4(shadowSampleBudget/9.0,0,0,1);}");auto old=link(referenceFS),current=link(mythFragmentShader);
 Mat identity{};identity[0]=identity[5]=identity[10]=identity[15]=1;
 auto init=[&](GLuint p){program=p;glUseProgram(p);glUniformMatrix4fv(glGetUniformLocation(p,"mvp"),1,GL_FALSE,identity.data());glUniform1i(glGetUniformLocation(p,"spriteLightCull"),1);};
 init(budget);glUniform1f(glGetUniformLocation(budget,"spriteShadowLodDistance"),10);
 auto sampleBudget=[&](int maximum,float distance,float radius){glUniform1i(glGetUniformLocation(budget,"spriteShadowSamples"),maximum);glUniform3f(glGetUniformLocation(budget,"spriteShadowLodEye"),0,0,distance);glVertexAttrib4f(5,0,0,0,radius);glDrawArrays(GL_TRIANGLES,0,6);unsigned char px[4];glReadPixels(32,32,1,1,GL_RGBA,GL_UNSIGNED_BYTE,px);return px[0]/255.f*9;};
 for(int maximum=1;maximum<=9;maximum++){float near=sampleBudget(maximum,10,1),mid=sampleBudget(maximum,17,1),far=sampleBudget(maximum,25,1),tiny=sampleBudget(maximum,17,.001);assert(abs(near-maximum)<.04&&mid<=near+.04&&far<1.04&&tiny<1.04);float left=sampleBudget(maximum,14.999,1),right=sampleBudget(maximum,15.001,1);assert(abs(left-right)<.04);printf("LOD max=%d near=%.2f middle=%.2f far=%.2f small=%.2f\n",maximum,near,mid,far,tiny);}
 glUniform1f(glGetUniformLocation(budget,"spriteShadowLodDistance"),0);assert(sampleBudget(9,1000,.001)>8.96);
 GLuint textures[4];glGenTextures(4,textures);std::vector<unsigned char> pixels(8*8*4,255);
 for(int t=0;t<4;t++){for(int y=0;y<8;y++)for(int x=0;x<8;x++){size_t at=(y*8+x)*4;pixels[at]=t==0?255:t==3?((x+y)%3):((x+y+t)%2?255:0);pixels[at+1]=pixels[at+2]=t==3?0:pixels[at];}glBindTexture(GL_TEXTURE_2D,textures[t]);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,8,8,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);}
 auto render=[&](GLuint p,int samples){init(p);auto loc=[&](const char*s){return glGetUniformLocation(p,s);};glUniform1i(loc("spriteShadowSamples"),samples);glUniform1f(loc("spriteShadowLodDistance"),0);glUniform1i(loc("tex"),0);glUniform1i(loc("detailTex"),1);glUniform1i(loc("spriteShadows"),1);glUniform1f(loc("spriteShadowAmount"),.6);glUniformMatrix4fv(loc("spriteSunMatrix"),1,GL_FALSE,identity.data());glUniform4f(loc("tint"),1,1,1,1);
 const char*names[]={"tex","spriteStaticDepth","spriteLiveDepth","spriteLiveOwner"};for(int i=0;i<4;i++){int unit=i?7+i:0;glActiveTexture(GL_TEXTURE0+unit);glBindTexture(GL_TEXTURE_2D,textures[i]);glUniform1i(loc(names[i]),unit);}glActiveTexture(GL_TEXTURE0);glDrawArrays(GL_TRIANGLES,0,6);std::vector<unsigned char> out(64*64*4);glReadPixels(0,0,64,64,GL_RGBA,GL_UNSIGNED_BYTE,out.data());assert(glGetError()==GL_NO_ERROR);return out;};
 auto before=render(old,9),after=render(current,9);int delta=0;for(size_t i=0;i<after.size();i++)delta=std::max(delta,abs(int(before[i])-int(after[i])));assert(delta<=1);for(int i=1;i<=9;i++)render(current,i);printf("PASS samples 1..9, static early-out and live owner exclusion; nine-sample reference max_error=%d; LOD OFF/continuous/bounds\n",delta);
}
