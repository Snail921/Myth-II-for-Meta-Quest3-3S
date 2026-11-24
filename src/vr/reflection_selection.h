#pragma once
#include "render_culling.h"
#include <numeric>
// Frame-local broad phase over final billboard bounds (HD extents, flips and
// plane-specific axial geometry included). No guessed size can hide a sprite.
// The leaf predicate is the single selection boundary for future projected LOD.
namespace reflection_selection {
// Plane-vs-AABB rejection is not monotonic under viewport cropping when a
// box crosses the camera plane: different planes can accept different corners.
// Retain such boxes using only near/far/height tests in the initial union.
struct UnionFrustum : rendercull::Frustum {
 float clipW[4]{};
 void set(const std::array<float,16>&m,float height){
  rendercull::Frustum::set(m,{-1,-1,1,1},height);
  for(int j=0;j<4;j++)clipW[j]=m[j*4+3];
 }
 bool visible(const rendercull::Bounds&b)const{
  if(rendercull::Frustum::visible(b))return true;
  float minimum=clipW[3];for(int j=0;j<3;j++)minimum+=clipW[j]*(clipW[j]>=0?b.lo[j]:b.hi[j]);
  if(minimum>.0001f||b.hi[1]<minimumHeight-.0001f)return false;
  for(int i=4;i<6;i++){const auto&p=planes[i];float maximum=p[3];for(int j=0;j<3;j++)maximum+=p[j]*(p[j]>=0?b.hi[j]:b.lo[j]);if(maximum<-.0001f)return false;}
  return true;
 }
};
struct SpatialIndex {
 struct Node {rendercull::Bounds bounds;size_t begin=0,end=0;int left=-1,right=-1;};
 std::vector<Node> nodes;std::vector<size_t> order;
 template<class BoundsAt> int branch(size_t begin,size_t end,const BoundsAt&at){
  int index=int(nodes.size());nodes.push_back({});Node node;node.begin=begin;node.end=end;
  for(size_t i=begin;i<end;i++){const auto&b=at(order[i]);node.bounds.add(b.lo[0],b.lo[1],b.lo[2]);node.bounds.add(b.hi[0],b.hi[1],b.hi[2]);}
  if(end-begin>16){int axis=0;for(int j=1;j<3;j++)if(node.bounds.hi[j]-node.bounds.lo[j]>node.bounds.hi[axis]-node.bounds.lo[axis])axis=j;
   size_t middle=(begin+end)/2;
   std::nth_element(order.begin()+begin,order.begin()+middle,order.begin()+end,[&](size_t a,size_t b){const auto&x=at(a);const auto&y=at(b);float xc=x.lo[axis]+x.hi[axis],yc=y.lo[axis]+y.hi[axis];return xc==yc?a<b:xc<yc;});
   node.left=branch(begin,middle,at);node.right=branch(middle,end,at);
  }
  nodes[index]=node;return index;
 }
 template<class BoundsAt> void build(size_t count,const BoundsAt&at){nodes.clear();order.resize(count);std::iota(order.begin(),order.end(),size_t(0));nodes.reserve(count/4+1);if(count)branch(0,count,at);}
 template<class View,class BoundsAt,class Accept> void query(const View&view,const BoundsAt&at,const Accept&accept,std::vector<size_t>&out,unsigned&tested)const{
  out.clear();if(nodes.empty())return;int stack[64];unsigned used=0;stack[used++]=0;
  while(used){const auto&n=nodes[stack[--used]];if(!view.visible(n.bounds))continue;
   if(n.left>=0){stack[used++]=n.right;stack[used++]=n.left;continue;}
   for(size_t i=n.begin;i<n.end;i++){auto id=order[i];++tested;if(accept(id)&&view.visible(at(id)))out.push_back(id);}
  }
 }
};
inline bool accepts(int candidatePlane,float top,float height,bool unit,int plane,float level,bool full){return (candidatePlane<0||candidatePlane==plane)&&top>=level&&(full||unit||height>=.65f);}
}
