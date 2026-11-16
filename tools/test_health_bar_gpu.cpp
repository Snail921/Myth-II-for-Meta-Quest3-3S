#include "../src/vr/main.cpp"
#include <cassert>
int main(){
 EGLDisplay display=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(display,0,0));EGLint aa[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_DEPTH_SIZE,24,EGL_NONE};EGLConfig cfg;EGLint count;assert(eglChooseConfig(display,aa,&cfg,1,&count)&&count);
 EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE},pa[]={EGL_WIDTH,64,EGL_HEIGHT,64,EGL_NONE};auto context=eglCreateContext(display,cfg,EGL_NO_CONTEXT,ca);auto surface=eglCreatePbufferSurface(display,cfg,pa);assert(eglMakeCurrent(display,surface,surface,context));
 GLuint fbo,tx;glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);glGenTextures(1,&tx);glBindTexture(GL_TEXTURE_2D,tx);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,64,64,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,tx,0);assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);glViewport(0,0,64,64);

 program=glCreateProgram();glAttachShader(program,shader(GL_VERTEX_SHADER,mythVertexShader));glAttachShader(program,shader(GL_FRAGMENT_SHADER,mythFragmentShader));glLinkProgram(program);glUseProgram(program);glGenVertexArrays(1,&vao);glBindVertexArray(vao);glGenBuffers(1,&vbo);
 Mat identity{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};glUniformMatrix4fv(glGetUniformLocation(program,"mvp"),1,GL_FALSE,identity.data());glUniform1i(glGetUniformLocation(program,"tex"),0);
 unsigned char object[80]{};auto ratio=[&](int current,int maximum){int16_t c=current,m=maximum;memcpy(object+66,&c,2);memcpy(object+68,&m,2);return unitHealth(object);};
 assert(ratio(194,561)>0.345f&&ratio(194,561)<0.346f);assert(ratio(-3,100)==0);assert(ratio(200,100)==1);assert(ratio(100,0)==-1);
 assert(showHealthBar(1,.5f,true,false));assert(showHealthBar(1,.5f,false,true));assert(!showHealthBar(1,.5f,false,false));assert(!showHealthBar(0,.5f,true,true));assert(!showHealthBar(1,-1,true,true));
 auto read=[](int y){std::array<unsigned char,4> p;glReadPixels(32,y,1,1,GL_RGBA,GL_UNSIGNED_BYTE,p.data());return p;};
 for(float health:{0.f,.25f,.5f,.75f,1.f}){
 std::vector<Vertex> bars;appendHealthBar(bars,-.2f,-.8f,.8f,.4f,health,[](float x,float y){return Vertex{x,y,0,0,0};});assert(bars.size()==(health==0||health==1?6:12));
 float low=1,high=-1;for(auto&v:bars){low=std::min(low,v.y);high=std::max(high,v.y);}assert(low==-.8f&&high==.8f);
 glClearColor(0,0,0,0);glClear(GL_COLOR_BUFFER_BIT);drawHealthBars(bars);assert(glGetError()==GL_NO_ERROR);auto bottom=read(10),top=read(53);
 if(health>0&&health<1){assert(bottom[0]+bottom[1]>2*(top[0]+top[1]));}
 if(health==1){assert(bottom[1]>=165&&bottom[1]<=167&&bottom[0]==0&&top==bottom);}
 if(health==0){assert(bottom[0]>=45&&bottom[0]<=47&&bottom[1]==0&&top==bottom);}
 if(health==.5f){assert(abs(int(bottom[0])-int(bottom[1]))<=1);assert(abs(int(top[0])-int(top[1]))<=1);}
 }
 puts("PASS actual GLES health bars: fixed full height, lower filled/upper depleted opacity, RGB interpolation, selection/hover gate, invalid max guard");return 0;
}
