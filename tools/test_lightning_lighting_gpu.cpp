#include "../src/vr/main.cpp"
#include <cassert>
int main(){
 EGLDisplay d=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(d,0,0));EGLint a[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};EGLConfig cfg;EGLint n;assert(eglChooseConfig(d,a,&cfg,1,&n)&&n);EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE},pa[]={EGL_WIDTH,128,EGL_HEIGHT,128,EGL_NONE};auto c=eglCreateContext(d,cfg,EGL_NO_CONTEXT,ca);auto s=eglCreatePbufferSurface(d,cfg,pa);assert(eglMakeCurrent(d,s,s,c));
 glGenVertexArrays(1,&vao);glBindVertexArray(vao);program=glCreateProgram();glAttachShader(program,shader(GL_VERTEX_SHADER,mythVertexShader));glAttachShader(program,shader(GL_FRAGMENT_SHADER,mythFragmentShader));glLinkProgram(program);
 GLuint t;glGenTextures(1,&t);glBindTexture(GL_TEXTURE_2D,t);unsigned char white[]={255,255,255,128};glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,white);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);

 glUseProgram(program);glGenBuffers(1,&vbo);Mat m{};m[0]=m[5]=m[10]=m[15]=1;glUniformMatrix4fv(glGetUniformLocation(program,"mvp"),1,GL_FALSE,m.data());glUniform1i(glGetUniformLocation(program,"tex"),0);glUniform1i(glGetUniformLocation(program,"detailTex"),1);glUniform1i(glGetUniformLocation(program,"detailEnabled"),0);glUniform1i(glGetUniformLocation(program,"softOnly"),0);fogEnabled=false;applyFog(program,false);glViewport(0,0,128,128);


 waterStaticDepth=spriteLiveDepth=spriteLiveOwner=t;waterSunShadowReady=enhancedShadowsEnabled=true;
 // White dummy depth makes every point unoccluded; isolate rounded-body shading.
 unsigned char opaque[]={255,255,255,255};glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,t);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,opaque);
 waterSunMatrix={};glUniform3f(glGetUniformLocation(program,"fogEye"),0,0,2);
 float receiverOwner=1;
 auto render=[&](bool mirror,float amount,Vec3 sun,bool on){
  enhancedShadowAmount=amount;enhancedShadowsEnabled=on;waterSun[0]=sun.x;waterSun[1]=sun.y;waterSun[2]=sun.z;bindSpriteShadows();
  float u0=mirror?1:0,u1=1-u0;Vertex a{-.8f,.8f,0,u0,0},b{.8f,.8f,0,u1,0},c{-.8f,-.8f,0,u0,1},dd{.8f,-.8f,0,u1,1},v[]={a,b,c,b,dd,c};
  attributes(vbo);glVertexAttrib1f(4,receiverOwner);glBufferData(GL_ARRAY_BUFFER,sizeof(v),v,GL_STREAM_DRAW);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,t);glUniform4f(glGetUniformLocation(program,"tint"),1,1,1,1);glClear(GL_COLOR_BUFFER_BIT);glDrawArrays(GL_TRIANGLES,0,6);
  std::array<int,2> result{};unsigned char pixel[4];glReadPixels(32,64,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);result[0]=pixel[0];glReadPixels(96,64,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);result[1]=pixel[0];assert(glGetError()==GL_NO_ERROR);return result;
 };

 liveLightsValid=true;LiveLight light{};light.domain=2;light.source=1;light.w=light.h=32;light.alpha=65535;light.pixels.assign(1024,255);
 light.x=0;light.y=512;light.z=0;liveLights={light};originX=originY=originZ=0;camera={0,0,2};snapshotRevision++;
 enhancedLightingEnabled=false;auto dark=render(false,.45,{0,0,-1},true);
 enhancedLightingEnabled=true;enhancedLightingAmount=0;auto zero=render(false,.45,{0,0,-1},true);assert(zero==dark);
 enhancedLightingAmount=.5;auto half=render(false,.45,{0,0,-1},true);
 enhancedLightingAmount=1;auto full=render(false,.45,{0,0,-1},true);
 assert(half[0]>dark[0]&&full[0]>half[0]&&full[0]<=255);
 liveLights[0].x=100*512;snapshotRevision++;auto far=render(false,.45,{0,0,-1},true);assert(far==dark);
 liveLights[0].x=0;liveLights[0].alpha=0;snapshotRevision++;assert(render(false,.45,{0,0,-1},true)==dark);
 liveLights.clear();snapshotRevision++;assert(render(false,.45,{0,0,-1},true)==dark);assert(spriteLights.empty());
 liveLights.assign(256,light);for(unsigned i=0;i<256;i++)liveLights[i].source=i;snapshotRevision++;
 auto many=render(false,.45,{0,0,-1},true);assert(many[0]<=255&&many[0]>=full[0]);GLint count=0;glGetUniformiv(program,glGetUniformLocation(program,"spriteLightCount"),&count);assert(count==8);
 for(int limit:{4,32,64,128}){enhancedLightingLimit=limit;auto value=render(false,.45,{0,0,-1},true);glGetUniformiv(program,glGetUniformLocation(program,"spriteLightCount"),&count);assert(count==limit&&value[0]<=255);}
 double started=seconds();for(int i=0;i<120;i++){snapshotRevision++;render(false,.45,{0,0,-1},true);}glFinish();printf("128 lights / 256 sources / 128x128 synthetic average %.3f ms\n",(seconds()-started)*1000/120);

 assert(render(false,.45,{0,0,-1},false)[0]>250);

 // Source art color, transparent RGB rejection, HD decoded-color lookup.
 particleCanonical=true;particleTextures.clear();particleQuads.clear();
 std::vector<uint8_t> art(21*21*4);for(size_t i=0;i<441;i++){art[i*4]=255;art[i*4+1]=80;art[i*4+2]=0;art[i*4+3]=255;}
 particleTextures.push_back({21,21,art,{},0});Quad q{};q.col=7;q.views=1;particleQuads[{7,0,0,0}]=q;
 LiveParticle p{};p[0]=1;p[13]=7;p[15]=0;liveParticles={p};
 auto orange=sourceLightColor(light);
 LiveLight ground=light;ground.domain=3;ground.source=0x10000;ground.key=7u<<16;
 liveParticles[0][0]=1; // slot zero in the ground-light reference
 assert(physicalSprite(Object{})==false);
 Object corpse{};corpse.objectType=4;assert(physicalSprite(corpse));
