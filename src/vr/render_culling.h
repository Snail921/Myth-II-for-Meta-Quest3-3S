#pragma once
#include <array>
#include <cstddef>
#include <algorithm>
#include <cmath>
#include <limits>
namespace rendercull {
struct Bounds {
 float lo[3]={INFINITY,INFINITY,INFINITY},hi[3]={-INFINITY,-INFINITY,-INFINITY};
 void add(float x,float y,float z){float p[]={x,y,z};for(int i=0;i<3;i++){lo[i]=std::min(lo[i],p[i]);hi[i]=std::max(hi[i],p[i]);}}
};
struct Rect {float l=1,b=1,r=-1,t=-1;bool valid()const{return l<=r&&b<=t;}void add(float x,float y){l=std::min(l,x);r=std::max(r,x);b=std::min(b,y);t=std::max(t,y);}};
struct Frustum {
 float planes[6][4]{};float minimumHeight=-INFINITY;
 void set(const std::array<float,16>&m,Rect rect,float height){
  minimumHeight=height;
  for(int j=0;j<4;j++){planes[0][j]=m[j*4]-rect.l*m[j*4+3];planes[1][j]=rect.r*m[j*4+3]-m[j*4];planes[2][j]=m[j*4+1]-rect.b*m[j*4+3];planes[3][j]=rect.t*m[j*4+3]-m[j*4+1];planes[4][j]=m[j*4+2]+m[j*4+3];planes[5][j]=m[j*4+3]-m[j*4+2];}
 }
 bool visible(const Bounds&b)const{
  if(b.hi[1]<minimumHeight-.0001f)return false;
  for(auto&p:planes){float maximum=p[3];for(int j=0;j<3;j++)maximum+=p[j]*(p[j]>=0?b.hi[j]:b.lo[j]);if(maximum < -.0001f)return false;}return true;
 }
};
// Homogeneous polygon clipping handles the near plane without projecting behind-eye points.
using Clip=std::array<float,4>;
inline float distance(const Clip&p,int plane){return p[3]+((plane&1)?-p[plane/2]:p[plane/2]);}
template<class Vertex> Rect projectedTriangles(const std::array<float,16>&m,const Vertex*vertices,size_t count){
 Rect result;
 for(size_t i=0;i+2<count;i+=3){Clip a[16],b[16];int n=3;
  for(int k=0;k<3;k++){float v[]={vertices[i+k].x,vertices[i+k].y,vertices[i+k].z,1};for(int r=0;r<4;r++){a[k][r]=0;for(int c=0;c<4;c++)a[k][r]+=m[c*4+r]*v[c];}}
  for(int plane=0;plane<6&&n;plane++){int next=0;Clip previous=a[n-1];float dp=distance(previous,plane);
   for(int j=0;j<n;j++){Clip current=a[j];float dc=distance(current,plane);if((dc>=0)!=(dp>=0)){float t=dp/(dp-dc);Clip p;for(int k=0;k<4;k++)p[k]=previous[k]+t*(current[k]-previous[k]);b[next++]=p;}if(dc>=0)b[next++]=current;previous=current;dp=dc;}
   n=next;for(int j=0;j<n;j++)a[j]=b[j];
  }
  for(int j=0;j<n;j++)if(a[j][3]>1e-7f)result.add(std::clamp(a[j][0]/a[j][3],-1.f,1.f),std::clamp(a[j][1]/a[j][3],-1.f,1.f));
 }return result;
}
struct Range {int first=0,count=0;Bounds bounds;};
}
static rendercull::Frustum reflectionCull;
static rendercull::Rect reflectionScreenRect;
static unsigned reflectObjectsTested=0,reflectObjectsDrawn=0,reflectFXTested=0,reflectFXDrawn=0,reflectTerrainTested=0,reflectTerrainDrawn=0,reflectModelsTested=0,reflectModelsDrawn=0;
static double reflectTerrainCPU=0,reflectModelsCPU=0,reflectSpritesCPU=0,reflectFXCPU=0;
