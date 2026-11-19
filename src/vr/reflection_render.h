#include "water_flow.h"
#include "water_visibility.h"
#include "water_surface.h"
// One original water plane. Each eye gets its own render, consumed immediately.
static bool reflectionEnabled=true,reflectionReady=false,reflectionFull=false,reflectionHigh=false;
static char reflectionSettingPath[1024]={0};
static double reflectionPoll=0,perfReflection=0;
static unsigned reflectionPasses=0;
static float reflectionLevel=0;
static GLuint reflectionFBO=0,reflectionTexture=0,reflectionDepth=0,reflectionWaterVBO=0,reflectionFlowVBO=0,reflectionProgram=0;
static int reflectionW=0,reflectionH=0;
static std::vector<Vertex> reflectionWater;
struct ReflectionBounds{float lo[3],hi[3];size_t first=0,count=0;};
static GLuint reflectionMaskProgram=0;
static unsigned waterChunksTested=0,waterChunksVisible=0,waterCornersProjected=0;
static double waterProjectionCPU=0;
static std::vector<ReflectionBounds> reflectionBounds;
struct ReflectionPlane {
 WaterVisibility visibility;float level=0;bool ready=false;GLuint fbo=0,texture=0,depth=0,vbo=0,flowVbo=0;int w=0,h=0;
 std::vector<Vertex> water;std::vector<ReflectionBounds> bounds;
};
static std::vector<ReflectionPlane> reflectionPlanes;static int activeReflectionPlane=-1;
static uint64_t waterVisibilityFrame=0;static double waterQueryCPU=0;
static void beginReflectionFrame(){++waterVisibilityFrame;for(auto&p:reflectionPlanes)p.visibility.poll(waterVisibilityFrame);}
static void releaseWaterVisibility(){for(auto&p:reflectionPlanes)p.visibility.release();waterVisibilityFrame=0;}
static void swapReflectionPlane(ReflectionPlane&p){
 std::swap(p.level,reflectionLevel);std::swap(p.ready,reflectionReady);std::swap(p.fbo,reflectionFBO);std::swap(p.texture,reflectionTexture);std::swap(p.depth,reflectionDepth);std::swap(p.vbo,reflectionWaterVBO);std::swap(p.flowVbo,reflectionFlowVBO);std::swap(p.w,reflectionW);std::swap(p.h,reflectionH);p.water.swap(reflectionWater);p.bounds.swap(reflectionBounds);
}
static bool activateReflectionPlane(unsigned index){
 if(activeReflectionPlane==(int)index)return false;
 if(activeReflectionPlane>=0)swapReflectionPlane(reflectionPlanes[activeReflectionPlane]);
 swapReflectionPlane(reflectionPlanes.at(index));activeReflectionPlane=index;return true;
}
static void pollReflectionSetting(){
 double now=seconds();if(now-reflectionPoll<1)return;reflectionPoll=now;pollWaterWaves();
 FILE*f=fopen(reflectionSettingPath,"rb");if(!f)return;int c=fgetc(f),full=fgetc(f),high=fgetc(f);fclose(f);
 if(c!='0'&&c!='1')return;if(full==EOF&&high==EOF)full=high='0';if((full!='0'&&full!='1')||(high!='0'&&high!='1'))return;bool enabled=c=='1',all=full=='1',hi=high=='1';if(enabled!=reflectionEnabled||all!=reflectionFull||hi!=reflectionHigh)LOG("Water reflections enabled=%d full=%d high=%d",enabled,all,hi);reflectionEnabled=enabled;reflectionFull=all;reflectionHigh=hi;
}
static void loadReflections(android_app*a){
 snprintf(waterSettingPath,sizeof(waterSettingPath),"%s/water-waves.enabled",a->activity->internalDataPath);waterWavesEnabled=true;initWaterNormal();
 snprintf(reflectionSettingPath,sizeof(reflectionSettingPath),"%s/reflection.enabled",a->activity->internalDataPath);reflectionPoll=0;pollReflectionSetting();
 auto b=asset(a,"reflection-water.pack");if(b.size()<12)throw std::runtime_error("reflection water header");
 bool grouped=!memcmp(b.data(),"MYTHRF03",8);bool multi=grouped||!memcmp(b.data(),"MYTHRF02",8);if(!multi&&memcmp(b.data(),"MYTHRF01",8))throw std::runtime_error("reflection magic");
 size_t pos=multi?12:8;unsigned planes=multi?u32(b.data()+8):1;if(planes>16)throw std::runtime_error("reflection plane count");
 releaseWaterVisibility();reflectionPlanes.clear();activeReflectionPlane=-1;reflectionWater.clear();reflectionBounds.clear();reflectionFBO=reflectionTexture=reflectionDepth=reflectionWaterVBO=reflectionFlowVBO=0;reflectionW=reflectionH=0;reflectionReady=false;
 for(unsigned pi=0;pi<planes;pi++){
  if(pos+8>b.size())throw std::runtime_error("reflection plane header");float level;memcpy(&level,b.data()+pos,4);unsigned n=u32(b.data()+pos+4);pos+=8;
  if(!std::isfinite(level)||n>levelWidth*levelHeight*6||n%3||size_t(n)*12>b.size()-pos)throw std::runtime_error("reflection bounds");
  ReflectionPlane p;p.level=(level-originZ)*scale;
  for(unsigned i=0;i<n;i++){float xyz[3];memcpy(xyz,b.data()+pos+i*12,12);if(!std::isfinite(xyz[0])||!std::isfinite(xyz[1])||xyz[0]<0||xyz[0]>levelWidth-1||xyz[1]<0||xyz[1]>levelHeight-1||!std::isfinite(xyz[2])||fabsf(xyz[2])>128||(!grouped&&xyz[2]!=level))throw std::runtime_error("reflection vertex");p.water.push_back({mythWorldX(xyz[0],originX,scale),(xyz[2]-originZ)*scale+.004f,(xyz[1]-originY)*scale,0,0});}pos+=size_t(n)*12;
  waterShoreDistances(p.water);
  // Fixed-size spatial tiles, independent of level dimensions and source ordering.
  std::map<std::pair<int,int>,std::vector<Vertex>> tiles;
  const float tileSize=8*scale;
  for(size_t i=0;i<p.water.size();i+=3){auto&a=p.water[i];auto&b=p.water[i+1];auto&c=p.water[i+2];auto key=std::make_pair((int)floorf((a.x+b.x+c.x)/(3*tileSize)),(int)floorf((a.z+b.z+c.z)/(3*tileSize)));auto&t=tiles[key];t.insert(t.end(),p.water.begin()+i,p.water.begin()+i+3);}
  p.water.clear();
  for(auto&entry:tiles){auto&t=entry.second;ReflectionBounds bounds{{1e9f,1e9f,1e9f},{-1e9f,-1e9f,-1e9f},p.water.size(),t.size()};for(auto&v:t){float xyz[]={v.x,v.y,v.z};for(int j=0;j<3;j++){bounds.lo[j]=std::min(bounds.lo[j],xyz[j]);bounds.hi[j]=std::max(bounds.hi[j],xyz[j]);}}p.bounds.push_back(bounds);p.water.insert(p.water.end(),t.begin(),t.end());}
  glGenBuffers(1,&p.vbo);glBindBuffer(GL_ARRAY_BUFFER,p.vbo);glBufferData(GL_ARRAY_BUFFER,p.water.size()*sizeof(Vertex),p.water.data(),GL_STATIC_DRAW);reflectionPlanes.push_back(std::move(p));
 }
 if(pos!=b.size())throw std::runtime_error("reflection trailing data");
 {std::vector<Vertex> joined;for(auto&p:reflectionPlanes)joined.insert(joined.end(),p.water.begin(),p.water.end());auto flow=buildWaterFlow(joined,scale);size_t offset=0,moving=0;for(auto&f:flow)if(hypotf(f[0],f[1])>.01f)moving++;
 for(auto&p:reflectionPlanes){glGenBuffers(1,&p.flowVbo);glBindBuffer(GL_ARRAY_BUFFER,p.flowVbo);glBufferData(GL_ARRAY_BUFFER,p.water.size()*sizeof(flow[0]),flow.data()+offset,GL_STATIC_DRAW);offset+=p.water.size();}LOG("Water flow vertices=%zu moving=%zu",flow.size(),moving);}
 if(!reflectionPlanes.empty())activateReflectionPlane(0);
 const char*vs="#version 300 es\nlayout(location=0) in vec3 pos;uniform mat4 mvp;void main(){gl_Position=mvp*vec4(pos,1);}";
 const char*fs="#version 300 es\nprecision highp float;uniform sampler2D tex;uniform vec4 viewport;out vec4 color;void main(){vec2 uv=(gl_FragCoord.xy-viewport.xy)/viewport.zw;vec4 r=texture(tex,uv);color=r*0.38;}";
 reflectionProgram=glCreateProgram();glAttachShader(reflectionProgram,shader(GL_VERTEX_SHADER,waterVertexShader));glAttachShader(reflectionProgram,shader(GL_FRAGMENT_SHADER,waterFragmentShader));glLinkProgram(reflectionProgram);GLint ok=0;glGetProgramiv(reflectionProgram,GL_LINK_STATUS,&ok);if(!ok)throw std::runtime_error("reflection shader");
 reflectionMaskProgram=glCreateProgram();glAttachShader(reflectionMaskProgram,shader(GL_VERTEX_SHADER,vs));glAttachShader(reflectionMaskProgram,shader(GL_FRAGMENT_SHADER,"#version 300 es\nprecision highp float;out vec4 color;void main(){color=vec4(1);}"));glLinkProgram(reflectionMaskProgram);glGetProgramiv(reflectionMaskProgram,GL_LINK_STATUS,&ok);if(!ok)throw std::runtime_error("reflection mask shader");
 LOG("Water reflection assets triangles=%zu enabled=%d level=%.3f",reflectionWater.size()/3,reflectionEnabled,reflectionLevel);
}
static Mat reflectionMatrix(){return {1,0,0,0,0,-1,0,0,0,0,1,0,0,2*reflectionLevel,0,1};}
static bool visibleReflectionWater(const Mat&m){
 for(const auto&b:reflectionBounds){unsigned outside=63;
  for(unsigned i=0;i<8;i++){float p[]={i&1?b.hi[0]:b.lo[0],i&2?b.hi[1]:b.lo[1],i&4?b.hi[2]:b.lo[2],1},q[4]={};for(int r=0;r<4;r++)for(int c=0;c<4;c++)q[r]+=m[c*4+r]*p[c];unsigned bits=(q[0]<-q[3]?1:0)|(q[0]>q[3]?2:0)|(q[1]<-q[3]?4:0)|(q[1]>q[3]?8:0)|(q[2]<-q[3]?16:0)|(q[2]>q[3]?32:0);outside&=bits;}
  if(!outside)return true;
 }return false;
}
static bool prepareReflectionTarget(int w,int h){
 // High mode uses the exact eye viewport, without downsampling or a 512px cap.
 if(!reflectionHigh){w=std::clamp(w/4,64,512);h=std::clamp(h/4,64,512);}
 GLint maxTexture,maxBuffer;glGetIntegerv(GL_MAX_TEXTURE_SIZE,&maxTexture);glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE,&maxBuffer);
 if(w<1||h<1||w>std::min(maxTexture,maxBuffer)||h>std::min(maxTexture,maxBuffer)){LOG("Reflection size unsupported %dx%d",w,h);reflectionEnabled=false;return false;}
 if(reflectionFBO&&w==reflectionW&&h==reflectionH){glBindFramebuffer(GL_FRAMEBUFFER,reflectionFBO);return true;}
 if(!reflectionFBO){glGenFramebuffers(1,&reflectionFBO);glGenTextures(1,&reflectionTexture);glGenRenderbuffers(1,&reflectionDepth);}
 glBindTexture(GL_TEXTURE_2D,reflectionTexture);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
 glBindRenderbuffer(GL_RENDERBUFFER,reflectionDepth);glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH24_STENCIL8,w,h);glBindFramebuffer(GL_FRAMEBUFFER,reflectionFBO);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,reflectionTexture,0);glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_STENCIL_ATTACHMENT,GL_RENDERBUFFER,reflectionDepth);
 if(glGetError()!=GL_NO_ERROR||glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE){LOG("Reflection target incomplete; disabled");reflectionEnabled=false;return false;}reflectionW=w;reflectionH=h;return true;
}
static void compositeReflections(const Mat&mvp){
 if(!reflectionReady)return;
 GLint viewport[4];glGetIntegerv(GL_VIEWPORT,viewport);glUseProgram(reflectionProgram);glUniformMatrix4fv(glGetUniformLocation(reflectionProgram,"mvp"),1,GL_FALSE,mvp.data());glUniform4f(glGetUniformLocation(reflectionProgram,"viewport"),viewport[0],viewport[1],viewport[2],viewport[3]);glUniform1i(glGetUniformLocation(reflectionProgram,"tex"),0);
 glUniform1i(glGetUniformLocation(reflectionProgram,"waves"),waterWavesEnabled);glUniform1i(glGetUniformLocation(reflectionProgram,"hasSky"),waterSkyTexture!=0);glUniform1f(glGetUniformLocation(reflectionProgram,"waveTime"),float(fmod(waterFrameTime,4096.)));glUniform1f(glGetUniformLocation(reflectionProgram,"worldScale"),scale);glUniform3fv(glGetUniformLocation(reflectionProgram,"eye"),1,waterEye);glUniform3fv(glGetUniformLocation(reflectionProgram,"sun"),1,waterSun);glUniform1f(glGetUniformLocation(reflectionProgram,"skyYaw"),waterSkyYaw);glUniform1f(glGetUniformLocation(reflectionProgram,"fogDensity"),waterFogDensity);
 glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D,waterNormalTexture);glUniform1i(glGetUniformLocation(reflectionProgram,"normalMap"),1);glActiveTexture(GL_TEXTURE2);glBindTexture(GL_TEXTURE_2D,waterSkyTexture);glUniform1i(glGetUniformLocation(reflectionProgram,"sky"),2);glActiveTexture(GL_TEXTURE0);
 glActiveTexture(GL_TEXTURE3);glBindTexture(GL_TEXTURE_2D,waterShadowTexture);glUniform1i(glGetUniformLocation(reflectionProgram,"shadowMap"),3);
 glUniform4f(glGetUniformLocation(reflectionProgram,"shadowUV"),-8.f/(scale*groundEffects.w),8.f/(scale*groundEffects.h),originX*8.f/groundEffects.w,originY*8.f/groundEffects.h);glActiveTexture(GL_TEXTURE0);
 glUniform1i(glGetUniformLocation(reflectionProgram,"sunShadowReady"),waterSunShadowReady);glUniformMatrix4fv(glGetUniformLocation(reflectionProgram,"sunMatrix"),1,GL_FALSE,waterSunMatrix.data());glUniform1f(glGetUniformLocation(reflectionProgram,"sunDepthBias"),waterSunDepthBias);
 glActiveTexture(GL_TEXTURE4);glBindTexture(GL_TEXTURE_2D,waterStaticDepth);glUniform1i(glGetUniformLocation(reflectionProgram,"staticSunDepth"),4);glActiveTexture(GL_TEXTURE0);
 glBindTexture(GL_TEXTURE_2D,reflectionTexture);attributes(reflectionWaterVBO);glBindBuffer(GL_ARRAY_BUFFER,reflectionFlowVBO);glEnableVertexAttribArray(2);glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,2*sizeof(float),0);glEnable(GL_BLEND);glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);glDepthMask(GL_FALSE);glDrawArrays(GL_TRIANGLES,0,(GLsizei)reflectionWater.size());glDepthMask(GL_TRUE);glDisable(GL_BLEND);glDisableVertexAttribArray(2);glUseProgram(program);
}

