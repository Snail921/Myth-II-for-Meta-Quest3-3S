#include "../src/vr/main.cpp"
#include <cassert>
int main(){
 EGLDisplay display=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(display,0,0));EGLint aa[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_DEPTH_SIZE,24,EGL_NONE};EGLConfig cfg;EGLint count;assert(eglChooseConfig(display,aa,&cfg,1,&count)&&count);
 EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE},pa[]={EGL_WIDTH,64,EGL_HEIGHT,64,EGL_NONE};auto context=eglCreateContext(display,cfg,EGL_NO_CONTEXT,ca);auto surface=eglCreatePbufferSurface(display,cfg,pa);assert(eglMakeCurrent(display,surface,surface,context));
 GLuint fbo,tx;glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);glGenTextures(1,&tx);glBindTexture(GL_TEXTURE_2D,tx);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,64,64,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,tx,0);assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);glViewport(0,0,64,64);
 initSkybox();std::vector<unsigned char> pixels(64*32*4);
 for(int y=0;y<32;y++)for(int x=0;x<64;x++){int p=(y*64+x)*4;pixels[p]=y<16?128:0;pixels[p+1]=x<32?0:180;pixels[p+2]=y>=16?255:0;pixels[p+3]=255;}
 uploadSkybox(64,32,pixels.data());skyYaw=0;skyReflectionBlend=1;
 XrView eye{XR_TYPE_VIEW};eye.fov={-.1f,.1f,.1f,-.1f};eye.pose.orientation={sinf(.3f),0,0,cosf(.3f)};
 auto read=[&](bool mirror){glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);drawSkybox(eye,mirror);std::array<unsigned char,4>p;glReadPixels(32,32,1,1,GL_RGBA,GL_UNSIGNED_BYTE,p.data());assert(glGetError()==GL_NO_ERROR);return p;};
 auto normal=read(false),mirror=read(true);assert(normal[0]>50&&normal[0]<60&&normal[2]<3);assert(mirror[0]<3&&mirror[2]>250);assert(normal[3]==255&&mirror[3]==255);
 for(float weight:{0.f,.25f,.5f,1.f}){skyReflectionBlend=weight;auto p=read(true);assert(abs(int(p[2])-int(lroundf(255*weight)))<=1);assert(abs(int(p[3])-int(lroundf(255*weight)))<=1);assert(read(false)==normal);}
 // An opaque reflected object overwrites the sky independently of its blend.
 GLuint obj=glCreateProgram();glAttachShader(obj,shader(GL_VERTEX_SHADER,"#version 300 es\nvoid main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);gl_Position=vec4(p*2.-1.,0,1);}"));
 glAttachShader(obj,shader(GL_FRAGMENT_SHADER,"#version 300 es\nprecision highp float;out vec4 c;void main(){c=vec4(.2,.4,.6,1);}"));glLinkProgram(obj);
 for(float weight:{0.f,.5f,1.f}){skyReflectionBlend=weight;read(true);glUseProgram(obj);glBindVertexArray(skyVAO);glDisable(GL_DEPTH_TEST);glDrawArrays(GL_TRIANGLES,0,3);std::array<unsigned char,4>p;glReadPixels(32,32,1,1,GL_RGBA,GL_UNSIGNED_BYTE,p.data());assert(abs(int(p[0])-51)<=1&&abs(int(p[1])-102)<=1&&abs(int(p[2])-153)<=1&&p[3]==255);}
 skyReflectionBlend=1;
 eye.pose.position={1000,-500,777};assert(read(false)==normal);assert(read(true)==mirror);
 eye.pose.orientation={0,sinf(.35f),0,cosf(.35f)};auto turned=read(false);eye.pose.orientation={0,sinf(-.35f),0,cosf(-.35f)};auto opposite=read(false);assert(turned[1]<3&&opposite[1]>100);
 snprintf(skyBlendPath,sizeof(skyBlendPath),"/data/local/tmp/myth-sky-blend-test");
 auto setting=[&](const char*s){FILE*f=fopen(skyBlendPath,"wb");assert(f);fputs(s,f);fclose(f);skyBlendPoll=-1;pollSkyReflectionBlend();};
 for(auto s:{"000","025","050","100"}){setting(s);assert(fabsf(skyReflectionBlend-atoi(s)/100.f)<.0001f);}
 for(auto s:{"101","-01","nan","050x","50",""}){setting(s);assert(skyReflectionBlend==1);}
 unlink(skyBlendPath);
 initSceneFade();assert(sceneFadeProgram);auto generation=rendererGeneration;resetMapRenderer();assert(rendererGeneration==generation+1&&sceneFadeProgram==0&&reflectionPlanes.empty()&&spriteViews[0].stream.buffers[0]==0);initSceneFade();assert(sceneFadeProgram);assert(glGetError()==GL_NO_ERROR);
 puts("PASS renderer recreation resets fade program and per-map GPU caches");
 // Weather uses one uploaded batch for normal and reflected passes.
 particleTextures.clear();particleQuads.clear();particleColors.clear();livePalettes.clear();originX=originY=originZ=0;levelWidth=160;levelHeight=224;
 particleTextures.push_back({1,1,{255,255,255,255},{0},0});Quad rain{123,0,0,0,1,2,16,0,16,4,1,32,8};particleQuads[{123,0,0,0}]=rain;
 liveParticles={LiveParticle{1,123,0,0,256,256,512,14336u|(18432u<<16),192,8,0,5,0,71,uint32_t(-65*65536),2}};
 weatherCenter={0,2,0};weatherReady=false;Mat identity{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};drawWeather(identity,false);assert(weatherDrops>0&&weatherDrops<20000);unsigned cursor=weatherStream.cursor;GLuint buffer=weatherStream.buffers[cursor];assert(buffer);drawWeather(identity,true);assert(weatherStream.cursor==cursor&&weatherStream.buffers[cursor]==buffer);assert(glGetError()==GL_NO_ERROR);
 puts("PASS weather shader, bounded repeated original rain geometry, shared normal/reflection buffer");
 snprintf(weatherSettingPath,sizeof(weatherSettingPath),"/data/local/tmp/myth-weather-mode-test");
 auto mode=[&](const char*text){FILE*f=fopen(weatherSettingPath,"wb");assert(f);fputs(text,f);fclose(f);weatherPoll=-1;pollWeatherSetting();weatherReady=false;};
 weatherCenter={0,100,0};mode("0");drawWeather(identity,false);assert(!weatherVR&&weatherDrops==0);
 mode("1");drawWeather(identity,false);assert(weatherVR&&weatherDrops>0);mode("1x");assert(weatherVR);mode("0");assert(!weatherVR);mode("x");assert(!weatherVR);unlink(weatherSettingPath);
 weatherCenter={0,2,0};weatherVR=false;weatherReady=false;glBindFramebuffer(GL_FRAMEBUFFER,fbo);glViewport(0,0,64,64);glDisable(GL_DEPTH_TEST);glClearColor(0,0,0,0);glClear(GL_COLOR_BUFFER_BIT);drawWeather(identity,false);std::vector<unsigned char> rainPixels(64*64*4);glReadPixels(0,0,64,64,GL_RGBA,GL_UNSIGNED_BYTE,rainPixels.data());unsigned maximum=0;for(size_t i=0;i<rainPixels.size();i+=4)maximum=std::max(maximum,unsigned(rainPixels[i]));assert(maximum>=74&&maximum<=79);
 puts("PASS rain modes: high-altitude VR coverage, original height preserved, strict saved settings, measured 30 percent alpha pixels");
 // Canonical FX03 keys disambiguate identical legacy signatures, including weather.
 particleQuads[{71,0,0,0}]=rain;particleCanonical=true;weatherReady=false;drawWeather(identity,false);assert(weatherDrops>0);particleQuads.erase({71,0,0,0});weatherReady=false;drawWeather(identity,false);assert(weatherDrops==0);particleCanonical=false;
 puts("PASS canonical weather collection lookup has no ambiguous signature fallback");
 // The original panel owns another EGL context on this same display.
 EGLContext panelContext=eglCreateContext(display,cfg,EGL_NO_CONTEXT,ca);assert(panelContext!=EGL_NO_CONTEXT);assert(eglMakeCurrent(display,surface,surface,panelContext));GLuint panelTexture;glGenTextures(1,&panelTexture);glBindTexture(GL_TEXTURE_2D,panelTexture);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
 EGLContext vrContext=eglCreateContext(display,cfg,EGL_NO_CONTEXT,ca);assert(eglMakeCurrent(display,surface,surface,vrContext));assert(eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT));assert(eglDestroyContext(display,vrContext));assert(eglMakeCurrent(display,surface,surface,panelContext));assert(glIsTexture(panelTexture));assert(eglMakeCurrent(display,surface,surface,context));eglDestroyContext(display,panelContext);
 puts("PASS shared EGL display: destroying VR context preserves original panel texture/context");

 clearSkybox();auto black=read(false);assert(black[0]==0&&black[1]==0&&black[2]==0);
 puts("PASS production sky shader: sRGB decode, reflected vertical ray, yaw orientation, no translation parallax, independent premultiplied sky blend and opaque objects, setting validation, missing asset black, GL_NO_ERROR");
}
