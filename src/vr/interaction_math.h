#ifndef MYTH_INTERACTION_MATH_H
#define MYTH_INTERACTION_MATH_H
#include <cmath>
#include "map_coordinates.h"
#include <algorithm>
#include <vector>
struct Vec3 {float x,y,z;};
static Vec3 add(Vec3 a,Vec3 b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
static Vec3 sub(Vec3 a,Vec3 b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
static Vec3 times(Vec3 a,float f){return {a.x*f,a.y*f,a.z*f};}
static float dot(Vec3 a,Vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
static Vec3 normalized(Vec3 a){float n=sqrtf(dot(a,a));return n>.0001f?times(a,1/n):Vec3{0,0,-1};}
static Vec3 turn(Vec3 a,float yaw){float c=cosf(yaw),s=sinf(yaw);return {c*a.x+s*a.z,a.y,-s*a.x+c*a.z};}
struct Terrain {
 unsigned width=0,height=0;float ox=0,oy=0,oz=0,scale=1;std::vector<float> heights;
 float rayCeiling=0;bool rayCeilingValid=false;
 void rebuildRayCeiling(){
  rayCeilingValid=false;if(width<2||height<2||heights.size()!=size_t(width)*height||!std::isfinite(scale)||scale<=0)return;
  float highest=-INFINITY;for(float h:heights){if(!std::isfinite(h))return;highest=std::max(highest,(h-oz)*scale);}
  rayCeiling=highest+1.f;rayCeilingValid=std::isfinite(rayCeiling);
 }
 float elevation(float x,float z)const{
  float gx=std::clamp(mythGameX(x,ox,scale),0.f,float(width)-1.001f),gy=std::clamp(z/scale+oy,0.f,float(height)-1.001f);
  unsigned ix=(unsigned)gx,iy=(unsigned)gy;float u=gx-ix,v=gy-iy;
  float a=heights[iy*width+ix],b=heights[iy*width+ix+1],c=heights[(iy+1)*width+ix],d=heights[(iy+1)*width+ix+1];
  float h;if((ix^iy)&1)h=u+v<=1?a+u*(b-a)+v*(c-a):d+(1-u)*(c-d)+(1-v)*(b-d);else h=u>=v?a+u*(b-a)+v*(d-b):a+u*(d-c)+v*(c-a);
  return (h-oz)*scale;
 }
 bool contains(float x,float z)const{float gx=mythGameX(x,ox,scale),gy=z/scale+oy;return gx>=0&&gy>=0&&gx<width-1&&gy<height-1;}
 // Exact vertical ray against this single-valued triangulated height field.
 bool rayDownFromAbove(float x,float z,Vec3&hit)const{
  if(width<2||height<2||heights.size()!=size_t(width)*height||!std::isfinite(scale)||scale<=0||!std::isfinite(x)||!std::isfinite(z)||!contains(x,z))return false;
  if(!rayCeilingValid)return false;
  float startY=rayCeiling,surface=elevation(x,z),distance=startY-surface;
  if(!std::isfinite(startY)||!std::isfinite(surface)||!std::isfinite(distance)||distance<0)return false;
  hit={x,startY-distance,z};return true;
 }
 bool ray(Vec3 p,Vec3 d,Vec3&hit,float&distance)const{
  float previous=0;bool above=false;
  for(float t=0;t<=300;t+=.25f){Vec3 q=add(p,times(d,t));if(!contains(q.x,q.z)){above=false;continue;}float gap=q.y-elevation(q.x,q.z);if(above&&gap<=0){float lo=previous,hi=t;for(int i=0;i<12;i++){float mid=(lo+hi)*.5f;Vec3 m=add(p,times(d,mid));if(m.y>elevation(m.x,m.z))lo=mid;else hi=mid;}distance=(lo+hi)*.5f;hit=add(p,times(d,distance));return true;}above=gap>=0;previous=t;}return false;
 }
};
static float raySphere(Vec3 p,Vec3 d,Vec3 center,float radius){Vec3 v=sub(p,center);float b=dot(v,d),c=dot(v,v)-radius*radius,h=b*b-c;if(h<0)return -1;float t=-b-sqrtf(h);return t>=0?t:-1;}
// Ray against the same vertical billboard rectangle used by the hover frame.
static float rayUnitFrame(Vec3 p,Vec3 d,Vec3 foot,Vec3 normal,float left,float right,float bottom,float top){
 float denom=dot(d,normal);if(fabsf(denom)<.00001f)return -1;
 float t=dot(sub(foot,p),normal)/denom;if(t<0)return -1;
 Vec3 delta=sub(add(p,times(d,t)),foot);float x=dot(delta,{normal.z,0,-normal.x});
 return x>=left&&x<=right&&delta.y>=bottom&&delta.y<=top?t:-1;
}
#endif
