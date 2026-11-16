#include "../src/vr/main.cpp"
#include <cassert>
int main(){
 objects.clear();quads.clear();selectionPreview.clear();pointerValid=false;reflectionFull=true;
 Quad q{};q.col=71;q.views=8;q.scale=512;q.w=8;q.h=12;q.rx=4;q.ry=12;
 for(unsigned d=0;d<8;d++){q.dir=d;q.texture=d;quads[key(71,0,0,d)]=q;}
 Object o{};o.col=71;o.scale=1;o.x=originX-2;o.y=originY+3;o.z=originZ+5;o.objectType=1;objects.push_back(o);
 XrVector3f center{0,7,0};beginSpriteFrame();prepareSprites(center,false);assert(spriteViews[0].sprites.size()==1);auto first=spriteViews[0].sprites[0];
 prepareSprites(center,false);assert(spriteBuilds[0]==1&&spriteReuse[0]==1);assert(!memcmp(first.vertices.data(),spriteViews[0].sprites[0].vertices.data(),sizeof(first.vertices)));
 prepareSprites({center.x,2*reflectionLevel-center.y,center.z},true);assert(spriteViews[1].sprites.size()==1);assert(spriteBuilds[1]==1);
 auto capacity=spriteViews[0].sprites.capacity();beginSpriteFrame();center.x=10;prepareSprites(center,false);assert(spriteViews[0].sprites.capacity()==capacity);assert(spriteBuilds[0]==2);assert(memcmp(first.vertices.data(),spriteViews[0].sprites[0].vertices.data(),sizeof(first.vertices)));
 objects[0].x+=4;beginSpriteFrame();prepareSprites(center,false);assert(spriteViews[0].sprites[0].vertices[0].x!=first.vertices[0].x);
 // Axial arrows depend on reflected head height: mode caches must stay separate.
 objects[0].col=87;objects[0].seq=9;objects[0].motionX=1;objects[0].motionY=0;objects[0].motionZ=.2f;
 q.col=87;q.seq=9;q.views=1;q.flags=0x40000010;q.dir=0;quads[key(87,9,0,0)]=q;
 beginSpriteFrame();prepareSprites(center,false);auto normalArrow=spriteViews[0].sprites[0].vertices;
 prepareSprites({center.x,2*reflectionLevel-center.y,center.z},true);
 assert(memcmp(normalArrow.data(),spriteViews[1].sprites[0].vertices.data(),sizeof(normalArrow)));
 assert(!memcmp(normalArrow.data(),spriteViews[0].sprites[0].vertices.data(),sizeof(normalArrow)));
 // Hidden original objects stay synchronized but never reach either eye/mode or picking.
 objects[0].unitID=10200;objects[0].selected=true;objects[0].rawFlags=0x04420610;
 ScreenPoint screen;assert(!objectVisible(objects[0]));assert(!visibleUnit(objects[0],screen));
 beginSpriteFrame();prepareSprites(center,false);prepareSprites(center,true);
 assert(spriteViews[0].sprites.empty()&&spriteViews[1].sprites.empty());assert(spriteViews[0].selectionFrames.empty());
 objects[0].rawFlags&=~0x200u;beginSpriteFrame();prepareSprites(center,false);assert(spriteViews[0].sprites.size()==1);
 objects[0].rawFlags|=0x20000000u;assert(!objectVisible(objects[0]));
 objects.clear();beginSpriteFrame();prepareSprites(center,false);assert(spriteViews[0].sprites.empty());
 puts("PASS sprite cache: per-mode build once, eye reuse, frame invalidation, capacity reuse, motion and removal");
}
