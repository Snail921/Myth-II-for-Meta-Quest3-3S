#include "../src/vr/main.cpp"
#include <cassert>
int main(){
 EGLDisplay display=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(display,0,0));EGLint aa[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_DEPTH_SIZE,24,EGL_NONE};EGLConfig cfg;EGLint n;assert(eglChooseConfig(display,aa,&cfg,1,&n)&&n);
 EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE},pa[]={EGL_WIDTH,512,EGL_HEIGHT,512,EGL_NONE};auto context=eglCreateContext(display,cfg,EGL_NO_CONTEXT,ca);auto surface=eglCreatePbufferSurface(display,cfg,pa);assert(eglMakeCurrent(display,surface,surface,context));glViewport(0,0,512,512);
 program=glCreateProgram();glAttachShader(program,shader(GL_VERTEX_SHADER,mythVertexShader));glAttachShader(program,shader(GL_FRAGMENT_SHADER,mythFragmentShader));glLinkProgram(program);GLint linked;glGetProgramiv(program,GL_LINK_STATUS,&linked);assert(linked);glUseProgram(program);glUniform1i(glGetUniformLocation(program,"tex"),0);glUniform1i(glGetUniformLocation(program,"detailTex"),1);
 glGenVertexArrays(1,&vao);glBindVertexArray(vao);glGenBuffers(1,&vbo);glGenTextures(1,&whiteTexture);glBindTexture(GL_TEXTURE_2D,whiteTexture);unsigned char p[]={255,255,255,255};glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,p);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);wristMapTexture=whiteTexture;


 originX=originY=originZ=0;waterSun[0]=0;waterSun[1]=1;waterSun[2]=0;reflectionEnabled=waterWavesEnabled=true;reflectionPlanes.resize(1);
 Vertex plane[]={{-2,0,-2,0,0},{2,0,-2,1,0},{-2,0,2,0,1},{2,0,-2,1,0},{2,0,2,1,1},{-2,0,2,0,1}};
 glGenBuffers(1,&terrainVBO);glBindBuffer(GL_ARRAY_BUFFER,terrainVBO);glBufferData(GL_ARRAY_BUFFER,sizeof(plane),plane,GL_STATIC_DRAW);terrainCount=6;rendercull::Range range;range.bounds.add(-2,0,-2);range.bounds.add(2,0,2);range.count=6;terrainRanges={range};
 // Fixed opaque roof plus transparent mesh exercise production cutout depth pass.
 std::vector<Vertex> roof;for(auto v:plane){v.y=1;roof.push_back(v);}GLuint roofBuffer;glGenBuffers(1,&roofBuffer);glBindBuffer(GL_ARRAY_BUFFER,roofBuffer);glBufferData(GL_ARRAY_BUFFER,sizeof(plane),roof.data(),GL_STATIC_DRAW);normalModels.push_back({whiteTexture,roofBuffer,6,range.bounds});
 snapshotRevision=1;updateSunShadows();assert(waterSunShadowReady&&sunStaticBuilds==1);updateSunShadows();assert(sunStaticBuilds==1);
 auto sampleProgram=glCreateProgram();const char*vs="#version 300 es\nlayout(location=0) in vec3 pos;void main(){gl_Position=vec4(pos.x,pos.z,0,1);}";const char*fs="#version 300 es\nprecision highp float;uniform sampler2D d;out vec4 c;void main(){float v=texture(d,vec2(.5)).r;c=vec4(v,v,v,1);}";
 glAttachShader(sampleProgram,shader(GL_VERTEX_SHADER,vs));glAttachShader(sampleProgram,shader(GL_FRAGMENT_SHADER,fs));glLinkProgram(sampleProgram);glUseProgram(sampleProgram);glUniform1i(glGetUniformLocation(sampleProgram,"d"),0);
 auto readDepth=[&](GLuint tex){glBindFramebuffer(GL_FRAMEBUFFER,0);glViewport(0,0,512,512);glDisable(GL_DEPTH_TEST);glUseProgram(sampleProgram);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,tex);attributes(terrainVBO);glDrawArrays(GL_TRIANGLES,0,6);unsigned char px[4];glReadPixels(256,256,1,1,GL_RGBA,GL_UNSIGNED_BYTE,px);return px[0];};

 int roofDepth=readDepth(waterStaticDepth);assert(roofDepth<128);
 snapshotRevision++;modelPoses.clear();updateSunShadows();assert(sunStaticBuilds==1&&readDepth(waterStaticDepth)==roofDepth);
 GLuint transparent;glGenTextures(1,&transparent);glBindTexture(GL_TEXTURE_2D,transparent);unsigned char rgba[4]={255,255,255,0};glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,rgba);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);normalModels[0].texture=transparent;waterSunShadowReady=false;updateSunShadows();assert(readDepth(waterStaticDepth)>roofDepth);assert(glGetError()==GL_NO_ERROR);

 // Exercise production receiver shader on dry maps; shadows must not depend on water.
 reflectionEnabled=waterWavesEnabled=false;reflectionPlanes.clear();
 const char* receiverVS=R"(#version 300 es
 layout(location=4) in float ownerAttribute;uniform bool useOwnerAttribute;uniform vec3 testPoint;uniform float testOwner;
 out vec2 tc;out float worldHeight;out vec3 fogPosition;flat out vec3 materialIds;out vec3 materialWeights;flat out float spriteOwner;
 void main(){vec2 v[3]=vec2[3](vec2(-1,-1),vec2(3,-1),vec2(-1,3));gl_Position=vec4(v[gl_VertexID%3],0,1);tc=vec2(.5);worldHeight=testPoint.y;fogPosition=testPoint;materialIds=vec3(0);materialWeights=vec3(0);spriteOwner=useOwnerAttribute?ownerAttribute:testOwner;})";
 GLuint receiver=glCreateProgram();glAttachShader(receiver,shader(GL_VERTEX_SHADER,receiverVS));glAttachShader(receiver,shader(GL_FRAGMENT_SHADER,mythFragmentShader));glLinkProgram(receiver);glGetProgramiv(receiver,GL_LINK_STATUS,&linked);assert(linked);
 auto sample=[&](float x,float y,float z,float owner){program=receiver;glUseProgram(program);bindSpriteShadows();glBindFramebuffer(GL_FRAMEBUFFER,0);glViewport(0,0,512,512);glDisable(GL_DEPTH_TEST);glDisable(GL_BLEND);glUniform1i(glGetUniformLocation(program,"tex"),0);glUniform1i(glGetUniformLocation(program,"detailTex"),1);glUniform4f(glGetUniformLocation(program,"tint"),1,1,1,1);glUniform3f(glGetUniformLocation(program,"testPoint"),x,y,z);glUniform1f(glGetUniformLocation(program,"testOwner"),owner);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,whiteTexture);glDrawArrays(GL_TRIANGLES,0,3);unsigned char pixel[4];glReadPixels(256,256,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);assert(glGetError()==GL_NO_ERROR);return pixel[0];};
 normalModels[0].texture=whiteTexture;waterSunShadowReady=false;updateSunShadows();
 int staticShade=sample(0,.5,0,2),staticLit=sample(0,2,0,2),fx=sample(0,.5,0,0);
 assert(staticShade<staticLit-60&&staticLit>250&&fx>250);
 // Move the roof away and introduce a sun-facing sprite. Different owner receives
 // its shadow; same owner is excluded, and glow/FX with owner=0 stays untouched.
 normalModels.clear();waterSun[0]=0;waterSun[1]=.6;waterSun[2]=1;waterSunShadowReady=false;
 for(auto&v:plane)v.y=-5;glBindBuffer(GL_ARRAY_BUFFER,terrainVBO);glBufferData(GL_ARRAY_BUFFER,sizeof(plane),plane,GL_STATIC_DRAW);
 Quad q{};q.col=1;q.views=1;q.w=q.h=32;q.rx=q.ry=16;q.scale=3.2;q.texture=0;q.flags=4;quads[key(1,0,0,0)]=q;textures={whiteTexture};
 Object unit{};unit.col=1;unit.unitID=0;unit.objectType=4;unit.objectKey=17;unit.scale=1;objects={unit};snapshotRevision++;updateSunShadows();
 int other=sample(0,-.1,-1,2),self=sample(0,-.1,-1,1),side=sample(3,-.1,-1,2);assert(other<self-60&&self>250&&side>250);
 // Actual streamed owner attribute must survive batching and both-eye reuse.
 glUniform1i(glGetUniformLocation(program,"useOwnerAttribute"),1);glUniform3f(glGetUniformLocation(program,"testPoint"),0,-.1,-1);
 SpriteStream receiverStream;Vertex dummy[6]{};
 for(unsigned owner:{2u,1u}){std::map<std::pair<GLuint,unsigned>,std::vector<Vertex>> batches; batches[{whiteTexture,owner}]={dummy,dummy+6};receiverStream.upload(batches);for(int eye=0;eye<2;eye++){receiverStream.draw();unsigned char pixel[4];glReadPixels(256,256,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);assert(owner==2?pixel[0]<180:pixel[0]>250);}}
 glUniform1i(glGetUniformLocation(program,"useOwnerAttribute"),0);
 enhancedShadowsEnabled=false;unsigned oldLive=spriteShadowBuilds;snapshotRevision++;updateSunShadows();assert(spriteShadowBuilds==oldLive);assert(sample(0,-.1,-1,2)>250);enhancedShadowsEnabled=true;updateSunShadows();assert(sample(0,-.1,-1,2)<180);
 // Camera movement cannot alter caster geometry or cause extra live builds.
 camera={13,7,-9};unsigned builds=spriteShadowBuilds;updateSunShadows();assert(spriteShadowBuilds==builds);
 // Fully transparent caster must not shadow another sprite.
 coloredSprites.clear();textures[0]=transparent;snapshotRevision++;updateSunShadows();assert(sample(0,-.1,-1,2)>250);
 objects.clear();snapshotRevision++;updateSunShadows();assert(sample(0,-.1,-1,2)>250);
 // OFF still permits the existing static water shadow pass and restores color writes.
 enhancedShadowsEnabled=false;reflectionEnabled=waterWavesEnabled=true;reflectionPlanes.resize(1);waterSunShadowReady=false;oldLive=spriteShadowBuilds;updateSunShadows();assert(waterSunShadowReady&&spriteShadowBuilds==oldLive);GLboolean masks[4];glGetBooleanv(GL_COLOR_WRITEMASK,masks);assert(masks[0]&&masks[1]&&masks[2]&&masks[3]);
 assert(glGetError()==GL_NO_ERROR);
 printf("Receiver PASS static=%d lit=%d FX=%d other=%d self=%d side=%d\n",staticShade,staticLit,fx,other,self,side);
 puts("PASS static sun depth: occlusion, cache unaffected by live snapshots, alpha cutout, GL_NO_ERROR");
}
