#include "../src/vr/surface_compositor.h"
#include "../src/vr/surface_updates.h"
#include <cassert>
#include <cstdio>
static std::vector<uint8_t> payload(uint64_t epoch,unsigned tile,uint8_t color,uint8_t shade){std::vector<uint8_t> b(12+surface::PersistentRow);uint32_t count=1;memcpy(b.data(),&epoch,8);memcpy(b.data()+8,&count,4);memcpy(b.data()+12,&tile,4);std::fill(b.begin()+16,b.begin()+16+65536,color);std::fill(b.begin()+16+65536,b.end(),shade);return b;}
int main(){
 surface::Updates q;auto a=payload(1,5,200,120),b=payload(1,6,201,130);assert(q.stage(a.data(),a.size()));assert(q.stage(b.data(),b.size()));assert(q.pending.size()==2);
 auto bad=payload(2,42,0,0);assert(!q.stage(bad.data(),bad.size()));assert(q.epoch==1&&q.pending.size()==2);assert(!q.stage(a.data(),a.size()-1));
 bad=payload(0,0,0,0);assert(!q.stage(bad.data(),bad.size()));
 auto newer=payload(2,5,202,140);assert(q.stage(newer.data(),newer.size()));assert(q.epoch==2&&q.pending.size()==1&&q.pending[5][0]==202);
 surface::Compositor c;c.w=1792;c.h=1536;c.indices.assign(c.w*c.h,10);c.base.assign(c.w*c.h,203);c.shade=c.base;
 c.replaceTile(5,q.pending[5].data(),q.pending[5].data()+65536);c.masks[0]={0,0,0,30,30,std::vector<uint8_t>(900,100)};surface::Stamp shadow{1,0,1290*64,10*64,65279};c.compose({shadow});assert(c.tiles.size()==16);assert(c.indices[10*c.w+1290]==202);assert(c.shade[10*c.w+1290]<140);
 c.compose({});assert(c.shade[10*c.w+1290]==140&&c.indices[10*c.w+1290]==202);assert(c.base[10*c.w+100]==203);
 c.compose({});assert(c.tiles.empty());
 c.indices.assign(c.w*c.h,10);c.base.assign(c.w*c.h,203);c.externalDirty.assign(28*24,1);c.compose({});assert(c.shade==c.base&&c.tiles.size()==28*24);
 surface::Updates second;second.tileLimit=35;auto last=payload(3,34,222,133);assert(second.stage(last.data(),last.size()));auto outside=payload(3,35,0,0);assert(!second.stage(outside.data(),outside.size()));
 surface::Compositor d;d.w=1280;d.h=1792;d.indices.assign(d.w*d.h,0);d.base.assign(d.w*d.h,0);d.replaceTile(34,last.data()+16,last.data()+16+65536);assert(d.indices.back()==222&&d.base.back()==133);assert(d.indices[0]==0);
 surface::Updates large;large.tileLimit=88;auto edge=payload(4,87,123,231);assert(large.stage(edge.data(),edge.size()));auto overflow=payload(4,88,0,0);assert(!large.stage(overflow.data(),overflow.size()));
 surface::Compositor e;e.w=2816;e.h=2048;e.indices.assign(e.w*e.h,0);e.base.assign(e.w*e.h,0);e.replaceTile(87,edge.data()+16,edge.data()+16+65536);assert(e.indices.back()==123&&e.base.back()==231&&e.indices.front()==0);
 puts("PASS persistent queue validation/generation, pending updates retained, stain survives moving shadow, reset restores baseline");
}
