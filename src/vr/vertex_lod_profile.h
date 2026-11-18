#pragma once
#include <cmath>
#include <algorithm>
// CPU mirror of the GLSL decision, not a GPU pixel/occlusion query.
namespace vertexlod {
inline float weight(float distance,float radius,float start=20.f){
 if(radius<=0||start<=0)return 0;
 auto smooth=[](float a,float b,float x){float t=std::clamp((x-a)/(b-a),0.f,1.f);return t*t*(3.f-2.f*t);};
 float nearest=distance-radius,extent=radius/std::max(nearest,.001f);
 return smooth(start,start*1.2f,nearest)*(1.f-smooth(.02f,.03f,extent));
}
struct Counts {
 bool sampled=false;unsigned total=0,detail=0,transition=0,vertex=0,nonreceiver=0,near=0,large=0,forced=0;
 void add(float distance,float radius,bool receiver,bool forceDetail=false){
  total++;if(!receiver){nonreceiver++;return;}
  if(forceDetail){detail++;forced++;return;}
  float w=weight(distance,radius);
  if(w>=1)vertex++;else if(w>0)transition++;else {detail++;if(distance-radius<=20)near++;else large++;}
 }
};
}
