#pragma once
// 1.8.5 detail shadow: light azimuth atan2(100,95), elevation 35 degrees,
// alpha silhouette, height-dependent softening and 0.8*(1-0.4*t)^2 density.
static bool detailShadowsEnabled=false;
static std::map<std::array<uint32_t,4>,unsigned> detailShadowMasks;
static unsigned detailShadowNext=0x40000000,detailShadowBuilds=0,detailShadowPixels=0;static size_t detailShadowBytes=0;
static void loadDetailShadowSetting(android_app*a){detailShadowMasks.clear();detailShadowNext=0x40000000;detailShadowBytes=0;std::ifstream f(std::string(a->activity->internalDataPath)+"/detail-shadow-setting.bin",std::ios::binary);uint32_t on=0;f.read((char*)&on,4);detailShadowsEnabled=on==1;}
static unsigned detailShadow(const Object&o){
 if(!detailShadowsEnabled)return 0;auto base=quads.find(key(o.col,o.seq,o.frame,0));if(base==quads.end())return 0;
 Quad q=base->second;if(q.flags&16)return 0;int views=int(q.views);if(views<1)return 0;
 int dir=(int(std::floor((uint16_t(0x2100-int(std::lround(o.yaw*65536.f)))*views+32768)/65536.0))+views/2)%views;
 auto found=quads.find(key(o.col,o.seq,o.frame,unsigned(dir)));if(!(q.seqflags&4)&&found!=quads.end())q=found->second;
 uint64_t id=q.texture;if((q.flags&1)&&hd::entries.count(id|(1u<<30)))id|=1u<<30;
 auto high=hd::alphas.find(id);bool hires=high!=hd::alphas.end();float worldScale=q.scale*o.scale;uint32_t scaleBits;memcpy(&scaleBits,&worldScale,4);
 std::array<uint32_t,4> cacheKey{q.texture,q.flags,scaleBits,uint32_t(hires)+(uint32_t(id>>30)<<1)};
 auto cached=detailShadowMasks.find(cacheKey);if(cached!=detailShadowMasks.end())return cached->second;
 if(detailShadowBuilds>=4||detailShadowMasks.size()>=8192||q.texture>=spriteIndices.size())return 0;
 unsigned w,h;const std::vector<uint8_t>*alpha;unsigned stride;
 if(hires){auto&m=hd::entries.at(id).meta;float factor=hd::be32(m.data()+96)/65536.f;if(!std::isfinite(factor)||factor<=0||factor>16)return 0;q.scale*=factor;q.w=high->second.w;q.h=high->second.h;q.rx=int16_t(hd::be16(m.data()+92));q.ry=int16_t(hd::be16(m.data()+94));if(q.flags&1){if(id&(1u<<30))q.flags&=~1u;else q.rx=q.w-q.rx;}w=high->second.w;h=high->second.h;alpha=&high->second.pixels;stride=1;}
 else{auto&s=spriteIndices[q.texture];w=s.w;h=s.h;alpha=&s.pairs;stride=2;}
 if(!w||!h)return 0;float ps=q.scale*(10.f/512.f)*o.scale*8.f;
 float left=-q.rx*ps,right=(q.w-q.rx)*ps,top=q.ry*ps,bottom=(q.ry-q.h)*ps;
 constexpr float dx=.6887494619f,dy=.7249994336f,cot=1.4281480067f;
 // Cross(light direction, up) is the horizontal sprite axis.
 auto point=[&](float x,float z){return std::pair<float,float>{dy*x+dx*z*cot,-dx*x+dy*z*cot};};
 auto a=point(left,top),b=point(right,top),c=point(left,bottom),d=point(right,bottom);
 int minx=int(floorf(std::min({a.first,b.first,c.first,d.first})))-1,miny=int(floorf(std::min({a.second,b.second,c.second,d.second})))-1;
 int maxx=int(ceilf(std::max({a.first,b.first,c.first,d.first})))+1,maxy=int(ceilf(std::max({a.second,b.second,c.second,d.second})))+1;
 int mw=maxx-minx+1,mh=maxy-miny+1;if(mw<1||mh<1||mw>256||mh>256||detailShadowPixels+mw*mh>8192||detailShadowBytes+mw*mh>32*1024*1024)return 0;
 surface::Mask mask{0,minx*64,miny*64,mw,mh,std::vector<uint8_t>(size_t(mw)*mh)};
 for(int y=0;y<mh;y++)for(int x=0;x<mw;x++){float gx=minx+x+.5f,gy=miny+y+.5f,lateral=dy*gx-dx*gy,height=(dx*gx+dy*gy)/cot;
  float u=(lateral-left)/(right-left),v=(top-height)/(top-bottom);if(q.flags&1)u=1-u;if(q.flags&2)v=1-v;
  int sx=int(floorf(u*w)),sy=int(floorf(v*h));float fade=std::clamp(std::max(0.f,height)*cot/12.f,0.f,1.f);int radius=std::clamp(int(std::lround(.5f+fade)),1,2);
  unsigned sum=0;for(int j=-radius;j<=radius;j++)for(int i=-radius;i<=radius;i++)if(sx+i>=0&&sx+i<int(w)&&sy+j>=0&&sy+j<int(h))sum+=(*alpha)[((sy+j)*w+sx+i)*stride+stride-1];
  float density=.8f*(1-.4f*fade)*(1-.4f*fade);mask.pixels[y*mw+x]=uint8_t(std::clamp(int(std::lround(sum*density/((radius*2+1)*(radius*2+1)))),0,255));
 }
 detailShadowPixels+=mw*mh;detailShadowBytes+=mw*mh;unsigned key=detailShadowNext++;groundEffects.masks.emplace(key,std::move(mask));detailShadowMasks.emplace(cacheKey,key);detailShadowBuilds++;return key;
}
