#pragma once
#include <array>
#include <cstring>
#include <cmath>
struct MultiplayerHud {
 static constexpr size_t bytes=400+64*132;
 std::array<unsigned char,bytes> data{};
 double updated=0,noticeStarted=0;
 static bool valid(const unsigned char*p){
  if(u32(p)>3||u32(p+4)>64||u32(p+8)>15||u32(p+12)||p[207]||p[271]||p[399])return false;
  for(unsigned i=0;i<u32(p+4);i++)if(p[400+i*132+67]||p[400+i*132+131])return false;
  return true;
 }
 void accept(const unsigned char*p,double now){
  if(!p||!(p[0]&2)||!notice(now)||memcmp(data.data()+272,p+272,128))noticeStarted=now;
  if(p)memcpy(data.data(),p,bytes);else data.fill(0);updated=now;
 }
 bool disconnected(double now)const{
  if(!notice(now))return false;
  const char*text=(const char*)data.data()+272;
  return !strcmp(text,"DISCONNECTED")||!strcmp(text,"Disconnected");
 }
 float noticeAlpha(double now)const{
  if(!disconnected(now))return 1.f;
  double elapsed=std::max(0.,(now-noticeStarted)/2.);return float(elapsed-std::floor(elapsed));
 }
 bool scores(double now)const{return (data[0]&1)&&now-updated<3;}
 bool notice(double now)const{return (data[0]&2)&&data[272]&&now-updated<3;}
};
