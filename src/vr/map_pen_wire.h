#pragma once
#include <array>
#include <cstring>
#include <cmath>
struct MapPen {
 static constexpr size_t bytes=560;
 std::array<unsigned char,bytes> data{};double updated=0;
 static float number(const unsigned char*p){float f;memcpy(&f,p,4);return f;}
 static bool valid(const unsigned char*p){return u32(p)<=3&&u32(p+4)<=200&&u32(p+8)<=32&&!u32(p+12)&&!u32(p+28)&&std::isfinite(number(p+16))&&std::isfinite(number(p+20))&&number(p+16)>=0&&number(p+16)<=256&&number(p+20)>=0&&number(p+20)<=256;}
 void accept(const unsigned char*p,double now){if(p)memcpy(data.data(),p,bytes);else data.fill(0);updated=now;}
 bool available(double now)const{return (data[0]&1)&&now-updated<3;}
 float rangeX()const{return number(data.data()+16);}float rangeY()const{return number(data.data()+20);}
};
