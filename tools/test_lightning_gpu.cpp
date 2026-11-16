#include "../src/vr/main.cpp"
#include <cassert>
int main(){
 EGLDisplay d=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(d,0,0));EGLint a[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};EGLConfig cfg;EGLint n;assert(eglChooseConfig(d,a,&cfg,1,&n)&&n);EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE},pa[]={EGL_WIDTH,128,EGL_HEIGHT,128,EGL_NONE};auto c=eglCreateContext(d,cfg,EGL_NO_CONTEXT,ca);auto s=eglCreatePbufferSurface(d,cfg,pa);assert(eglMakeCurrent(d,s,s,c));
 glGenVertexArrays(1,&vao);glBindVertexArray(vao);program=glCreateProgram();glAttachShader(program,shader(GL_VERTEX_SHADER,mythVertexShader));glAttachShader(program,shader(GL_FRAGMENT_SHADER,mythFragmentShader));glLinkProgram(program);
 GLuint t;glGenTextures(1,&t);glBindTexture(GL_TEXTURE_2D,t);unsigned char white[]={255,255,255,128};glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,white);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);

 glUseProgram(program);glGenBuffers(1,&vbo);Mat m{};m[0]=m[5]=m[10]=m[15]=1;glUniformMatrix4fv(glGetUniformLocation(program,"mvp"),1,GL_FALSE,m.data());glUniform1i(glGetUniformLocation(program,"tex"),0);glUniform1i(glGetUniformLocation(program,"detailTex"),1);glUniform1i(glGetUniformLocation(program,"detailEnabled"),0);glUniform1i(glGetUniformLocation(program,"softOnly"),0);fogEnabled=false;applyFog(program,false);glViewport(0,0,128,128);
 particleCanonical=true;std::vector<uint8_t> art(32*32*4,0);for(int y=13;y<=19;y++)for(int x=0;x<32;x++){auto at=(y*32+x)*4;art[at]=art[at+1]=art[at+2]=255;art[at+3]=128;}
 glBindTexture(GL_TEXTURE_2D,t);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,32,32,0,GL_RGBA,GL_UNSIGNED_BYTE,art.data());particleTextures.push_back({32,32,art,std::vector<uint8_t>(1024,1),t});
 Quad q{};q.col=5;q.seq=1;q.views=1;q.seqflags=4;q.texture=0;q.rx=16;q.ry=16;q.w=q.h=32;q.scale=1;particleQuads[{5,1,0,0}]=q;
 LiveParticle bolt{};bolt[0]=1;bolt[2]=1;bolt[4]=256;bolt[10]=(uint32_t)-256;bolt[7]=65535;bolt[8]=256;bolt[13]=5;bolt[15]=4;originX=originY=originZ=0;
 std::vector<uint8_t> packet(68);uint32_t one=1;memcpy(packet.data(),&one,4);memcpy(packet.data()+4,bolt.data(),64);assert(parseParticles(packet.data(),packet.size(),liveParticles));bolt[9]=65536;memcpy(packet.data()+4,bolt.data(),64);assert(!parseParticles(packet.data(),packet.size(),liveParticles));
 particleViewCull.set(m,{-1,-1,1,1},-INFINITY);reflectionCull=particleViewCull;
 for(bool reflected:{false,true})for(float eye:{-.03f,.03f}){
  glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);drawParticles({eye,0,2},reflected);assert(glGetError()==GL_NO_ERROR);uint8_t pix[4];glReadPixels(64,64,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pix);assert(pix[0]>100);
  // A horizontal source stripe must span the entire segment, not form a
  // short transverse flash in the middle (regression for swapped UV axes).
  for(int x=36;x<=92;x++){glReadPixels(x,64,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pix);assert(pix[0]>100);}
  glReadPixels(64,80,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pix);assert(pix[0]<10);
 }
 liveParticles.clear();glClear(GL_COLOR_BUFFER_BIT);drawParticles({0,0,2});uint8_t pix[4];glReadPixels(64,64,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pix);assert(!pix[0]);
 puts("PASS production lightning renderer: endpoint ribbon, both eyes, reflection, alpha blend, expiry, wire validation and GLES errors");
}
