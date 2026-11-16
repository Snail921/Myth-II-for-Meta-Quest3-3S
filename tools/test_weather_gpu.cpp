#include "../src/vr/main.cpp"
#include <cassert>
int main(){
 EGLDisplay d=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(d,0,0));EGLint a[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};EGLConfig cfg;EGLint n;assert(eglChooseConfig(d,a,&cfg,1,&n)&&n);EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE},pa[]={EGL_WIDTH,32,EGL_HEIGHT,32,EGL_NONE};auto c=eglCreateContext(d,cfg,EGL_NO_CONTEXT,ca);auto s=eglCreatePbufferSurface(d,cfg,pa);assert(eglMakeCurrent(d,s,s,c));
 glGenVertexArrays(1,&vao);glBindVertexArray(vao);program=glCreateProgram();glAttachShader(program,shader(GL_VERTEX_SHADER,mythVertexShader));glAttachShader(program,shader(GL_FRAGMENT_SHADER,mythFragmentShader));glLinkProgram(program);
 GLuint t;glGenTextures(1,&t);glBindTexture(GL_TEXTURE_2D,t);unsigned char white[]={255,255,255,128};glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,white);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
 std::map<GLuint,std::vector<Vertex>> batches; batches[t]={{-1,-1,0,0,0},{1,-1,0,1,0},{0,1,0,.5,1}};weatherStream.upload(batches);weatherReady=true;weatherDrops=1;weatherNear=5;weatherFar=8;weatherCenter={0,0,0};fogEnabled=false;
 Mat m{};m[0]=m[5]=m[10]=m[15]=1;glViewport(0,0,32,32);
 for(float opacity:{.3f,1.f}){weatherOpacity=opacity;glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);drawWeather(m,false);auto err=glGetError();printf("weather opacity=%.2f error=%u\n",opacity,err);assert(err==GL_NO_ERROR);unsigned char pixel[4];glReadPixels(16,16,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);assert(abs(int(pixel[0])-int(lroundf(128*opacity)))<=2);}
 // Reproduce the old shader sampler collision: a 2D and 2DArray sampler on unit 0.
 glUseProgram(weatherProgram);glUniform1i(glGetUniformLocation(weatherProgram,"detailTex"),0);glDrawArrays(GL_TRIANGLES,0,3);assert(glGetError()==GL_INVALID_OPERATION);
 glUniform1i(glGetUniformLocation(weatherProgram,"detailTex"),1);
 particleCanonical=true;particleTextures.push_back({1,1,{255,255,255,255},{1},t});
 Quad q{};q.col=5;q.seq=1;q.frame=0;q.dir=7;q.views=8;q.flags=4;q.texture=0;q.rx=q.ry=8;q.w=q.h=16;q.scale=1;particleQuads[{5,1,0,7}]=q;
 LiveParticle flake{};flake[0]=1;flake[2]=1;flake[4]=flake[5]=16*512;flake[6]=512;flake[7]=14336u|(18432u<<16);flake[8]=47;flake[9]=4;flake[11]=7u|(7u<<8)|(1536u<<16);flake[13]=5;flake[15]=2;liveParticles={flake};originX=originY=originZ=0;levelWidth=levelHeight=128;weatherCenter={-16,10,16};
 for(bool vr:{false,true}){snowVR=vr;weatherReady=false;drawWeather(m,false);assert(glGetError()==GL_NO_ERROR&&weatherDrops>0&&weatherOpacity==1);float highest=-100;for(auto&v:weatherStream.packed)highest=std::max(highest,v.y);assert(vr?highest>10:highest<2);}
 puts("PASS original snow view 7, original altitude, VR vertical tiling");
 puts("PASS production weather GLES draw: rain/snow alpha, no foreign uniforms, old sampler collision reproduced");
}

