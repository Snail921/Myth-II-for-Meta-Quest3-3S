#include "../src/vr/main.cpp"
#include <cassert>
int main(){
 EGLDisplay d=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(d,0,0));EGLint a[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};EGLConfig cfg;EGLint n;assert(eglChooseConfig(d,a,&cfg,1,&n)&&n);EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE},pa[]={EGL_WIDTH,128,EGL_HEIGHT,128,EGL_NONE};auto c=eglCreateContext(d,cfg,EGL_NO_CONTEXT,ca);auto s=eglCreatePbufferSurface(d,cfg,pa);assert(eglMakeCurrent(d,s,s,c));
 glGenVertexArrays(1,&vao);glBindVertexArray(vao);program=glCreateProgram();glAttachShader(program,shader(GL_VERTEX_SHADER,mythVertexShader));glAttachShader(program,shader(GL_FRAGMENT_SHADER,mythFragmentShader));glLinkProgram(program);

 assert(DisplaySettings::valid(1832,1920,120,2));assert(!DisplaySettings::valid(1832,2208,72,1));assert(!DisplaySettings::valid(2064,2208,144,2));assert(!DisplaySettings::valid(2064,2208,90,4));
 const char*root="/data/local/tmp/myth-speed-test";
 {FILE*f=fopen("/data/local/tmp/myth-speed-test/display.settings","w");fputs("2064 2208 90 2\n",f);fclose(f);auto v=DisplaySettings::load(root);assert(v.width==2064&&v.hz==90&&v.samples==2);}
 {FILE*f=fopen("/data/local/tmp/myth-speed-test/display.settings","w");fputs("2064 2208 90 2 garbage",f);fclose(f);auto v=DisplaySettings::load(root);assert(!v.width&&v.hz==72&&v.samples==1);}
 for(GLenum format:{GLenum(GL_RGBA8),GLenum(GL_SRGB8_ALPHA8)})for(auto wh:{std::array<unsigned,2>{1680,1760},{1832,1920},{1920,2048},{2064,2208}}){
  GLuint target,tex;glGenFramebuffers(1,&target);glGenTextures(1,&tex);glBindTexture(GL_TEXTURE_2D,tex);glTexImage2D(GL_TEXTURE_2D,0,format,wh[0],wh[1],0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);glBindFramebuffer(GL_FRAMEBUFFER,target);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,tex,0);assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);
  glViewport(0,0,wh[0],wh[1]);glClearColor(.2,.4,.6,1);glClear(GL_COLOR_BUFFER_BIT);uint8_t expected[4],actual[4];glReadPixels(10,10,1,1,GL_RGBA,GL_UNSIGNED_BYTE,expected);
  EyeMSAA msaa;assert(msaa.create(wh[0],wh[1],format,2));glBindFramebuffer(GL_FRAMEBUFFER,msaa.fbo);GLint samples;glGetIntegerv(GL_SAMPLES,&samples);assert(samples>=2);glClear(GL_COLOR_BUFFER_BIT);
  msaa.resolve(target);glReadPixels(10,10,1,1,GL_RGBA,GL_UNSIGNED_BYTE,actual);for(int i=0;i<4;i++)assert(abs(int(expected[i])-actual[i])<=1);assert(glGetError()==GL_NO_ERROR);
  printf("PASS resolve %ux%u format=%x samples=%d\n",wh[0],wh[1],format,samples);msaa.destroy();glDeleteFramebuffers(1,&target);glDeleteTextures(1,&tex);
 }
 EyeMSAA off;assert(!off.create(64,64,GL_RGBA8,1)&&!off.fbo);
 puts("PASS settings validation/defaults, four resolutions, linear/sRGB MSAA resolve, OFF path");
}
