#pragma once
#include <vector>
#include <array>
#include <map>
#include <cmath>
#include <algorithm>
// Shared across reflection groups. Only triangle-connected water propagates flow.
// No inferred downhill evidence => no bulk current (standing waves only).
template<class V> static std::vector<std::array<float,2>> buildWaterFlow(const std::vector<V>& v,float unit){
 using P=std::array<float,3>;using F=std::array<float,2>;
 // Original water often consists of flat terraces meeting at identical XZ edges.
 // Weld those seams for inference only; render positions remain untouched.
 std::map<std::array<float,2>,unsigned> ids;std::vector<P> p;std::vector<unsigned> ix,hits;
 for(auto&a:v){std::array<float,2> key{a.x,a.z};auto r=ids.emplace(key,(unsigned)p.size());if(r.second){p.push_back({a.x,a.y,a.z});hits.push_back(1);}else{p[r.first->second][1]+=a.y;hits[r.first->second]++;}ix.push_back(r.first->second);}
 for(size_t i=0;i<p.size();i++)p[i][1]/=hits[i];
 std::vector<std::vector<unsigned>> adj(p.size());std::vector<F> seed(p.size(),F{}),flow(p.size(),F{});std::vector<unsigned> count(p.size());
 for(size_t i=0;i+2<ix.size();i+=3){auto a=ix[i],b=ix[i+1],c=ix[i+2];float x=p[b][0]-p[a][0],z=p[b][2]-p[a][2],y=p[b][1]-p[a][1],X=p[c][0]-p[a][0],Z=p[c][2]-p[a][2],Y=p[c][1]-p[a][1];float det=x*Z-X*z;
  F f{};if(fabsf(det)>unit*unit*1e-8f){f={-(y*Z-Y*z)/det,-(x*Y-X*y)/det};float len=hypotf(f[0],f[1]);if(len>.002f){float speed=std::min(1.f,.3f+len*2.f);f[0]*=speed/len;f[1]*=speed/len;}else f={};}
  for(int j=0;j<3;j++){unsigned u=ix[i+j],w=ix[i+(j+1)%3];adj[u].push_back(w);adj[w].push_back(u);if(f[0]!=0||f[1]!=0){seed[u][0]+=f[0];seed[u][1]+=f[1];count[u]++;}}
 }
 for(size_t i=0;i<p.size();i++){auto&a=adj[i];std::sort(a.begin(),a.end());a.erase(std::unique(a.begin(),a.end()),a.end());if(count[i]){seed[i][0]/=count[i];seed[i][1]/=count[i];flow[i]=seed[i];}}
 // Local connected neighbourhood covariance estimates channel axis, not global map axes.
 std::vector<F> axis(p.size());std::vector<float> narrow(p.size());std::vector<int> seen(p.size(),-1);
 for(unsigned i=0;i<p.size();i++){std::vector<unsigned> q{i};seen[i]=int(i);double xx=0,xz=0,zz=0;unsigned n=0;
  for(size_t j=0;j<q.size();j++){auto a=q[j];float x=p[a][0]-p[i][0],z=p[a][2]-p[i][2];xx+=x*x;xz+=x*z;zz+=z*z;n++;for(auto b:adj[a])if(seen[b]!=int(i)){seen[b]=int(i);float dx=p[b][0]-p[i][0],dz=p[b][2]-p[i][2];if(dx*dx+dz*dz<=36*unit*unit)q.push_back(b);}}
  double total=xx+zz,delta=sqrt((xx-zz)*(xx-zz)+4*xz*xz);float theta=.5f*atan2f(float(2*xz),float(xx-zz));axis[i]={cosf(theta),sinf(theta)};narrow[i]=total>0?std::clamp(float(delta/total)*2.f-.3f,0.f,1.f):0;
 }
 // Screened diffusion: bends follow banks, broad lakes lose directed flow, seeds stay downhill.
 for(int iteration=0;iteration<160;iteration++){auto next=flow;float change=0;
  for(size_t i=0;i<p.size();i++)if(!count[i]&&!adj[i].empty()){F f{};float weights=0;for(auto j:adj[i]){float d=hypotf(p[j][0]-p[i][0],p[j][2]-p[i][2]);float w=1/std::max(d,unit*.01f);f[0]+=flow[j][0]*w;f[1]+=flow[j][1]*w;weights+=w;}f[0]/=weights;f[1]/=weights;float along=f[0]*axis[i][0]+f[1]*axis[i][1];float bend=.5f*narrow[i],keep=.93f+.069f*narrow[i];for(int k=0;k<2;k++){next[i][k]=((1-bend)*f[k]+bend*along*axis[i][k])*keep;change=std::max(change,fabsf(next[i][k]-flow[i][k]));}}
  flow.swap(next);if(change<.0001f)break;
 }
 std::vector<F> result;result.reserve(v.size());for(auto i:ix)result.push_back(flow[i]);return result;
}
