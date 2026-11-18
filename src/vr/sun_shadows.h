#pragma once
// Cached sun-space depth for fixed meshes. Live model/unit shadows reuse surface stamps.
static bool spriteSkipShadowLookup=false;
static bool enhancedShadowsEnabled=true;static float enhancedShadowAmount=.45f;
static int enhancedShadowSamples=9;static float enhancedShadowLodDistance=0;static XrVector3f shadowLodEye{};
static void loadEnhancedShadows(const char*root){
 spriteSkipShadowLookup=false;std::ifstream skip(std::string(root)+"/diagnostic-skip-sprite-shadow.enabled");char skipValue=0;if(skip.get(skipValue))spriteSkipShadowLookup=skipValue=='1';
 enhancedShadowSamples=9;enhancedShadowLodDistance=0;
 std::ifstream samples(std::string(root)+"/enhanced-shadow-samples");int n=9;if(samples>>n)enhancedShadowSamples=std::clamp(n,1,9);
 std::ifstream lod(std::string(root)+"/enhanced-shadow-lod");int meters=0;if(lod>>meters)enhancedShadowLodDistance=float(std::clamp(meters,0,100));
enhancedShadowAmount=.45f;std::ifstream amount(std::string(root)+"/enhanced-shadow-amount");int value=45;if(amount>>value)enhancedShadowAmount=std::clamp(value,0,100)/100.f;enhancedShadowsEnabled=true;std::ifstream f(std::string(root)+"/enhanced-shadows.enabled");char c=0;if(f.get(c)&&(c=='0'||c=='1'))enhancedShadowsEnabled=c=='1';}
static GLuint sunDepthProgram=0,sunDepthFBO=0;
static unsigned sunStaticBuilds=0;
static GLuint spriteLiveDepth=0,spriteLiveOwner=0,spriteLiveFBO=0;
static uint64_t spriteShadowRevision=~uint64_t(0),sunGeometryKey=0;
static unsigned spriteShadowBuilds=0,spriteCasterDraws=0,spriteCasterCount=0;
struct ShadowVertex {Vertex vertex;float owner;};
static GLuint spriteCasterBuffers[3]{};static unsigned spriteCasterCursor=0;
static std::map<GLuint,std::vector<ShadowVertex>> spriteCasterBatches;
static std::vector<ShadowVertex> spriteCasterVertices;
static const char* sunDepthVS=R"(#version 300 es
layout(location=0) in vec3 pos;layout(location=1) in vec2 uv;layout(location=4) in float shadowOwner;flat out float vertexOwner;
uniform mat4 mvp;out vec2 tc;
void main(){vertexOwner=shadowOwner;tc=uv;gl_Position=mvp*vec4(pos,1.);})";
static const char* sunDepthFS=R"(#version 300 es
precision highp float;uniform sampler2D tex;uniform highp uint casterID;in vec2 tc;flat in float vertexOwner;layout(location=0) out vec4 owner;
void main(){if(texture(tex,tc).a<.5)discard;highp uint id=casterID!=0u?casterID:uint(vertexOwner+.5);owner=vec4(float(id&255u),float((id>>8)&255u),float((id>>16)&255u),255.0)/255.0;})";
static void resetSunShadows(){
 spriteCasterBatches.clear();spriteCasterVertices.clear();for(auto&b:spriteCasterBuffers)b=0;spriteCasterCursor=0;spriteCasterDraws=spriteCasterCount=0;
 sunDepthProgram=sunDepthFBO=waterStaticDepth=0;waterSunShadowReady=false;
 sunStaticBuilds=0;spriteLiveDepth=spriteLiveOwner=spriteLiveFBO=0;spriteShadowRevision=~uint64_t(0);sunGeometryKey=0;spriteShadowBuilds=0;
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
// Sun-facing upright cards, independent of camera and stereo eye. Near-vertical
// light is approximated by the same upright card (no unavailable top-view art).
static unsigned drawSpriteCasters(){
 Vec3 light=normalized({waterSun[0],waterSun[1],waterSun[2]});float len=sqrtf(light.x*light.x+light.z*light.z);
 float dx=len>.001f?light.x/len:0,dz=len>.001f?light.z/len:1,rx=dz,rz=-dx;unsigned count=0;
 for(auto it=spriteCasterBatches.begin();it!=spriteCasterBatches.end();)if(it->second.empty())it=spriteCasterBatches.erase(it);else{it->second.clear();++it;}
 spriteCasterDraws=0;
 for(size_t i=0;i<objects.size();i++){
  const auto&o=objects[i];if(!objectVisible(o)||(o.rawFlags&0x200000))continue;
  auto it=quads.find(key(o.col,o.seq,o.frame,0));if(it==quads.end())continue;Quad q=it->second;
  // Exclude flames/smoke/projectiles; keep blended scenery and corpses as well as units.
  if(!physicalSprite(o)&&blendedSprite(q))continue;
  unsigned flips=(o.rawFlags>>17)&3,dir=myth_object_sprite_direction(dx,dz,o.yaw,q.views,flips);
  if(!(q.seqflags&4)){auto jt=quads.find(key(o.col,o.seq,o.frame,dir));if(jt!=quads.end())q=jt->second;}
  // Shadows only need the source alpha: no recoloring, HD requests or HD LRU
  // touches for off-camera/light-facing frames. Source silhouette is stable.
  if(q.texture>=textures.size())continue;GLuint tex=textures[q.texture];
  float ps=q.scale*(10.f/512.f)*scale*o.scale,l=-q.rx*ps,r=(q.w-q.rx)*ps,b=(q.ry-q.h)*ps,t=q.ry*ps;
  unsigned flags=q.flags;if(flips&1){float v=l;l=-r;r=-v;flags^=1;}if(flips&2){float v=b;b=-t;t=-v;flags^=2;}
  float x=mythWorldX(o.x,originX,scale),y=(o.z-originZ)*scale,z=(o.y-originY)*scale;
  float u0=(flags&1)?1:0,u1=1-u0,v0=(flags&2)?1:0,v1=1-v0;
  Vertex a{x+rx*l,y+t,z+rz*l,u0,v0},bb{x+rx*r,y+t,z+rz*r,u1,v0},c{x+rx*l,y+b,z+rz*l,u0,v1},d{x+rx*r,y+b,z+rz*r,u1,v1};
  auto&batch=spriteCasterBatches[tex];for(auto&v:std::array<Vertex,6>{a,bb,c,bb,d,c})batch.push_back({v,float(i+1)});count++;
 }
 spriteCasterCount=count;spriteCasterVertices.clear();for(auto&b:spriteCasterBatches)spriteCasterVertices.insert(spriteCasterVertices.end(),b.second.begin(),b.second.end());
 if(spriteCasterVertices.empty())return count;
 spriteCasterCursor=(spriteCasterCursor+1)%3;auto&buffer=spriteCasterBuffers[spriteCasterCursor];if(!buffer)glGenBuffers(1,&buffer);
 glBindBuffer(GL_ARRAY_BUFFER,buffer);glBufferData(GL_ARRAY_BUFFER,spriteCasterVertices.size()*sizeof(ShadowVertex),spriteCasterVertices.data(),GL_STREAM_DRAW);
 glEnableVertexAttribArray(0);glEnableVertexAttribArray(1);glEnableVertexAttribArray(4);
 glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(ShadowVertex),(void*)0);glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,sizeof(ShadowVertex),(void*)12);glVertexAttribPointer(4,1,GL_FLOAT,GL_FALSE,sizeof(ShadowVertex),(void*)sizeof(Vertex));
 glUniform1ui(glGetUniformLocation(sunDepthProgram,"casterID"),0);GLint first=0;
 for(auto&b:spriteCasterBatches)if(!b.second.empty()){glBindTexture(GL_TEXTURE_2D,b.first);glDrawArrays(GL_TRIANGLES,first,(GLsizei)b.second.size());first+=(GLint)b.second.size();spriteCasterDraws++;}
 glDisableVertexAttribArray(4);glVertexAttrib1f(4,0);return count;
}
static void bindSpriteShadows(){
 if(!enhancedSpritePath)return;
 SpriteWaitScope total(spriteWait.kind,1);SpriteWaitScope part(spriteWait.kind,2);bindSpriteLighting();part.next(3);
 glUniform1i(glGetUniformLocation(program,"spriteSkipShadowLookup"),spriteSkipShadowLookup);
 glUniform1i(glGetUniformLocation(program,"spriteShadowSamples"),enhancedShadowSamples);
 glUniform1f(glGetUniformLocation(program,"spriteShadowLodDistance"),enhancedShadowLodDistance);
 glUniform3f(glGetUniformLocation(program,"spriteShadowLodEye"),shadowLodEye.x,shadowLodEye.y,shadowLodEye.z);
 glUniform1i(glGetUniformLocation(program,"spriteShadows"),enhancedShadowsEnabled&&waterSunShadowReady&&spriteLiveDepth);
 part.next(4);glUniform1f(glGetUniformLocation(program,"spriteShadowAmount"),enhancedShadowAmount);glUniform3f(glGetUniformLocation(program,"spriteSunDirection"),waterSun[0],waterSun[1],waterSun[2]);
 glUniformMatrix4fv(glGetUniformLocation(program,"spriteSunMatrix"),1,GL_FALSE,waterSunMatrix.data());
 glUniform1f(glGetUniformLocation(program,"spriteShadowBias"),waterSunDepthBias);
 const char*names[]={"spriteStaticDepth","spriteLiveDepth","spriteLiveOwner"};GLuint ids[]={waterStaticDepth,spriteLiveDepth,spriteLiveOwner};
 for(int i=0;i<3;i++){part.next(5+i);glActiveTexture(GL_TEXTURE8+i);glBindTexture(GL_TEXTURE_2D,ids[i]);glUniform1i(glGetUniformLocation(program,names[i]),8+i);}glActiveTexture(GL_TEXTURE0);
}
static void updateSunShadows(){
 if(terrainRanges.empty()||(!enhancedShadowsEnabled&&(!reflectionEnabled||!waterWavesEnabled||reflectionPlanes.empty())))return; // Also required on dry maps and with water disabled.
 uint64_t geometryKey=terrainVBO;for(auto&m:normalModels)geometryKey=geometryKey*1099511628211ull+m.buffer*31ull+m.texture+m.count;
 if(geometryKey!=sunGeometryKey){waterSunShadowReady=false;sunGeometryKey=geometryKey;}
 bool rebuild=!waterSunShadowReady;
 if(!rebuild&&(!enhancedShadowsEnabled||spriteShadowRevision==snapshotRevision))return;
 GLint savedFBO,viewport[4],savedProgram;glGetIntegerv(GL_FRAMEBUFFER_BINDING,&savedFBO);glGetIntegerv(GL_VIEWPORT,viewport);glGetIntegerv(GL_CURRENT_PROGRAM,&savedProgram);
 double started=seconds();glActiveTexture(GL_TEXTURE0);
 if(!sunDepthProgram){sunDepthProgram=glCreateProgram();glAttachShader(sunDepthProgram,shader(GL_VERTEX_SHADER,sunDepthVS));glAttachShader(sunDepthProgram,shader(GL_FRAGMENT_SHADER,sunDepthFS));glLinkProgram(sunDepthProgram);GLint ok;glGetProgramiv(sunDepthProgram,GL_LINK_STATUS,&ok);if(!ok)throw std::runtime_error("sun shadow shader");glGenFramebuffers(1,&sunDepthFBO);allocateSunDepth(waterStaticDepth,2048);}
 if(enhancedShadowsEnabled&&!spriteLiveFBO){glGenFramebuffers(1,&spriteLiveFBO);allocateSunDepth(spriteLiveDepth,2048);glGenTextures(1,&spriteLiveOwner);glBindTexture(GL_TEXTURE_2D,spriteLiveOwner);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,2048,2048,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
 }
 if(rebuild)sunProjection();
 glUseProgram(sunDepthProgram);glUniformMatrix4fv(glGetUniformLocation(sunDepthProgram,"mvp"),1,GL_FALSE,waterSunMatrix.data());glUniform1i(glGetUniformLocation(sunDepthProgram,"tex"),0);glBindVertexArray(vao);glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LESS);glDepthMask(GL_TRUE);glDisable(GL_BLEND);glDisable(GL_CULL_FACE);glDisable(GL_SCISSOR_TEST);glDisable(GL_STENCIL_TEST);glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);glEnable(GL_POLYGON_OFFSET_FILL);glPolygonOffset(1.5f,2.f);
 if(rebuild){sunTarget(waterStaticDepth,2048);attributes(terrainVBO);glBindTexture(GL_TEXTURE_2D,whiteTexture);glDrawArrays(GL_TRIANGLES,0,terrainCount);for(auto&m:normalModels){attributes(m.buffer);glBindTexture(GL_TEXTURE_2D,hdModel(m.texture));glDrawArrays(GL_TRIANGLES,0,m.count);}sunStaticBuilds++;}
 if(enhancedShadowsEnabled){glBindFramebuffer(GL_FRAMEBUFFER,spriteLiveFBO);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,spriteLiveDepth,0);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,spriteLiveOwner,0);GLenum color=GL_COLOR_ATTACHMENT0;glDrawBuffers(1,&color);glReadBuffer(color);if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)throw std::runtime_error("sprite shadow framebuffer");
 glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glViewport(0,0,2048,2048);float zero[4]={};glClearBufferfv(GL_COLOR,0,zero);glClearDepthf(1);glClear(GL_DEPTH_BUFFER_BIT);
 glUniform1ui(glGetUniformLocation(sunDepthProgram,"casterID"),16777215u);drawAnimatedModels(false);
 drawSpriteCasters();spriteShadowRevision=snapshotRevision;spriteShadowBuilds++;}
 glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glDisable(GL_POLYGON_OFFSET_FILL);glBindFramebuffer(GL_FRAMEBUFFER,savedFBO);glViewport(viewport[0],viewport[1],viewport[2],viewport[3]);glUseProgram(savedProgram);glActiveTexture(GL_TEXTURE0);
 waterSunShadowReady=true;
 if(rebuild||spriteShadowBuilds%120==1)LOG("Sprite shadows static=%u live=%u enabled=%d casters=%u draws=%u submission_ms=%.3f",sunStaticBuilds,spriteShadowBuilds,enhancedShadowsEnabled,spriteCasterCount,spriteCasterDraws,(seconds()-started)*1000);
}
