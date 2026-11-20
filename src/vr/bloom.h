#pragma once
#include <GLES3/gl3.h>
#include <algorithm>
#include <cstdio>
#include <string>
#include <stdexcept>
namespace bloom {
static bool enabled=false;
static int resolution=0,iterations=1;
static float strength=.15f;
static constexpr int divisors[]={8,6,4,2,16,32};
static void load(const char*root){
 auto read=[&](const char*n,int fallback,int maximum){FILE*f=fopen((std::string(root)+"/"+n).c_str(),"rb");if(!f)return fallback;int v;char extra;bool ok=fscanf(f,"%d %c",&v,&extra)==1&&v>=0&&v<=maximum;fclose(f);return ok?v:fallback;};
 enabled=read("bloom.enabled",0,1)!=0;resolution=read("bloom.resolution",0,5);iterations=std::max(1,read("bloom.iterations",1,4));strength=std::min(200,read("bloom.strength",15,400))/100.f;
}
static bool active(){return enabled&&strength>0;}
static GLuint snapshot=0,snapshotFBO=0,textures[2]{},fbos[2]{},extractProgram=0,blurProgram=0,composeProgram=0,vao=0;
static int width=0,height=0,lowWidth=0,lowHeight=0;static GLenum format=0;
static const char*vertex=R"(#version 300 es
out vec2 uv;
void main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);uv=p;gl_Position=vec4(p*2.0-1.0,0,1);})";
static const char*extract=R"(#version 300 es
precision highp float;
in vec2 uv;out vec4 color;uniform sampler2D source;uniform vec2 footprint;
vec3 bright(vec2 p){
 vec4 c=texture(source,p);
 // Alpha is scene coverage: untouched sky is zero. Threshold before averaging
 // so small bright effects survive downsampling. Work entirely in linear RGB.
 float peak=max(c.r,max(c.g,c.b));float weight=smoothstep(.65,.95,peak);
 return c.rgb*weight*c.a;
}
void main(){vec3 sum=vec3(0);for(int y=0;y<4;y++)for(int x=0;x<4;x++)sum+=bright(uv+(vec2(x,y)-1.5)*footprint/4.0);color=vec4(sum/16.0,1);})";
static const char*blur=R"(#version 300 es
precision highp float;
in vec2 uv;out vec4 color;uniform sampler2D source;uniform vec2 stepUV;
void main(){vec3 c=texture(source,uv).rgb*.227027;
 c+=(texture(source,uv+stepUV*1.384615).rgb+texture(source,uv-stepUV*1.384615).rgb)*.316216;
 c+=(texture(source,uv+stepUV*3.230769).rgb+texture(source,uv-stepUV*3.230769).rgb)*.070270;
 color=vec4(c,1);})";
