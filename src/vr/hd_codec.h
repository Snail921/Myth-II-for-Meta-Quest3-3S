#pragma once
#include <vector>
#include <array>
#include <cstdint>
#include <stdexcept>
namespace hd {
inline uint16_t be16(const uint8_t*b){return uint16_t(b[0])<<8|b[1];}
inline uint32_t be32(const uint8_t*b){return uint32_t(be16(b))<<16|be16(b+2);}
struct Pixel {std::array<uint8_t,4> first{},second{};uint8_t alpha=0;}; // RGB + A5H3 for each component
struct Image {unsigned width=0,height=0;std::vector<Pixel> pixels;};
inline Image decode(const std::vector<uint8_t>&b){
 if(b.size()<52)throw std::runtime_error("HD bitmap header");
 unsigned w=be16(b.data()),h=be16(b.data()+2),flags=be16(b.data()+6),encoding=be16(b.data()+16);
 if(!w||!h||w>4096||h>4096||!(flags&16384)||(encoding!=1&&encoding!=2&&encoding!=3))throw std::runtime_error("HD bitmap format");
 size_t p=52+((flags&32)?0:4*(h-1));
 auto byte=[&](){if(p>=b.size())throw std::runtime_error("HD bitmap truncated");return b[p++];};
 Image out{w,h,std::vector<Pixel>(size_t(w)*h)};
 auto pixel=[&](uint8_t state){Pixel v;uint8_t blue=byte(),green=byte(),red=byte();v.first={red,green,blue,state};
  v.alpha=uint8_t((unsigned(state&31)*1053)>>7);
  if(blue&1){blue=byte();green=byte();red=byte();uint8_t a=byte();v.second={red,green,blue,a};}return v;};
 // Original extended 3 becomes native format 12, uploaded as GL_RGB/UNSIGNED_BYTE.
 // Extended 2 is BGRA; its byte order must not be applied to RGB24 images.
 if(encoding==2||encoding==3){for(auto&v:out.pixels){uint8_t first=byte(),green=byte(),third=byte(),a=encoding==3?255:byte();v.first=encoding==3?std::array<uint8_t,4>{first,green,third,0}:std::array<uint8_t,4>{third,green,first,0};v.alpha=a;}return out;}
 size_t n=0;uint8_t state=0;
 while(n<out.pixels.size()){
  unsigned alpha=state&31;
  if(alpha==0||alpha==31){unsigned count=byte();if(count>out.pixels.size()-n)throw std::runtime_error("HD run overflow");
   for(unsigned i=0;i<count;i++)out.pixels[n++]=alpha?pixel(state):Pixel{};state=1;
  }else{state=byte();out.pixels[n++]=(state&31)?pixel(state):Pixel{};}
 }
 return out;
}
// Original 1.8.5 0x44ff10: tagged pixels contain H/V/S, not RGB.
inline uint32_t hueRGB(uint32_t pixel,const uint8_t*control){
 uint32_t saturation=pixel&255,hue=(pixel>>16)&255,value=(pixel>>8)&255,originalValue=value;
 if(control[0]&1){value=(uint32_t(control[6]|control[7]<<8)*value)>>16;
  if(control[0]&2)value=int32_t((value-originalValue)*saturation)>>8, value+=originalValue;
  saturation=(uint32_t(control[4]|control[5]<<8)*saturation)>>16;
  int inverse=int8_t(control[1]);hue=(((hue-control[2])^inverse)+control[3]-inverse)&255;
 }
 hue*=0x60606u;
 uint32_t packed=((((((hue>>24&1)-1)^hue)&0xffffff)*saturation>>8&0xff0000)|saturation);
 packed=(packed^0xff00ffu)*value;unsigned shift=uint8_t(hue>>24)*5;
 return ((packed>>8)&255)<<((0x10884000u>>(shift&31))&31) | (packed>>24)<<((0x1040208u>>(shift&31))&31) | value<<((0x20002110u>>(shift&31))&31);
}
inline std::vector<uint8_t> rgba(const Image&image,const std::array<std::array<uint8_t,8>,8>&controls){
 std::vector<uint8_t> out(image.pixels.size()*4);
 for(size_t i=0;i<image.pixels.size();i++){const Pixel&p=image.pixels[i];
  auto color=[&](const std::array<uint8_t,4>&c){uint32_t v=uint32_t(c[0])<<16|uint32_t(c[1])<<8|c[2];unsigned hue=c[3]>>5;return hue?hueRGB(v,controls[hue-1].data()):v;};
  uint32_t c=color(p.first),second=color(p.second),weight=(p.second[3]&31)*4;
  if(p.second[3]){uint32_t rb=((((second&0xff00ff)-(c&0xff00ff))*weight)>>8)+(c&0xff00ff);uint32_t g=((((second&0xff00)-(c&0xff00))*weight)>>8)+(c&0xff00);c=(rb&0xff00ff)|(g&0xff00);}
  out[i*4]=c>>16;out[i*4+1]=c>>8;out[i*4+2]=c;out[i*4+3]=p.alpha;
 }return out;
}

}
