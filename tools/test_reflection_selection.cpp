#include "../src/vr/reflection_selection.h"
#include <cassert>
#include <cstdio>
#include <random>
struct Candidate {rendercull::Bounds bounds;float top,height;bool unit;int plane;};
int main(){
 std::mt19937 rng(1234);std::uniform_real_distribution<float> position(-100,100),size(.01f,20);
 std::vector<Candidate> candidates;
 for(int i=0;i<8192;i++){float x=position(rng),y=position(rng),z=position(rng),w=size(rng),h=size(rng);Candidate c;c.bounds.add(x-w,y-h,z-w);c.bounds.add(x+w,y+h,z+w);c.top=y+h;c.height=h*2;c.unit=i%3==0;c.plane=i%7==0?i%4:-1;candidates.push_back(c);}
 reflection_selection::SpatialIndex tree;auto at=[&](size_t i)->const rendercull::Bounds&{return candidates[i].bounds;};tree.build(candidates.size(),at);
 unsigned checks=0,tested=0;std::vector<size_t> result,reference;
 for(int turn=0;turn<120;turn++)for(int plane=0;plane<4;plane++)for(bool full:{false,true}){
  float level=plane*7-15.f;std::array<float,16> m={.05f,0,0,0,0,.05f,0,0,0,0,.05f,0,position(rng)*.03f,position(rng)*.03f,position(rng)*.03f,1};
  rendercull::Frustum f;f.set(m,{-.8f,-.9f,.9f,.8f},level);
  auto accept=[&](size_t i){auto&c=candidates[i];return reflection_selection::accepts(c.plane,c.top,c.height,c.unit,plane,level,full);};
  tree.query(f,at,accept,result,tested);std::sort(result.begin(),result.end());reference.clear();for(size_t i=0;i<candidates.size();i++)if(accept(i)&&f.visible(at(i)))reference.push_back(i);assert(result==reference);checks+=candidates.size();
 }
 // Perspective camera-plane crossing: cropped AABB test can accept a box
 // rejected by the full viewport, even though the actual frustum is smaller.
 std::array<float,16> perspective={-.96066925f,0,.27797284f,.27769514f,0,1,0,0,.27769514f,0,.96162992f,.96066925f,0,0,-.1f,0};
 rendercull::Bounds crossing;crossing.add(-1.5554357f,-4.0243903f,-9.565985f);crossing.add(5.6642357f,2.077221f,-1.4290512f);
 rendercull::Frustum fullView,cropped;fullView.set(perspective,{-1,-1,1,1},-INFINITY);cropped.set(perspective,{-.25f,-.25f,.25f,.25f},-INFINITY);
 reflection_selection::UnionFrustum unionView;unionView.set(perspective,-INFINITY);
 assert(!fullView.visible(crossing));assert(cropped.visible(crossing));assert(unionView.visible(crossing));
 for(int turn=0;turn<100;turn++){
  float a=position(rng),c=cosf(a),sn=sinf(a);std::array<float,16> m={c,0,1.001f*sn,sn,0,1,0,0,sn,0,-1.001f*c,-c,0,0,-.1f,0};
  unionView.set(m,-INFINITY);cropped.set(m,{-.25f,-.25f,.25f,.25f},-INFINITY);
  tree.query(unionView,at,[](size_t){return true;},result,tested);std::sort(result.begin(),result.end());
  for(size_t i=0;i<candidates.size();i++)if(cropped.visible(at(i)))assert(std::binary_search(result.begin(),result.end(),i));
 }
 
 // Empty frames, large straddling bounds, exact water/small-object thresholds.
 tree.build(0,at);rendercull::Frustum f;tree.query(f,at,[](size_t){return true;},result,tested);assert(result.empty());
 assert(reflection_selection::accepts(-1,2,.65f,false,0,2,false));assert(!reflection_selection::accepts(-1,1.99f,10,true,0,2,true));assert(!reflection_selection::accepts(1,10,10,true,0,2,true));
 assert(!reflection_selection::accepts(-1,10,.64f,false,0,2,false));assert(reflection_selection::accepts(-1,10,.1f,true,0,2,false));
 printf("PASS: %u brute-force comparisons, %u leaf tests; exact candidates and height/plane policies\n",checks,tested);
}
