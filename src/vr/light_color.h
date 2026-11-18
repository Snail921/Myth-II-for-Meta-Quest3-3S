#pragma once
#include <array>
#include <algorithm>
#include <cmath>
// Five small patches: center and four offsets. Ignore alpha edges and give
// colored pixels more weight than the white-hot center. Return hue, not power.
template<class Pixel> static std::array<float,3> sampleLightColor(unsigned w,unsigned h,Pixel pixel){
 float sum[3]={},weight=0;const float points[5][2]={{.5,.5},{.3,.5},{.7,.5},{.5,.3},{.5,.7}};
 if(!w||!h)return {1,1,1};
 for(auto&p:points)for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++){
  unsigned x=std::clamp(int(p[0]*(w-1))+dx,0,int(w)-1),y=std::clamp(int(p[1]*(h-1))+dy,0,int(h)-1);auto c=pixel(x,y);
  if(c[3]<32)continue;float hi=std::max({c[0],c[1],c[2]})/255.f,lo=std::min({c[0],c[1],c[2]})/255.f;
  float wt=c[3]/255.f*(.2f+hi-lo)*hi;
  for(int k=0;k<3;k++){float v=c[k]/255.f;sum[k]+=wt*(v<=.04045f?v/12.92f:powf((v+.055f)/1.055f,2.4f));}weight+=wt;
 }
 if(weight<1e-6f)return {1,1,1};float peak=std::max({sum[0],sum[1],sum[2]});if(peak<1e-6f)return {1,1,1};return {sum[0]/peak,sum[1]/peak,sum[2]/peak};
}
