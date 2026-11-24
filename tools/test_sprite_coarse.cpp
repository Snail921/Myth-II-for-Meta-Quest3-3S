#include "../src/vr/main.cpp"
#include "sprite_prepare_reference.inc"
#include <cassert>
#include <random>
static unsigned comparisons=0;
static void compareVisible(const std::vector<PreparedSprite>&reference,const std::vector<PreparedSprite>&actual,bool reflected){
 for(int plane=0;plane<(reflected?2:1);plane++)for(int eye=0;eye<2;eye++){
  float h=plane==activeReflectionPlane?reflectionLevel:reflectionPlanes[plane].level;
  Mat mirror={1,0,0,0,0,-1,0,0,0,0,1,0,0,2*h,0,1};rendercull::Frustum f;f.set(reflected?mul(lightingEyeMatrices[eye],mirror):lightingEyeMatrices[eye],reflected?rendercull::Rect{-.6f,-.7f,.8f,.9f}:rendercull::Rect{-1,-1,1,1},reflected?h:-INFINITY);
  auto visible=[&](const PreparedSprite&p){return (!reflected||reflection_selection::accepts(p.plane,p.clipTop,p.clipHeight,objects[p.object].unitID!=0,plane,h,reflectionFull))&&f.visible(p.bounds);};
  std::vector<const PreparedSprite*> a,b;for(auto&p:reference)if(visible(p))a.push_back(&p);for(auto&p:actual)if(visible(p))b.push_back(&p);
  assert(a.size()==b.size());for(size_t i=0;i<a.size();i++){assert(a[i]->object==b[i]->object&&a[i]->texture==b[i]->texture&&a[i]->plane==b[i]->plane);assert(!memcmp(a[i]->vertices.data(),b[i]->vertices.data(),sizeof(a[i]->vertices)));}++comparisons;
 }
}
int main(){
 textures.resize(16);sprite_atlas::enabled=false;pointerValid=false;reflectionPlanes.resize(2);activeReflectionPlane=0;reflectionLevel=-.2f;reflectionPlanes[1].level=.2f;
 std::mt19937 rng(42);std::uniform_real_distribution<float> pos(-80,80);
 for(unsigned col:{71u,87u})for(unsigned frame=0;frame<3;frame++)for(unsigned dir=0;dir<8;dir++){
  Quad q{};q.col=col;q.frame=frame;q.dir=dir;q.views=col==87?1:8;q.scale=3+frame;q.w=8+dir;q.h=12;q.rx=dir-2.f;q.ry=10;q.flags=col==87?0x40000010u:dir%4;q.texture=dir;quads[key(col,0,frame,dir)]=q;
 }
 for(int i=0;i<1040;i++){Object o{};o.col=i%5?71:87;o.frame=i%3;o.scale=.5f+(i%4);o.x=originX+pos(rng);o.y=originY+pos(rng);o.z=originZ+pos(rng)*.1f;o.objectType=2;o.rawFlags=(i%4)<<17;o.motionX=1;o.motionY=.3f;o.motionZ=.2f;o.unitID=i%11?0:i+1;o.selected=i%101==0;objects.push_back(o);}
 for(int turn=0;turn<24;turn++){
  pointerValid=turn%3==0;hoverID=objects[11].unitID;hoverItem=objects[5].objectKey;selectionPreview={objects[22].unitID};objects[7].frame=turn%3;objects[8].x+=.1f;
  if(turn==12||turn==18)for(unsigned texture=0;texture<8;texture++){
   hd::Entry e{};auto put=[&](int offset,int value){e.meta[offset]=uint16_t(value)>>8;e.meta[offset+1]=value;};put(80,24);put(82,30);put(92,-8);put(94,26);e.meta[97]=1;
   uint64_t id=texture|(turn==18?(1u<<30):0);hd::entries[id]=e;hd::cache[id]={123+texture,0,hd::frame};
  }
  reflectionFull=turn%2;XrVector3f center{float(turn)*.3f,5,0};
  for(int eye=0;eye<2;eye++){XrPosef pose{};pose.orientation.w=cosf(turn*.11f);pose.orientation.y=sinf(turn*.11f);pose.position={center.x+(eye?.03f:-.03f),center.y,center.z};lightingEyeMatrices[eye]=mul(projection({-.7f,.7f,.7f,-.7f}),view(pose));}
  XrVector3f reflectedCenter{center.x,2*reflectionLevel-center.y,center.z};
  beginSpriteFrame();prepareSpritesReference(center,false);prepareSpritesReference(reflectedCenter,true);auto normal=spriteViews[0].sprites,reflection=spriteViews[1].sprites;auto selection=spriteViews[0].selectionFrames,health=spriteViews[0].healthBars,mana=spriteViews[0].manaBars;
  beginSpriteFrame();if(turn%2){prepareSprites(center,false);prepareSprites(reflectedCenter,true);}else{prepareSprites(reflectedCenter,true);prepareSprites(center,false);}
  compareVisible(normal,spriteViews[0].sprites,false);compareVisible(reflection,spriteViews[1].sprites,true);
  auto equal=[](const auto&a,const auto&b){assert(a.size()==b.size());if(!a.empty())assert(!memcmp(a.data(),b.data(),a.size()*sizeof(Vertex)));};equal(selection,spriteViews[0].selectionFrames);equal(health,spriteViews[0].healthBars);equal(mana,spriteViews[0].manaBars);
 }
 assert(spriteCoarseRejected[0]>0&&spriteCoarseRejected[1]>0&&spritePreparationShared>0);
 hd::entries.clear();hd::cache.clear();
 // HD metadata may extend far beyond the legacy image, before it is decoded.
 hd::Entry entry{};auto set16=[&](int offset,int value){entry.meta[offset]=uint16_t(value)>>8;entry.meta[offset+1]=value;};set16(80,200);set16(82,180);set16(92,-80);set16(94,160);entry.meta[97]=2;
 auto&o=objects[0];o.col=71;o.frame=0;o.rawFlags=0;beginSpriteFrame();assert(chooseSpritePreparation(0,{0,5,0}));auto legacy=spritePreparations[0];hd::entries[legacy.q.texture]=entry;
 beginSpriteFrame();assert(chooseSpritePreparation(0,{0,5,0}));assert(spritePreparations[0].coarse.hi[0]>legacy.coarse.hi[0]);hd::entries.clear();
 printf("PASS %u exact visible-geometry comparisons; overlays, flips, axial planes, both preparation orders, frame invalidation, HD extents; coarse rejected=%u,%u shared=%u\n",comparisons,spriteCoarseRejected[0],spriteCoarseRejected[1],spritePreparationShared);
}