static rendercull::Rect projectReflectionWater(const Mat&m){
 double start=seconds();rendercull::Rect result;rendercull::Frustum frustum;frustum.set(m,{-1,-1,1,1},-INFINITY);
 for(auto&b:reflectionBounds){waterChunksTested++;rendercull::Bounds bounds;for(int i=0;i<3;i++){bounds.lo[i]=b.lo[i];bounds.hi[i]=b.hi[i];}if(!frustum.visible(bounds))continue;waterChunksVisible++;waterCornersProjected+=8;
 auto r=rendercull::projectedBounds(m,bounds);if(r.valid()){result.add(r.l,r.b);result.add(r.r,r.t);}}
 waterProjectionCPU+=seconds()-start;return result;
}
static void maskReflectionWater(const Mat&normalMVP){
 // Project the actual (including sloping) water, not its proxy reflection height.
 glEnable(GL_STENCIL_TEST);glStencilMask(0xff);glClearStencil(0);glClear(GL_STENCIL_BUFFER_BIT);
 glStencilFunc(GL_ALWAYS,1,0xff);glStencilOp(GL_KEEP,GL_KEEP,GL_REPLACE);
 glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);glDepthMask(GL_FALSE);glDisable(GL_DEPTH_TEST);
 glUseProgram(reflectionMaskProgram);glUniformMatrix4fv(glGetUniformLocation(reflectionMaskProgram,"mvp"),1,GL_FALSE,normalMVP.data());attributes(reflectionWaterVBO);glDrawArrays(GL_TRIANGLES,0,(GLsizei)reflectionWater.size());
 glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glDepthMask(GL_TRUE);glEnable(GL_DEPTH_TEST);glStencilMask(0);glStencilFunc(GL_EQUAL,1,0xff);glStencilOp(GL_KEEP,GL_KEEP,GL_KEEP);glUseProgram(program);
}
// Called after opaque terrain/models, before sprites and overlays. Query drawing
// writes neither color nor depth, and continues even when reflections are skipped.
static void queryReflectionVisibility(const Mat&mvp,unsigned eye){
 if(!reflectionEnabled||reflectionPlanes.empty())return;double start=seconds();GLint oldDepthFunc;glGetIntegerv(GL_DEPTH_FUNC,&oldDepthFunc);
 glUseProgram(reflectionMaskProgram);glUniformMatrix4fv(glGetUniformLocation(reflectionMaskProgram,"mvp"),1,GL_FALSE,mvp.data());glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);glDepthMask(GL_FALSE);glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LEQUAL);glDisable(GL_CULL_FACE);
 for(unsigned i=0;i<reflectionPlanes.size();i++){activateReflectionPlane(i);auto&q=reflectionPlanes[i].visibility;if(q.begin(eye,waterVisibilityFrame)){attributes(reflectionWaterVBO);glDrawArrays(GL_TRIANGLES,0,(GLsizei)reflectionWater.size());q.end();}}
 glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glDepthMask(GL_TRUE);glDepthFunc(oldDepthFunc);glUseProgram(program);waterQueryCPU+=seconds()-start;
}
static void reportReflectionWater(){unsigned issued=0,completed=0,busy=0,skipped=0;for(auto&p:reflectionPlanes){auto&q=p.visibility;issued+=q.issued;completed+=q.completed;busy+=q.busy;skipped+=q.skipped;q.issued=q.completed=q.busy=q.skipped=0;}LOG("Water visibility issued=%u completed=%u busy=%u skipped_eye_passes=%u query_cpu_ms=%.3f",issued,completed,busy,skipped,waterQueryCPU*1000);waterQueryCPU=0;LOG("Reflection water chunks=%u/%u projected_corners=%u projection_ms=%.3f stencil=1 waves=%d",waterChunksVisible,waterChunksTested,waterCornersProjected,waterProjectionCPU*1000,waterWavesEnabled);waterChunksVisible=waterChunksTested=waterCornersProjected=0;waterProjectionCPU=0;}
