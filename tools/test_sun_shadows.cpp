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
 puts("PASS static sun depth: occlusion, cache unaffected by live snapshots, alpha cutout, GL_NO_ERROR");
}
