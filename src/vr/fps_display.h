#pragma once
#include <cstdio>
#include <string>
// Count submitted stereo frames, independently of diagnostic logging and game speed.
struct FpsDisplay {
 bool enabled=false; double start=-1,last=-1; unsigned frames=0; float value=0; bool ready=false;
 void reset(){start=last=-1;frames=0;value=0;ready=false;}
 void load(const char* root){reset();enabled=false;FILE*f=fopen((std::string(root)+"/fps-display.enabled").c_str(),"rb");if(f){enabled=fgetc(f)=='1';fclose(f);}}
 void frame(double now,bool rendered){
  if(!enabled)return;
  if(!rendered){reset();return;}
  if(last<0||now<=last||now-last>2){reset();start=last=now;return;}
  last=now;++frames;double elapsed=now-start;
  if(elapsed>=.1){value=float(frames/elapsed);ready=true;frames=0;start=now;}
 }
 std::string label()const{char text[32];if(ready)snprintf(text,sizeof(text),"FPS %.1f",value);else snprintf(text,sizeof(text),"FPS --.-");return text;}
};
