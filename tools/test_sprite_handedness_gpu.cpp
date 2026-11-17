#include "../src/vr/main.cpp"
#include <cassert>
int main(){
 EGLDisplay d=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(d,0,0));EGLint a[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};EGLConfig cfg;EGLint n;assert(eglChooseConfig(d,a,&cfg,1,&n)&&n);EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE},pa[]={EGL_WIDTH,128,EGL_HEIGHT,128,EGL_NONE};auto c=eglCreateContext(d,cfg,EGL_NO_CONTEXT,ca);auto s=eglCreatePbufferSurface(d,cfg,pa);assert(eglMakeCurrent(d,s,s,c));
 glGenVertexArrays(1,&vao);glBindVertexArray(vao);program=glCreateProgram();glAttachShader(program,shader(GL_VERTEX_SHADER,mythVertexShader));glAttachShader(program,shader(GL_FRAGMENT_SHADER,mythFragmentShader));glLinkProgram(program);
 GLuint t;glGenTextures(1,&t);glBindTexture(GL_TEXTURE_2D,t);unsigned char white[]={255,255,255,128};glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,white);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);

 glUseProgram(program);glGenBuffers(1,&vbo);Mat m{};m[0]=m[5]=m[10]=m[15]=1;glUniformMatrix4fv(glGetUniformLocation(program,"mvp"),1,GL_FALSE,m.data());glUniform1i(glGetUniformLocation(program,"tex"),0);glUniform1i(glGetUniformLocation(program,"detailTex"),1);glUniform1i(glGetUniformLocation(program,"detailEnabled"),0);glUniform1i(glGetUniformLocation(program,"softOnly"),0);fogEnabled=false;applyFog(program,false);glViewport(0,0,128,128);

 particleCanonical=true;originX=originY=originZ=0;
 for(unsigned dir=0;dir<8;dir++){
  std::vector<uint8_t> art(32*32*4,0);for(int y=0;y<32;y++)for(int x=0;x<32;x++){auto at=(y*32+x)*4;art[at+(x<16?0:1)]=(dir+1)*25;art[at+3]=255;}
  GLuint tx;glGenTextures(1,&tx);glBindTexture(GL_TEXTURE_2D,tx);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,32,32,0,GL_RGBA,GL_UNSIGNED_BYTE,art.data());glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
  particleTextures.push_back({32,32,art,std::vector<uint8_t>(1024,1),tx});
  Quad q{};q.col=5;q.views=8;q.seqflags=2;q.flags=4;q.texture=dir;q.rx=q.ry=16;q.w=q.h=32;q.scale=1;q.dir=dir;particleQuads[{5,0,0,dir}]=q;
 }
 LiveParticle p{};p[0]=1;p[7]=65535;p[8]=p[9]=256;p[11]=0x400000;p[13]=5;p[15]=1;liveParticles={p};
 particleViewCull.set(m,{-1,-1,1,1},-INFINITY);reflectionCull=particleViewCull;
 for(bool hdMode:{false,true}){
  if(hdMode)for(unsigned dir=0;dir<8;dir++){
   uint64_t id=(uint64_t(3)<<32)|dir;hd::Entry e{};e.meta[81]=e.meta[83]=32;e.meta[93]=e.meta[95]=16;e.meta[97]=1;hd::entries[id]=e;hd::cache[id]={particleTextures[dir].texture,4096,0};
  }
  for(bool reflected:{false,true})for(float eye:{-.03f,.03f})for(unsigned flip=0;flip<2;flip++){
   liveParticles[0][10]=flip;glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);drawParticles({eye,0,2},reflected);assert(glGetError()==GL_NO_ERROR);
   uint8_t left[4],right[4];glReadPixels(56,64,1,1,GL_RGBA,GL_UNSIGNED_BYTE,left);glReadPixels(72,64,1,1,GL_RGBA,GL_UNSIGNED_BYTE,right);
   printf("mode=%d reflected=%d eye=%f flip=%u L=%u,%u,%u R=%u,%u,%u\n",hdMode,reflected,eye,flip,left[0],left[1],left[2],right[0],right[1],right[2]);fflush(stdout);
   // Normal chooses view 2; mirrored chooses view 6 AND exchanges left/right.
   if(!flip){assert(left[0]>left[1]+5&&right[1]>right[0]+5);assert(left[0]>=16&&left[0]<=20);}
   else {assert(left[1]>left[0]+5&&right[0]>right[1]+5);assert(left[1]>=107&&left[1]<=112);}
  }
 }
 // Opaque sprites use the same angular reflection and retain UV handedness.
 quads.clear();objects.clear();hd::entries.clear();hd::cache.clear();
 for(unsigned dir=0;dir<8;dir++){Quad q=particleQuads.at({5,0,0,dir});q.flags=0;quads[key(5,0,0,dir)]=q;}
 Object o{};o.col=5;o.scale=1;objects={o};
 for(unsigned flip=0;flip<2;flip++){objects[0].rawFlags=flip<<17;beginSpriteFrame();prepareSprites({0,0,2},false);assert(spriteViews[0].sprites.size()==1);auto &sprite=spriteViews[0].sprites[0];assert(sprite.texture==(flip?6u:2u));assert(sprite.vertices[0].u==(flip?1.f:0.f));}
 puts("PASS production handedness: reverse view + mirror UV, standard/HD, both eyes, reflections, opaque path, GLES errors");
}
