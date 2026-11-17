#pragma once
#include "display_settings.h"
// Use an ordinary single-sample OpenXR swapchain and explicitly resolve MSAA.
// This keeps compositor images and diagnostic readback portable.
struct EyeMSAA {
 GLuint fbo=0,color=0,depth=0;unsigned width=0,height=0;
 void destroy(){if(fbo)glDeleteFramebuffers(1,&fbo);if(color)glDeleteRenderbuffers(1,&color);if(depth)glDeleteRenderbuffers(1,&depth);fbo=color=depth=0;}
 bool create(unsigned w,unsigned h,GLenum format,unsigned samples){
  destroy();if(samples<=1)return false;
  for(GLenum f:{format,GLenum(GL_DEPTH_COMPONENT24)}){GLint count=0;glGetInternalformativ(GL_RENDERBUFFER,f,GL_NUM_SAMPLE_COUNTS,1,&count);if(count<=0)return false;std::vector<GLint> values(count);glGetInternalformativ(GL_RENDERBUFFER,f,GL_SAMPLES,count,values.data());if(std::find(values.begin(),values.end(),int(samples))==values.end())return false;}
  width=w;height=h;glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);
  glGenRenderbuffers(1,&color);glBindRenderbuffer(GL_RENDERBUFFER,color);glRenderbufferStorageMultisample(GL_RENDERBUFFER,samples,format,w,h);glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_RENDERBUFFER,color);
  glGenRenderbuffers(1,&depth);glBindRenderbuffer(GL_RENDERBUFFER,depth);glRenderbufferStorageMultisample(GL_RENDERBUFFER,samples,GL_DEPTH_COMPONENT24,w,h);glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,depth);
  bool ok=glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE&&glGetError()==GL_NO_ERROR;
  glBindFramebuffer(GL_FRAMEBUFFER,0);if(!ok){destroy();while(glGetError()!=GL_NO_ERROR){}}return ok;
 }
 void resolve(GLuint destination){if(fbo){glBindFramebuffer(GL_READ_FRAMEBUFFER,fbo);glBindFramebuffer(GL_DRAW_FRAMEBUFFER,destination);glBlitFramebuffer(0,0,width,height,0,0,width,height,GL_COLOR_BUFFER_BIT,GL_NEAREST);}glBindFramebuffer(GL_FRAMEBUFFER,destination);}
};
struct DisplayRuntime {
 DisplaySettings requested;std::string root;float actualHz=0;unsigned width=0,height=0,samples=1;
 PFN_xrEnumerateDisplayRefreshRatesFB enumerate=nullptr;PFN_xrRequestDisplayRefreshRateFB request=nullptr;PFN_xrGetDisplayRefreshRateFB get=nullptr;
 std::vector<float> rates;
 void init(XrInstance instance,XrSession session,bool extension,const char*path){
  root=path;requested=DisplaySettings::load(path);
  if(extension){xrGetInstanceProcAddr(instance,"xrEnumerateDisplayRefreshRatesFB",(PFN_xrVoidFunction*)&enumerate);xrGetInstanceProcAddr(instance,"xrRequestDisplayRefreshRateFB",(PFN_xrVoidFunction*)&request);xrGetInstanceProcAddr(instance,"xrGetDisplayRefreshRateFB",(PFN_xrVoidFunction*)&get);}
  uint32_t count=0;if(enumerate&&XR_SUCCEEDED(enumerate(session,0,&count,nullptr))&&count&&count<100){rates.resize(count);if(XR_FAILED(enumerate(session,count,&count,rates.data())))rates.clear();else rates.resize(count);}
  if(!rates.empty()){std::string tmp=root+"/display-rates.tmp";FILE*f=fopen(tmp.c_str(),"w");if(f){for(float hz:rates)if(hz>=72&&hz<=120)fprintf(f,"%.0f ",hz);fclose(f);rename(tmp.c_str(),(root+"/display-rates").c_str());}}
  LOG("Display requested=%ux%u Hz=%u MSAA=%u refresh_extension=%d",requested.width,requested.height,requested.hz,requested.samples,extension);
  for(float hz:rates)LOG("Display supported Hz=%.2f",hz);
 }
 void applyRate(XrSession session){
  if(request){float hz=float(requested.hz);if(std::find(rates.begin(),rates.end(),hz)==rates.end()){LOG("Display Hz=%u unsupported; using runtime default",requested.hz);hz=0;}
   XrResult result=request(session,hz);LOG("Display refresh request=%.2f result=%d",hz,result);}
  if(get)get(session,&actualHz);report();
 }
 void report(){
  LOG("Display applied=%ux%u Hz=%.2f MSAA=%u",width,height,actualHz,samples);
  std::string tmp=root+"/display-applied.tmp";FILE*f=fopen(tmp.c_str(),"w");if(f){fprintf(f,"%u × %u / %.0f Hz / MSAA %s",width,height,actualHz,samples>1?"2x":"OFF");fclose(f);rename(tmp.c_str(),(root+"/display-applied").c_str());}
 }
};
static bool supportsRefreshExtension(){uint32_t count=0;if(XR_FAILED(xrEnumerateInstanceExtensionProperties(nullptr,0,&count,nullptr)))return false;std::vector<XrExtensionProperties> props(count,{XR_TYPE_EXTENSION_PROPERTIES});if(XR_FAILED(xrEnumerateInstanceExtensionProperties(nullptr,count,&count,props.data())))return false;for(auto&p:props)if(!strcmp(p.extensionName,XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME))return true;return false;}
