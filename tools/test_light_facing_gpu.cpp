#include "../src/vr/main.cpp"
#include <cassert>
int main(){
 EGLDisplay d=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(d,0,0));EGLint a[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};EGLConfig cfg;EGLint n;assert(eglChooseConfig(d,a,&cfg,1,&n)&&n);EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE},pa[]={EGL_WIDTH,128,EGL_HEIGHT,128,EGL_NONE};auto c=eglCreateContext(d,cfg,EGL_NO_CONTEXT,ca);auto s=eglCreatePbufferSurface(d,cfg,pa);assert(eglMakeCurrent(d,s,s,c));
 glGenVertexArrays(1,&vao);glBindVertexArray(vao);program=glCreateProgram();glAttachShader(program,shader(GL_VERTEX_SHADER,mythVertexShader));glAttachShader(program,shader(GL_FRAGMENT_SHADER,mythFragmentShader));glLinkProgram(program);
 GLuint t;glGenTextures(1,&t);glBindTexture(GL_TEXTURE_2D,t);unsigned char white[]={255,255,255,128};glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,white);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);

 glUseProgram(program);glGenBuffers(1,&vbo);Mat m{};m[0]=m[5]=m[10]=m[15]=1;glUniformMatrix4fv(glGetUniformLocation(program,"mvp"),1,GL_FALSE,m.data());glUniform1i(glGetUniformLocation(program,"tex"),0);glUniform1i(glGetUniformLocation(program,"detailTex"),1);glUniform1i(glGetUniformLocation(program,"detailEnabled"),0);glUniform1i(glGetUniformLocation(program,"softOnly"),0);fogEnabled=false;applyFog(program,false);glViewport(0,0,128,128);



 GLuint depth;glGenTextures(1,&depth);glBindTexture(GL_TEXTURE_2D,depth);unsigned char whiteDepth[]={255,255,255,255};glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,whiteDepth);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
 waterStaticDepth=spriteLiveDepth=spriteLiveOwner=depth;waterSunShadowReady=true;waterSunMatrix={};waterSun[0]=0;waterSun[1]=.2;waterSun[2]=-1;
 glBindTexture(GL_TEXTURE_2D,t);unsigned char gray[]={128,128,128,255};glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,gray);
 glUniform3f(glGetUniformLocation(program,"fogEye"),0,0,2);enhancedLightingEnabled=false;
 auto render=[&](Vec3 light,bool capsule,bool mirror,bool shadows,bool enabled,bool reflected){
  enhancedShadowsEnabled=shadows;bindSpriteShadows();
  float pos[]={light.x,light.y,light.z,.04f},axis[]={capsule?4.f:0.f,0,0,capsule?1.f/16:0.f},color[]={1,.2f,.05f,1};
  glUniform1i(glGetUniformLocation(program,"spriteLightCount"),enabled?1:0);glUniform4fv(glGetUniformLocation(program,"spriteLights"),1,pos);glUniform4fv(glGetUniformLocation(program,"spriteLightAxes"),1,axis);glUniform4fv(glGetUniformLocation(program,"spriteLightColors"),1,color);
  glUniform1f(glGetUniformLocation(program,"reflectionClip"),reflected?1:0);glUniform1f(glGetUniformLocation(program,"reflectionHeight"),-100);
  float u0=mirror?1:0,u1=1-u0;Vertex aa{-.8f,.8f,0,u0,0},bb{.8f,.8f,0,u1,0},cc{-.8f,-.8f,0,u0,1},dd{.8f,-.8f,0,u1,1},v[]={aa,bb,cc,bb,dd,cc};
  attributes(vbo);glVertexAttrib1f(4,1);glBufferData(GL_ARRAY_BUFFER,sizeof(v),v,GL_STREAM_DRAW);glBindTexture(GL_TEXTURE_2D,t);glUniform4f(glGetUniformLocation(program,"tint"),1,1,1,1);glClear(GL_COLOR_BUFFER_BIT);glDrawArrays(GL_TRIANGLES,0,6);
  std::array<int,2> result{};unsigned char p[4];glReadPixels(32,64,1,1,GL_RGBA,GL_UNSIGNED_BYTE,p);result[0]=p[0];glReadPixels(96,64,1,1,GL_RGBA,GL_UNSIGNED_BYTE,p);result[1]=p[0];assert(glGetError()==GL_NO_ERROR);return result;
 };
 for(bool shadows:{false,true})for(bool reflected:{false,true}){
  auto off=render({0,0,1},false,false,shadows,false,reflected);
  for(bool capsule:{false,true})for(bool mirror:{false,true}){
   auto front=render({capsule?-2.f:0.f,0,1},capsule,mirror,shadows,true,reflected);
   auto back=render({capsule?-2.f:0.f,0,-1},capsule,mirror,shadows,true,reflected);
   assert(back==off);assert(front[0]>off[0]+10&&front[1]>off[1]+10);
  }
 }
 auto side=render({2,0,0},false,false,true,true,false),flipped=render({2,0,0},false,true,true,true,false),off=render({2,0,0},false,false,true,false,false);
 assert(side==flipped&&side[0]==off[0]&&side[1]>off[1]+10);
 puts("PASS point/capsule front vs back; back cannot relieve shadow; shadow OFF; reflected pass; mirrored UV; side-light direction; GL_NO_ERROR");
}
