#include "../src/vr/main.cpp"
#include <cassert>
int main(){
 EGLDisplay d=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(d,0,0));EGLint a[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};EGLConfig cfg;EGLint n;assert(eglChooseConfig(d,a,&cfg,1,&n)&&n);EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE},pa[]={EGL_WIDTH,128,EGL_HEIGHT,128,EGL_NONE};auto c=eglCreateContext(d,cfg,EGL_NO_CONTEXT,ca);auto s=eglCreatePbufferSurface(d,cfg,pa);assert(eglMakeCurrent(d,s,s,c));
 glGenVertexArrays(1,&vao);glBindVertexArray(vao);program=glCreateProgram();glAttachShader(program,shader(GL_VERTEX_SHADER,mythVertexShader));glAttachShader(program,shader(GL_FRAGMENT_SHADER,mythFragmentShader));glLinkProgram(program);
 GLuint t;glGenTextures(1,&t);glBindTexture(GL_TEXTURE_2D,t);unsigned char white[]={255,255,255,128};glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,white);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);

 glUseProgram(program);glGenBuffers(1,&vbo);Mat m{};m[0]=m[5]=m[10]=m[15]=1;glUniformMatrix4fv(glGetUniformLocation(program,"mvp"),1,GL_FALSE,m.data());glUniform1i(glGetUniformLocation(program,"tex"),0);glUniform1i(glGetUniformLocation(program,"detailTex"),1);glUniform1i(glGetUniformLocation(program,"detailEnabled"),0);glUniform1i(glGetUniformLocation(program,"softOnly"),0);fogEnabled=false;applyFog(program,false);glViewport(0,0,128,128);



 unsigned char opaque[]={255,255,255,255};glBindTexture(GL_TEXTURE_2D,t);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,opaque);
 waterStaticDepth=spriteLiveDepth=spriteLiveOwner=t;waterSunShadowReady=true;waterSunMatrix={};
 waterSun[0]=0;waterSun[1]=.2;waterSun[2]=-1;enhancedShadowAmount=.45;
 glUniform3f(glGetUniformLocation(program,"fogEye"),0,0,2);
 particleCanonical=true;originX=originY=originZ=0;
 particleTextures.push_back({1,1,{255,255,255,255},{1},t});
 Quad q{};q.col=5;q.views=1;q.seqflags=4;q.flags=4;q.w=q.h=32;q.rx=q.ry=16;q.scale=1;
 particleQuads[{5,0,0,0}]=q;quads[key(5,0,0,0)]=q;textures={t};
 LiveParticle p{};p[0]=17;p[7]=65535;p[8]=p[9]=256;p[11]=0x400000;p[13]=5;p[15]=1;liveParticles={p};
 Object o{};o.col=5;o.scale=1;o.objectKey=17;objects={o};
 particleViewCull.set(m,{-1,-1,1,1},-INFINITY);reflectionCull=particleViewCull;
 auto render=[&](unsigned type,unsigned unit,unsigned kind,bool on,bool reflected){
  objects[0].objectType=type;objects[0].unitID=unit;liveParticles[0][15]=kind;enhancedShadowsEnabled=on;bindSpriteShadows();
  glUniform1f(glGetUniformLocation(program,"reflectionClip"),reflected?1.f:0.f);glUniform1f(glGetUniformLocation(program,"reflectionHeight"),-10.f);
  glClearColor(0,0,0,0);glClear(GL_COLOR_BUFFER_BIT);drawParticles({0,0,2},reflected);
  unsigned char pixel[4];glReadPixels(64,64,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);assert(glGetError()==GL_NO_ERROR);return int(pixel[0]);
 };
 for(bool reflected:{false,true}){
  int alive=render(2,9,1,true,reflected),dead=render(4,0,1,true,reflected),scenery=render(1,0,1,true,reflected);
  int fx=render(4,0,0,true,reflected),projectile=render(0,0,1,true,reflected),off=render(4,0,1,false,reflected);
  assert(alive<100&&dead==alive&&scenery==alive&&fx>250&&projectile>250&&off>250);
  printf("PASS reflected=%d live=%d corpse=%d scenery=%d emitter=%d projectile=%d OFF=%d\n",reflected,alive,dead,scenery,fx,projectile,off);
 }

 // An ordinarily opaque unit must enter the sorted alpha pass while fading.
 quads[key(5,0,0,0)].flags=0;
 for(bool reflected:{false,true}){
  liveParticles[0][7]=65535;int solidSkipped=render(2,9,1,false,reflected);
  liveParticles[0][7]=32768;int half=render(2,9,1,false,reflected);
  liveParticles[0][7]=16384;int quarter=render(2,9,1,false,reflected);
  liveParticles[0][7]=0;int invisible=render(2,9,1,false,reflected);
  fprintf(stderr,"fade diagnostic reflected=%d solid=%d half=%d quarter=%d zero=%d\n",reflected,solidSkipped,half,quarter,invisible);
  assert(solidSkipped==0&&half>120&&half<136&&quarter>56&&quarter<72&&invisible==0);
  printf("PASS teleport fade reflected=%d half=%d quarter=%d invisible=%d\n",reflected,half,quarter,invisible);
 }
 liveParticles[0][7]=65535;
 // The opaque projectile path uses the same owner-bearing stream as units.
 q.flags=0;quads[key(5,0,0,0)]=q;objects[0].objectType=3;objects[0].unitID=0;
 assert(physicalSprite(objects[0])&&spriteLightingOwner(objects[0],0)==1);
 quads[key(5,0,0,0)].flags=4;assert(!physicalSprite(objects[0]));
 physicalDeathSequence[5][0]=true;assert(physicalSprite(objects[0]));
 auto deathFlight=render(3,0,1,true,false),deathRest=render(4,0,1,true,false);assert(deathFlight==deathRest&&deathFlight<100);
 physicalDeathSequence[5][0]=false;objects[0].objectType=3;assert(!physicalSprite(objects[0]));
 printf("PASS metadata-tagged translucent death: flying=%d landed=%d; untagged FX excluded\n",deathFlight,deathRest);
 quads[key(5,0,0,0)].flags=0;objects[0].seq=99;assert(!physicalSprite(objects[0]));objects[0].seq=0;
 SpriteStream solid;std::vector<Vertex> vertices{{-.8f,.8f,0,0,0},{.8f,.8f,0,1,0},{-.8f,-.8f,0,0,1},{.8f,.8f,0,1,0},{.8f,-.8f,0,1,1},{-.8f,-.8f,0,0,1}};
 auto renderSolid=[&](unsigned type,bool shadows,bool light,bool enhanced){
  objects[0].objectType=type;enhancedSpritePath=enhanced;enhancedShadowsEnabled=shadows;
  bindSpriteShadows();glUniform1i(glGetUniformLocation(program,"spriteShadows"),shadows);
  glUniform1i(glGetUniformLocation(program,"spriteLightCount"),light?1:0);
  glUniform4f(glGetUniformLocation(program,"spriteLights[0]"),0,0,2,.01f);
  glUniform4f(glGetUniformLocation(program,"spriteLightColors[0]"),1,.1f,0,1);
  glUniform4f(glGetUniformLocation(program,"spriteLightAxes[0]"),0,0,0,0);
  std::map<std::pair<GLuint,unsigned>,std::vector<Vertex>> batches;
  batches[{t,spriteLightingOwner(objects[0],0)}]=vertices;solid.upload(batches);
  glUniform4f(glGetUniformLocation(program,"tint"),1,1,1,1);glClearColor(0,0,0,0);glClear(GL_COLOR_BUFFER_BIT);solid.draw(false);
  std::array<unsigned char,4> pixel;glReadPixels(64,64,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel.data());assert(glGetError()==GL_NO_ERROR);return pixel;
 };
 auto flying=renderSolid(3,true,false,true),landed=renderSolid(4,true,false,true),plain=renderSolid(3,false,false,true);
 assert(flying==landed&&flying[0]<plain[0]);
 unsigned char gray[]={80,80,80,255};glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,t);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,gray);
 auto lit=renderSolid(3,false,true,true),restLit=renderSolid(4,false,true,true),unlit=renderSolid(3,false,false,true),bypass=renderSolid(3,false,true,false);
 assert(lit==restLit&&lit[0]>unlit[0]&&lit[0]>lit[1]&&bypass==unlit);
 printf("PASS opaque projectile: shadow flying=%u landed=%u OFF=%u; light=%u,%u,%u unlit=%u bypass=%u; blended and missing art excluded\n",flying[0],landed[0],plain[0],lit[0],lit[1],lit[2],unlit[0],bypass[0]);
}
