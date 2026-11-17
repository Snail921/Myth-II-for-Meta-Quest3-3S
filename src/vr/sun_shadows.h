#pragma once
// Cached sun-space depth for fixed meshes. Live model/unit shadows reuse surface stamps.
static GLuint sunDepthProgram=0,sunDepthFBO=0;
static unsigned sunStaticBuilds=0;
static const char* sunDepthVS=R"(#version 300 es
layout(location=0) in vec3 pos;layout(location=1) in vec2 uv;
uniform mat4 mvp;out vec2 tc;
void main(){tc=uv;gl_Position=mvp*vec4(pos,1.);})";
static const char* sunDepthFS=R"(#version 300 es
precision highp float;uniform sampler2D tex;in vec2 tc;
void main(){if(texture(tex,tc).a<.5)discard;})";
static void resetSunShadows(){
 sunDepthProgram=sunDepthFBO=waterStaticDepth=0;waterSunShadowReady=false;
 sunStaticBuilds=0;
}
static void sunProjection(){
 Vec3 light=normalized({waterSun[0],waterSun[1],waterSun[2]});
 Vec3 right=normalized({light.z,0,-light.x});if(fabsf(light.y)>.999f)right={1,0,0};
 Vec3 up{light.y*right.z-light.z*right.y,light.z*right.x-light.x*right.z,light.x*right.y-light.y*right.x};
 Vec3 axes[]={right,up,times(light,-1)};float lo[3]={INFINITY,INFINITY,INFINITY},hi[3]={-INFINITY,-INFINITY,-INFINITY};
 auto include=[&](const rendercull::Bounds&b){for(int i=0;i<8;i++){Vec3 p{(i&1)?b.hi[0]:b.lo[0],(i&2)?b.hi[1]:b.lo[1],(i&4)?b.hi[2]:b.lo[2]};for(int j=0;j<3;j++){float v=dot(p,axes[j]);lo[j]=std::min(lo[j],v);hi[j]=std::max(hi[j],v);}}};
 for(auto&r:terrainRanges)include(r.bounds);for(auto&m:normalModels)include(m.bounds);for(auto&m:normalFallback)include(m.bounds);
 waterSunMatrix={};waterSunMatrix[15]=1;
 for(int j=0;j<3;j++){lo[j]-=32*scale;hi[j]+=32*scale;float extent=std::max(1.f,hi[j]-lo[j]);waterSunMatrix[j]=axes[j].x*2/extent;waterSunMatrix[4+j]=axes[j].y*2/extent;waterSunMatrix[8+j]=axes[j].z*2/extent;waterSunMatrix[12+j]=-(hi[j]+lo[j])/extent;if(j==2)waterSunDepthBias=.08f*scale/extent;}
}
static void allocateSunDepth(GLuint&texture,int size){
 glGenTextures(1,&texture);glBindTexture(GL_TEXTURE_2D,texture);glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT24,size,size,0,GL_DEPTH_COMPONENT,GL_UNSIGNED_INT,nullptr);
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
}
static void sunTarget(GLuint texture,int size){
 glBindFramebuffer(GL_FRAMEBUFFER,sunDepthFBO);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,texture,0);GLenum none=GL_NONE;glDrawBuffers(1,&none);glReadBuffer(GL_NONE);
 if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)throw std::runtime_error("sun shadow framebuffer");
 glViewport(0,0,size,size);glClearDepthf(1);glClear(GL_DEPTH_BUFFER_BIT);
}
static void updateSunShadows(){
 if(!reflectionEnabled||!waterWavesEnabled||reflectionPlanes.empty()||terrainRanges.empty())return;
 if(waterSunShadowReady)return;
 GLint savedFBO,viewport[4],savedProgram;glGetIntegerv(GL_FRAMEBUFFER_BINDING,&savedFBO);glGetIntegerv(GL_VIEWPORT,viewport);glGetIntegerv(GL_CURRENT_PROGRAM,&savedProgram);
 double started=seconds();glActiveTexture(GL_TEXTURE0);
 {sunProjection();sunDepthProgram=glCreateProgram();glAttachShader(sunDepthProgram,shader(GL_VERTEX_SHADER,sunDepthVS));glAttachShader(sunDepthProgram,shader(GL_FRAGMENT_SHADER,sunDepthFS));glLinkProgram(sunDepthProgram);GLint ok;glGetProgramiv(sunDepthProgram,GL_LINK_STATUS,&ok);if(!ok)throw std::runtime_error("sun shadow shader");glGenFramebuffers(1,&sunDepthFBO);allocateSunDepth(waterStaticDepth,2048);}
 glUseProgram(sunDepthProgram);glUniformMatrix4fv(glGetUniformLocation(sunDepthProgram,"mvp"),1,GL_FALSE,waterSunMatrix.data());glUniform1i(glGetUniformLocation(sunDepthProgram,"tex"),0);glBindVertexArray(vao);glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LESS);glDepthMask(GL_TRUE);glDisable(GL_BLEND);glDisable(GL_CULL_FACE);glDisable(GL_SCISSOR_TEST);glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);glEnable(GL_POLYGON_OFFSET_FILL);glPolygonOffset(1.5f,2.f);
 {sunTarget(waterStaticDepth,2048);attributes(terrainVBO);glBindTexture(GL_TEXTURE_2D,whiteTexture);glDrawArrays(GL_TRIANGLES,0,terrainCount);for(auto&m:normalModels){attributes(m.buffer);glBindTexture(GL_TEXTURE_2D,m.texture);glDrawArrays(GL_TRIANGLES,0,m.count);}sunStaticBuilds++;}
 glDisable(GL_POLYGON_OFFSET_FILL);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glBindFramebuffer(GL_FRAMEBUFFER,savedFBO);glViewport(viewport[0],viewport[1],viewport[2],viewport[3]);glUseProgram(savedProgram);glActiveTexture(GL_TEXTURE0);
 waterSunShadowReady=true;
 LOG("Sun shadow static=%u live casters=existing surface masks CPU submission_ms=%.3f",sunStaticBuilds,(seconds()-started)*1000);
}
