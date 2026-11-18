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
  glClearColor(0,0,0,0);glClear(GL_COLOR_BUFFER_BIT);drawParticles({0,0,2},reflected);
  unsigned char pixel[4];glReadPixels(64,64,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);assert(glGetError()==GL_NO_ERROR);return int(pixel[0]);
 };
 for(bool reflected:{false,true}){
  int alive=render(2,9,1,true,reflected),dead=render(4,0,1,true,reflected),scenery=render(1,0,1,true,reflected);
  int fx=render(4,0,0,true,reflected),projectile=render(0,0,1,true,reflected),off=render(4,0,1,false,reflected);
  assert(alive<100&&dead==alive&&scenery==alive&&fx>250&&projectile>250&&off>250);
  printf("PASS reflected=%d live=%d corpse=%d scenery=%d emitter=%d projectile=%d OFF=%d\n",reflected,alive,dead,scenery,fx,projectile,off);
 }
}