static const char*compose=R"(#version 300 es
precision highp float;
in vec2 uv;out vec4 color;uniform sampler2D source;uniform float strength;
void main(){color=vec4(texture(source,uv).rgb*strength,1);})";
static GLuint makeProgram(const char*fs){
 GLuint p=glCreateProgram();for(GLenum kind:{GLenum(GL_VERTEX_SHADER),GLenum(GL_FRAGMENT_SHADER)}){const char*s=kind==GL_VERTEX_SHADER?vertex:fs;GLuint sh=glCreateShader(kind);glShaderSource(sh,1,&s,nullptr);glCompileShader(sh);GLint ok;glGetShaderiv(sh,GL_COMPILE_STATUS,&ok);if(!ok){char log[2048];glGetShaderInfoLog(sh,sizeof(log),nullptr,log);glDeleteShader(sh);glDeleteProgram(p);throw std::runtime_error(std::string("Bloom shader: ")+log);}glAttachShader(p,sh);glDeleteShader(sh);}glLinkProgram(p);GLint ok;glGetProgramiv(p,GL_LINK_STATUS,&ok);if(!ok){glDeleteProgram(p);throw std::runtime_error("Bloom link");}return p;
}
static void releaseTargets(){glDeleteTextures(1,&snapshot);glDeleteFramebuffers(1,&snapshotFBO);glDeleteTextures(2,textures);glDeleteFramebuffers(2,fbos);snapshot=snapshotFBO=0;for(int i=0;i<2;i++)textures[i]=fbos[i]=0;width=height=lowWidth=lowHeight=0;}
static void release(){releaseTargets();for(GLuint p:{extractProgram,blurProgram,composeProgram})if(p)glDeleteProgram(p);if(vao)glDeleteVertexArrays(1,&vao);extractProgram=blurProgram=composeProgram=vao=0;}
static void target(GLuint&texture,GLuint&fbo,int w,int h,GLenum fmt){
 glGenTextures(1,&texture);glBindTexture(GL_TEXTURE_2D,texture);glTexImage2D(GL_TEXTURE_2D,0,fmt,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
 glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,texture,0);if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)throw std::runtime_error("Bloom framebuffer");
}
// Restore all state touched here, including independent read/draw FBO bindings.
struct State {
 GLint draw,read,viewport[4],program,vao,active,texture,sampler,srcRGB,dstRGB,srcA,dstA,eqRGB,eqA;GLboolean depthMask,colorMask[4];
 GLenum caps[5]={GL_DEPTH_TEST,GL_STENCIL_TEST,GL_BLEND,GL_SCISSOR_TEST,GL_CULL_FACE};GLboolean on[5];
 State(){glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&draw);glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING,&read);glGetIntegerv(GL_VIEWPORT,viewport);glGetIntegerv(GL_CURRENT_PROGRAM,&program);glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&vao);glGetIntegerv(GL_ACTIVE_TEXTURE,&active);glActiveTexture(GL_TEXTURE0);glGetIntegerv(GL_TEXTURE_BINDING_2D,&texture);glGetIntegerv(GL_SAMPLER_BINDING,&sampler);glBindSampler(0,0);glGetIntegerv(GL_BLEND_SRC_RGB,&srcRGB);glGetIntegerv(GL_BLEND_DST_RGB,&dstRGB);glGetIntegerv(GL_BLEND_SRC_ALPHA,&srcA);glGetIntegerv(GL_BLEND_DST_ALPHA,&dstA);glGetIntegerv(GL_BLEND_EQUATION_RGB,&eqRGB);glGetIntegerv(GL_BLEND_EQUATION_ALPHA,&eqA);glGetBooleanv(GL_DEPTH_WRITEMASK,&depthMask);glGetBooleanv(GL_COLOR_WRITEMASK,colorMask);for(int i=0;i<5;i++){on[i]=glIsEnabled(caps[i]);glDisable(caps[i]);}glDepthMask(GL_FALSE);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);}
 ~State(){glBindFramebuffer(GL_DRAW_FRAMEBUFFER,draw);glBindFramebuffer(GL_READ_FRAMEBUFFER,read);glViewport(viewport[0],viewport[1],viewport[2],viewport[3]);glUseProgram(program);glBindVertexArray(vao);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,texture);glBindSampler(0,sampler);glActiveTexture(active);glBlendFuncSeparate(srcRGB,dstRGB,srcA,dstA);glBlendEquationSeparate(eqRGB,eqA);glDepthMask(depthMask);glColorMask(colorMask[0],colorMask[1],colorMask[2],colorMask[3]);for(int i=0;i<5;i++)if(on[i])glEnable(caps[i]);else glDisable(caps[i]);}
};
static void render(){
 if(!active())return; // No allocation, snapshot, shader, or GL calls when OFF.
 State saved;int w=saved.viewport[2],h=saved.viewport[3];if(w<1||h<1)return;
 GLint encoding=GL_LINEAR;glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING,&encoding);
 GLenum fmt=encoding==GL_SRGB?GL_SRGB8_ALPHA8:GL_RGBA8;int lw=std::max(1,(w+divisors[resolution]-1)/divisors[resolution]),lh=std::max(1,(h+divisors[resolution]-1)/divisors[resolution]);
 if(!extractProgram){extractProgram=makeProgram(extract);blurProgram=makeProgram(blur);composeProgram=makeProgram(compose);glGenVertexArrays(1,&vao);}
 if(w!=width||h!=height||lw!=lowWidth||lh!=lowHeight||fmt!=format){releaseTargets();target(snapshot,snapshotFBO,w,h,fmt);for(int i=0;i<2;i++)target(textures[i],fbos[i],lw,lh,GL_RGBA8);width=w;height=h;lowWidth=lw;lowHeight=lh;format=fmt;}
 // Same-size color resolve works both with ordinary and multisampled eye FBOs.
 // This full-resolution snapshot is the principal bandwidth cost of enabling Bloom.
 glBindFramebuffer(GL_READ_FRAMEBUFFER,saved.draw);glBindFramebuffer(GL_DRAW_FRAMEBUFFER,snapshotFBO);glBlitFramebuffer(saved.viewport[0],saved.viewport[1],saved.viewport[0]+w,saved.viewport[1]+h,0,0,w,h,GL_COLOR_BUFFER_BIT,GL_NEAREST);
 glBindVertexArray(vao);glBindFramebuffer(GL_FRAMEBUFFER,fbos[0]);glViewport(0,0,lw,lh);glUseProgram(extractProgram);glBindTexture(GL_TEXTURE_2D,snapshot);glUniform1i(glGetUniformLocation(extractProgram,"source"),0);glUniform2f(glGetUniformLocation(extractProgram,"footprint"),1.f/lw,1.f/lh);glDrawArrays(GL_TRIANGLES,0,3);
 glUseProgram(blurProgram);glUniform1i(glGetUniformLocation(blurProgram,"source"),0);
 for(int i=0;i<iterations;i++)for(int axis=0;axis<2;axis++){glBindFramebuffer(GL_FRAMEBUFFER,fbos[1-axis]);glBindTexture(GL_TEXTURE_2D,textures[axis]);glUniform2f(glGetUniformLocation(blurProgram,"stepUV"),axis?0:1.f/lw,axis?1.f/lh:0);glDrawArrays(GL_TRIANGLES,0,3);}
 glBindFramebuffer(GL_FRAMEBUFFER,saved.draw);glViewport(saved.viewport[0],saved.viewport[1],w,h);glUseProgram(composeProgram);glBindTexture(GL_TEXTURE_2D,textures[0]);glUniform1i(glGetUniformLocation(composeProgram,"source"),0);glUniform1f(glGetUniformLocation(composeProgram,"strength"),strength);
 // Add in linear light; restore opaque eye alpha after using it as sky coverage.
 glEnable(GL_BLEND);glBlendEquation(GL_FUNC_ADD);glBlendFuncSeparate(GL_ONE,GL_ONE,GL_ONE,GL_ZERO);glDrawArrays(GL_TRIANGLES,0,3);
}
}
