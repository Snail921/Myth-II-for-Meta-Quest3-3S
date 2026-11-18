#pragma once
// android_native_app_glue acknowledges lifecycle commands on this thread.
// Never leave the UI thread waiting while loading level assets.
static android_app* loadingApp=nullptr;
static double loadingPollAt=0;
static void pumpLoadingEvents(bool force=false){
 if(!loadingApp)return;
 timespec ts{};clock_gettime(CLOCK_MONOTONIC,&ts);double now=ts.tv_sec+ts.tv_nsec/1e9;
 if(!force&&now-loadingPollAt<.02)return;loadingPollAt=now;
 int events;android_poll_source*source=nullptr;
 for(int i=0;i<32;i++){
  int id=ALooper_pollOnce(0,nullptr,&events,(void**)&source);if(id<0)break;
  if(source)source->process(loadingApp,source);
  if(loadingApp->destroyRequested)throw std::runtime_error("VR loading cancelled by lifecycle");
 }
 if(loadingApp->destroyRequested)throw std::runtime_error("VR loading cancelled by lifecycle");
}
static void awaitLoadingResume(android_app*a){
 loadingApp=a;loadingPollAt=0;
 while(a->activityState!=APP_CMD_RESUME){
  int events;android_poll_source*source=nullptr;
  int id=ALooper_pollOnce(50,nullptr,&events,(void**)&source);
  if(id>=0&&source)source->process(a,source);
  if(a->destroyRequested)throw std::runtime_error("VR loading cancelled before resume");
 }
 pumpLoadingEvents(true);
}