assert(orange[0]>.99&&orange[1]<.2&&orange[2]<.01);
 auto transparent=sampleLightColor(1,1,[](unsigned,unsigned){return std::array<unsigned char,4>{0,0,255,0};});assert(transparent[0]==1&&transparent[1]==1);
 hd::cache[uint64_t(3)<<32]={0,0,0,{0,0,1}};assert(sourceLightColor(light)[2]==1&&sourceLightColor(light)[0]==0);hd::cache.clear();
 // Radius includes original height reach, and does not inflate as source rises.
 liveLights={light};spriteLightRevision=~uint64_t(0);prepareSpriteLights();float radius=spriteLights[0].radius,power=spriteLights[0].intensity;assert(radius>2*scale);
 liveLights[0].z=3*512;snapshotRevision++;prepareSpriteLights();assert(spriteLights[0].radius==radius&&spriteLights[0].intensity==power);
 liveLights[0].z=0;snapshotRevision++;
 // Gray material shows hue even with enhanced shadows OFF; FX owner zero stays gray.
 unsigned char gray[]={80,80,80,255};glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,t);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,gray);
 render(false,.45,{0,0,-1},false);unsigned char colored[4];glReadPixels(64,64,1,1,GL_RGBA,GL_UNSIGNED_BYTE,colored);assert(colored[0]>colored[1]+20&&colored[1]>=colored[2]);
 receiverOwner=0;render(false,.45,{0,0,-1},false);unsigned char fx[4];glReadPixels(64,64,1,1,GL_RGBA,GL_UNSIGNED_BYTE,fx);assert(fx[0]==fx[1]&&fx[1]==fx[2]);
 assert(glGetError()==GL_NO_ERROR);printf("PASS source color/HD/alpha/radius/height; local RGB=%u,%u,%u FX=%u,%u,%u\n",colored[0],colored[1],colored[2],fx[0],fx[1],fx[2]);

 // Lightning lights the middle of a long segment, not just its endpoints.
 receiverOwner=1;liveLights.clear();liveParticles.clear();particleQuads[{7,0,0,0}].h=32;particleQuads[{7,0,0,0}].scale=1;
 LiveParticle bolt{};bolt[0]=42;bolt[13]=7;bolt[15]=4;bolt[7]=65535;bolt[8]=256;bolt[5]=bolt[11]=512;bolt[4]=uint32_t(-4*512);bolt[10]=4*512;
 liveParticles={bolt};snapshotRevision++;auto middle=render(false,.45,{0,0,-1},false);assert(spriteLights.size()==1&&spriteLights[0].segment&&middle[0]>30);
 enhancedLightingEnabled=false;auto noBolt=render(false,.45,{0,0,-1},false);assert(middle[0]>noBolt[0]);enhancedLightingEnabled=true;
 liveParticles[0][6]=liveParticles[0][12]=10*512;snapshotRevision++;assert(render(false,.45,{0,0,-1},false)==noBolt);
 liveParticles.clear();snapshotRevision++;assert(render(false,.45,{0,0,-1},false)==noBolt&&spriteLights.empty());
 bolt[4]=uint32_t(-512);bolt[10]=0;auto second=bolt;second[4]=0;second[10]=512;liveParticles={bolt,second};snapshotRevision++;render(false,.45,{0,0,-1},false);assert(spriteLights.size()==1);
 // An unrelated bolt must not merge, and the common budget still caps capsules.
 liveParticles.clear();for(unsigned i=0;i<140;i++){bolt[0]=i;liveParticles.push_back(bolt);}snapshotRevision++;render(false,.45,{0,0,-1},false);glGetUniformiv(program,glGetUniformLocation(program,"spriteLightCount"),&count);assert(count==128);
 liveParticles[0][7]=0;snapshotRevision++;render(false,.45,{0,0,-1},false);assert(spriteLights.size()==139);
 printf("PASS lightning capsule midpoint=%d OFF=%d; range, disappearance, merge, independent bolts, 128 budget, alpha zero\n",middle[0],noBolt[0]);
 printf("PASS lighting OFF=%d zero=%d half=%d full=%d far=%d capped lights=%d; expiration, alpha zero, shadows OFF, GL_NO_ERROR\n",dark[0],zero[0],half[0],full[0],far[0],count);
}
