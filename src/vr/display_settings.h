#pragma once
#include <cstdio>
#include <string>
#include <algorithm>
struct DisplaySettings {
 unsigned width=0,height=0,hz=72,samples=1;
 static bool valid(unsigned w,unsigned h,unsigned hz,unsigned samples){
  return ((w==0&&h==0)||(w==1680&&h==1760)||(w==1832&&h==1920)||(w==1920&&h==2048)||(w==2064&&h==2208))&&
         (hz==72||hz==80||hz==90||hz==120)&&(samples==1||samples==2);
 }
 static DisplaySettings load(const char*root){DisplaySettings value;FILE*f=fopen((std::string(root)+"/display.settings").c_str(),"r");if(!f)return value;unsigned w,h,hz,samples;char extra;int n=fscanf(f,"%u %u %u %u %c",&w,&h,&hz,&samples,&extra);fclose(f);if(n==4&&valid(w,h,hz,samples))value={w,h,hz,samples};return value;}
};
