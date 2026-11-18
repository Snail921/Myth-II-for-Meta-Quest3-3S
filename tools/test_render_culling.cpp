#include "../src/vr/render_culling.h"
#include <cassert>
#include <cstdio>
struct V{float x,y,z;};
int main(){
 using namespace rendercull;
 std::array<float,16> m={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
 Rect full{-1,-1,1,1}; Frustum f;f.set(m,full,-100);
 Bounds in;in.add(-.5,-.5,-.5);in.add(.5,.5,.5);assert(f.visible(in));
 Bounds out;out.add(2,0,0);out.add(3,1,1);assert(!f.visible(out));
 Bounds edge;edge.add(1,0,0);assert(f.visible(edge));
 f.set(m,full,2);assert(!f.visible(in));
 V crossing[]={{-3,-3,0},{3,-3,0},{0,3,0}};auto rect=projectedTriangles(m,crossing,3);assert(rect.valid()&&rect.l==-1&&rect.r==1&&rect.b==-1&&rect.t==1);
 V near[]={{-.5,-.5,-2},{.5,-.5,0},{0,.5,0}};assert(projectedTriangles(m,near,3).valid());
 V behind[]={{0,0,-3},{1,0,-3},{0,1,-3}};assert(!projectedTriangles(m,behind,3).valid());
 f.set(m,Rect{-.25f,-.25f,.25f,.25f},-100);assert(!f.visible(edge));
 // Translation/scale and arbitrary water heights must not introduce map coordinates.
 unsigned checks=0;
 for(float size:{.1f,1.f,100.f,1024.f})for(float origin:{-900.f,0.f,781.f})for(float water:{-8.f,0.f,19.f}){
  auto t=m;t[0]=t[5]=t[10]=1/size;t[12]=-origin/size;t[13]=-water/size;
  f.set(t,full,water);
  for(int x=-12;x<=12;x++)for(int y=-12;y<=12;y++)for(int z=-12;z<=12;z++){
   float px=origin+x*size/10,py=water+y*size/10,pz=z*size/10;Bounds b;b.add(px-.001f,py-.001f,pz-.001f);b.add(px+.001f,py+.001f,pz+.001f);
   bool point=x>=-10&&x<=10&&y>=0&&y<=10&&z>=-10&&z<=10;
   if(point)assert(f.visible(b));checks++;
  }
 }
 // Perspective: points behind the eye disappear, a near-plane crossing remains.
 std::array<float,16> perspective={1,0,0,0,0,1,0,0,0,0,-1.02f,-1,0,0,-.202f,0};
 V back[]={{-.1f,-.1f,1},{.1f,-.1f,1},{0,.1f,1}};
 assert(!projectedTriangles(perspective,back,3).valid());
 V crossingEye[]={{-.1f,-.1f,.1f},{.5f,-.5f,-1},{0,.5f,-1}};
 assert(projectedTriangles(perspective,crossingEye,3).valid());
 f.set(perspective,full,-100);
 for(float z:{-.11f,-1.f,-5.f}){Bounds b;b.add(0,0,z);assert(f.visible(b));}
 Bounds behindEye;behindEye.add(0,0,1);assert(!f.visible(behindEye));
 // Per-eye visibility must not reuse the first eye's decision at the edge.
 auto left=m,right=m;left[12]=-.04f;right[12]=.04f;
 Frustum lf,rf;lf.set(left,full,-INFINITY);rf.set(right,full,-INFINITY);
 Bounds stereoEdge;stereoEdge.add(1.02f,0,0);assert(lf.visible(stereoEdge)&&!rf.visible(stereoEdge));
 Bounds spans;spans.add(-2,0,0);spans.add(2,0,0);assert(lf.visible(spans)&&rf.visible(spans));
 printf("PASS reflection culling: %u cases; crop, edges, clipping, scales, origins, water levels\n",checks);
}
