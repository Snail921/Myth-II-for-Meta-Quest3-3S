#pragma once
#include <algorithm>
#include <cstdint>
#include <map>
#include <vector>
#include <stdexcept>
namespace surface {
constexpr int Tile=64;
struct Mask {unsigned kind;int ox,oy,w,h;std::vector<uint8_t> pixels;};
struct Stamp {
 uint64_t source;unsigned mask;int x,y,strength;
 bool operator==(const Stamp&s)const{return mask==s.mask&&x==s.x&&y==s.y&&strength==s.strength;}
};
inline int floor64(int x){return x>=0?x/64:-int((-int64_t(x)+63)/64);}
// Mirrors original 0x4bae80. Carry runs across full stamp, even across dirty tile boundaries.
inline void apply(std::vector<uint8_t>&dst,int dw,int dh,const Mask&m,const Stamp&s,const std::vector<uint8_t>*dirty=nullptr){
 int left=floor64(s.x),top=floor64(s.y),sx=std::max(0,-left),sy=std::max(0,-top);
 int w=std::min(m.w-sx,dw-std::max(left,0)),h=std::min(m.h-sy,dh-std::max(top,0));
 if(w<=0||h<=0)return;
 left=std::max(left,0);top=std::max(top,0);int fx=s.x&63,fy=s.y&63,cols=(dw+Tile-1)/Tile;
 std::vector<int> carry(w,0);
 for(int y=0;y<h;y++){int horizontal=0;for(int x=0;x<w;x++){
  int source=m.pixels[(sy+y)*m.w+sx+x],amount=0,value=0;
  if(m.kind!=2){value=source*s.strength>>16;int part=value*(63-fx);horizontal+=part;int vertical=horizontal*(63-fy);amount=(carry[x]+vertical)>>12;carry[x]=horizontal*63-vertical;horizontal=value*63-part;}
  if(dirty&&!(*dirty)[((top+y)/Tile)*cols+(left+x)/Tile])continue;
  auto&v=dst[(top+y)*dw+left+x];
  if(m.kind==2)v=std::min<int>(v,source);
  else if(m.kind==0){if(value)v=std::min<int>(v,std::max(64,(255-amount)*v>>8));}
  else if(m.kind==1)v=std::min(255,v+amount);
 }}
}
// Original shadow height fade and horizontal displacement (0x4be64d).
inline int place(int height,unsigned kind,int transfer,int phase,int&offset,unsigned alpha=65535){
 offset=0;int strength=65535;
 if(height){int fade;if(height<0)fade=int(int64_t(height)*65536/512);else{int div=kind==1?1024:3072;fade=int(int64_t(height)*65536/div);if(kind==0)offset=int(int64_t(fade)*1536>>16);fade=-fade;}strength=std::min(65535,fade+65536);}
 if(strength<=0)return 0;
 strength=(alpha>>8)*strength>>8;
 phase=(int16_t)phase;
 if(transfer==1)strength=uint16_t((strength>>8)*(~phase>>8));
 else if(transfer==2)strength=uint16_t((strength>>8)*(phase>>8));
 else if(transfer==5)strength=uint16_t((255-((65535-(phase>>1))>>8))*(strength>>8));
 else if(transfer==6)strength=uint16_t((255-(((phase>>1)+32767)>>8))*(strength>>8));
 return strength;
}
struct Compositor {
 int w=0,h=0;std::vector<uint8_t> indices,base,shade,lut,shadow;
 void initShadow(){shadow.assign(size_t(w)*h,255);}
 std::map<unsigned,Mask> masks;std::map<uint64_t,Stamp> previous;
 std::vector<uint8_t> externalDirty,dirty;std::vector<unsigned> tiles;
 void replaceTile(unsigned tile,const uint8_t*color,const uint8_t*light){
  if(w<=0||h<=0||w%256||h%256||tile>=unsigned(w/256*(h/256)))throw std::runtime_error("persistent tile dimensions");
  int x=(tile%(w/256))*256,y=(tile/(w/256))*256;int cols=(w+63)/64;externalDirty.resize(cols*((h+63)/64));
  for(int j=0;j<256;j++){std::copy_n(color+j*256,256,indices.data()+(y+j)*w+x);std::copy_n(light+j*256,256,base.data()+(y+j)*w+x);}
  for(int j=y/64;j<(y+256)/64;j++)for(int i=x/64;i<(x+256)/64;i++)externalDirty[j*cols+i]=1;
 }
 void mark(const Stamp&s){auto it=masks.find(s.mask);if(it==masks.end())return;auto&m=it->second;int x=floor64(s.x),y=floor64(s.y);
  if(x>=w||y>=h||int64_t(x)+m.w<=0||int64_t(y)+m.h<=0)return;
  int cols=(w+Tile-1)/Tile;for(int j=std::max(0,y)/Tile;j<=(std::min(h,y+m.h)-1)/Tile;j++)for(int i=std::max(0,x)/Tile;i<=(std::min(w,x+m.w)-1)/Tile;i++)dirty[j*cols+i]=1;
 }
 bool touches(const Stamp&s)const{auto&m=masks.at(s.mask);int x=floor64(s.x),y=floor64(s.y);if(x>=w||y>=h||int64_t(x)+m.w<=0||int64_t(y)+m.h<=0)return false;int cols=(w+Tile-1)/Tile;
  for(int j=std::max(0,y)/Tile;j<=(std::min(h,y+m.h)-1)/Tile;j++)for(int i=std::max(0,x)/Tile;i<=(std::min(w,x+m.w)-1)/Tile;i++)if(dirty[j*cols+i])return true;return false;
 }
 void compose(const std::vector<Stamp>&stamps){
  if(shadow.size()!=size_t(w)*h)initShadow();
  int cols=(w+Tile-1)/Tile;dirty.assign(cols*((h+Tile-1)/Tile),0);if(externalDirty.size()==dirty.size()){dirty.swap(externalDirty);std::fill(externalDirty.begin(),externalDirty.end(),0);}tiles.clear();std::map<uint64_t,Stamp> next;
  for(auto&s:stamps){next.emplace(s.source,s);auto it=previous.find(s.source);if(it==previous.end()||!(s==it->second)){mark(s);if(it!=previous.end())mark(it->second);}}
  for(auto&old:previous)if(!next.count(old.first))mark(old.second);
  previous.swap(next);
  for(unsigned i=0;i<dirty.size();i++)if(dirty[i]){tiles.push_back(i);int x=(i%cols)*Tile,y=(i/cols)*Tile;for(int j=y;j<std::min(y+Tile,h);j++){std::copy_n(base.data()+j*w+x,std::min(Tile,w-x),shade.data()+j*w+x);std::fill_n(shadow.data()+j*w+x,std::min(Tile,w-x),255);}}
  if(tiles.empty())return;
  for(auto&s:stamps)if(touches(s)){auto&m=masks.at(s.mask);apply(shade,w,h,m,s,&dirty);if(m.kind!=1)apply(shadow,w,h,m,s,&dirty);}
 }
 void rgba(unsigned tile,std::vector<uint8_t>&out)const{
  int cols=(w+Tile-1)/Tile,x=(tile%cols)*Tile,y=(tile/cols)*Tile,tw=std::min(Tile,w-x),th=std::min(Tile,h-y);out.resize(tw*th*4);
  for(int j=0;j<th;j++)for(int i=0;i<tw;i++){int at=(y+j)*w+x+i;auto p=lut.data()+(shade[at]*256+indices[at])*4;std::copy_n(p,4,out.data()+(j*tw+i)*4);}
 }
};
}
