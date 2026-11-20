#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <cstdio>
#include <cassert>
#include <cmath>
#include <algorithm>
#include "../src/vr/detail_materials.h"
#include "../src/vr/color_pipeline.h"
GLuint compile(GLenum type,const char*s){GLuint x=glCreateShader(type);glShaderSource(x,1,&s,0);glCompileShader(x);GLint ok;glGetShaderiv(x,GL_COMPILE_STATUS,&ok);if(!ok){char b[8192];glGetShaderInfoLog(x,sizeof(b),0,b);puts(b);}assert(ok);return x;}
int main(){auto d=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(d,0,0));EGLint a[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};EGLConfig cfg;EGLint n;assert(eglChooseConfig(d,a,&cfg,1,&n)&&n);EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE},pa[]={EGL_WIDTH,16,EGL_HEIGHT,16,EGL_NONE};auto ctx=eglCreateContext(d,cfg,EGL_NO_CONTEXT,ca);auto surface=eglCreatePbufferSurface(d,cfg,pa);assert(eglMakeCurrent(d,surface,surface,ctx));

// CPU: all material/zero permutations and barycentric positions.
unsigned checks=0;
for(unsigned a=0;a<4;a++)for(unsigned b=0;b<4;b++)for(unsigned c=0;c<4;c++){
 std::array<unsigned,3> ids{a,b,c};auto packed=detail_materials::compact(ids);
 for(int x=0;x<=20;x++)for(int y=0;y<=20-x;y++){
  float weights[]={x/20.f,y/20.f,(20-x-y)/20.f},old=1.f,next=1.f;
  for(int i=0;i<3;i++)if(ids[i])old+=(ids[i]*.137f-.25f)*weights[i];
  for(int i=0;i<3;i++)if(packed.ids[i]){float weight=0;for(int j=0;j<3;j++)weight+=weights[j]*packed.weights[j][i];next+=(packed.ids[i]*.137f-.25f)*weight;}
  assert(fabsf(old-next)<1e-6f);++checks;
 }
}
GLuint p=glCreateProgram();auto vs=spriteShaderSource(mythVertexShader,false,false,false),fs=spriteShaderSource(mythFragmentShader,false,false,false);
glAttachShader(p,compile(GL_VERTEX_SHADER,vs.c_str()));glAttachShader(p,compile(GL_FRAGMENT_SHADER,fs.c_str()));glLinkProgram(p);GLint ok;glGetProgramiv(p,GL_LINK_STATUS,&ok);assert(ok);glUseProgram(p);auto u=[&](const char*s){return glGetUniformLocation(p,s);};
glUniform1i(u("tex"),0);glUniform1i(u("detailTex"),1);glUniform1i(u("detailEnabled"),1);glUniform4f(u("tint"),1,1,1,1);glUniform1f(u("reflectionHeight"),-10.f);glUniform3f(u("fogEye"),0,0,3);
float ident[]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};glUniformMatrix4fv(u("mvp"),1,GL_FALSE,ident);
GLuint textures[2],vao,vbo;glGenTextures(2,textures);glBindTexture(GL_TEXTURE_2D,textures[0]);unsigned char ground[]={130,170,90,255};glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,ground);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D_ARRAY,textures[1]);unsigned char texels[4*8*8*4];for(int layer=0;layer<4;layer++)for(int y=0;y<8;y++)for(int x=0;x<8;x++)for(int ch=0;ch<4;ch++)texels[((layer*8+y)*8+x)*4+ch]=ch==3?255:40+(x*31+y*17+layer*71+ch*59)%180;
glTexImage3D(GL_TEXTURE_2D_ARRAY,0,GL_RGBA8,8,8,4,0,GL_RGBA,GL_UNSIGNED_BYTE,texels);glGenerateMipmap(GL_TEXTURE_2D_ARRAY);glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glActiveTexture(GL_TEXTURE0);
float scales[]={1,1,1,1,2,2,3,3};glUniform2fv(u("detailScales"),4,scales);glGenVertexArrays(1,&vao);glBindVertexArray(vao);glGenBuffers(1,&vbo);glBindBuffer(GL_ARRAY_BUFFER,vbo);
int sizes[]={3,2,3,3},offsets[]={0,3,5,8};for(int i=0;i<4;i++){glEnableVertexAttribArray(i);glVertexAttribPointer(i,sizes[i],GL_FLOAT,GL_FALSE,11*sizeof(float),(void*)(offsets[i]*sizeof(float)));}
glViewport(0,0,16,16);glDisable(GL_DITHER);unsigned images=0;int maxError=0;
for(unsigned a=0;a<4;a++)for(unsigned b=0;b<4;b++)for(unsigned c=0;c<4;c++)for(float scale:{1.f,16.f,256.f})for(int fade=0;fade<2;fade++)for(int reflection=0;reflection<2;reflection++){
 std::array<unsigned,3> ids{a,b,c};auto packed=detail_materials::compact(ids);glUniform2f(u("detailDimensions"),scale,scale);glUniform2f(u("detailLodRange"),fade?2.f:0.f,fade?4.f:0.f);glUniform4f(u("fogColorDensity"),.4,.5,.6,fade?.08f:0.f);glUniform1f(u("reflectionClip"),reflection);
 unsigned char reference[1024],optimized[1024];
 for(int mode=0;mode<2;mode++){
  float data[33]={};for(int i=0;i<3;i++){float x=i==1?3.f:-1.f,y=i==2?3.f:-1.f;data[i*11]=x;data[i*11+1]=y;data[i*11+3]=(x+1)/2;data[i*11+4]=(y+1)/2;for(int k=0;k<3;k++){data[i*11+5+k]=mode?packed.ids[k]:float(ids[k]);data[i*11+8+k]=mode?packed.weights[i][k]:(i==k?1.f:0.f);}}
  glBufferData(GL_ARRAY_BUFFER,sizeof(data),data,GL_STREAM_DRAW);glClearColor(0,0,0,0);glClear(GL_COLOR_BUFFER_BIT);glDrawArrays(GL_TRIANGLES,0,3);glReadPixels(0,0,16,16,GL_RGBA,GL_UNSIGNED_BYTE,mode?optimized:reference);assert(glGetError()==GL_NO_ERROR);
 }
 for(int i=0;i<1024;i++){int error=std::abs(int(reference[i])-int(optimized[i]));maxError=std::max(maxError,error);assert(error<=1);}++images;
}
printf("PASS CPU %u barycentric cases; GPU %u image comparisons, max byte error=%d (normal/reflection, zero/duplicate IDs, mip scales, detail fade, fog)\n",checks,images,maxError);
}
