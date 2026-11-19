#pragma once
#include <cstdio>
#include <cstdlib>
#include <string>
#include <algorithm>
namespace ui_contrast {
static bool enabled=false,active=false;
static unsigned samples=0,pale=0,yellow=0;
static unsigned color=0xF5B642;
static float red(){return active?((color>>16)&255)/255.f:1.f;}
static float green(){return active?((color>>8)&255)/255.f:1.f;}
static float blue(){return active?(color&255)/255.f:0.f;}
static void load(const char*root){enabled=false;active=false;samples=pale=yellow=0;color=0xF5B642;FILE*c=fopen((std::string(root)+"/auto-ui-contrast.color").c_str(),"rb");if(c){unsigned value;char extra;if(fscanf(c,"%u %c",&value,&extra)==1&&value<=0xFFFFFF)color=value;fclose(c);}FILE*f=fopen((std::string(root)+"/auto-ui-contrast.enabled").c_str(),"rb");if(f){enabled=fgetc(f)=='1';fclose(f);}}
static void sample(unsigned r,unsigned g,unsigned b){
 ++samples;unsigned hi=std::max(r,std::max(g,b)),lo=std::min(r,std::min(g,b));
 float brightness=.2126f*r+.7152f*g+.0722f*b;
 if(brightness>=165&&hi-lo<=65)++pale;
 if(r>=160&&g>=145&&b<=.8f*std::min(r,g)&&std::abs(int(r)-int(g))<=65)++yellow;
}
static void finish(){active=enabled&&samples&&(pale*100ull>=samples*30ull||yellow*100ull>=samples*25ull);}
// Uniform grid samples actual base-shaded terrain colors, once per level.
static void analyze(unsigned w,unsigned h,const unsigned char*indices,const unsigned char*shade,const unsigned char*lut){
 active=false;samples=pale=yellow=0;if(!enabled||!w||!h)return;
 unsigned nx=std::min(w,64u),ny=std::min(h,64u);
 for(unsigned y=0;y<ny;y++)for(unsigned x=0;x<nx;x++){size_t at=size_t((2*y+1)*h/(2*ny))*w+(2*x+1)*w/(2*nx);auto p=lut+(size_t(shade[at])*256+indices[at])*4;if(p[3])sample(p[0],p[1],p[2]);}
 finish();
}
}
