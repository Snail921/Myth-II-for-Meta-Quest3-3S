#include <initializer_list>
#include "../src/vr/radial_math.h"
#include <cassert>
#include <cstdio>
int main(){
 assert(radialSector(0,0,4)==-1);
 assert(radialSector(.02f,.02f,4)==-1);
 assert(radialSector(0,.1f,4)==0);
 assert(radialSector(.1f,0,4)==1);
 assert(radialSector(0,-.1f,4)==2);
 assert(radialSector(-.1f,0,4)==3);
 for(int i=0;i<10;i++){float a=i*6.283185307f/10;assert(radialSector(.1f*sinf(a),.1f*cosf(a),10)==i);}
 RadialMenu menu;menu.open(1,{1,2,3},{0,0,-1},{0,1,0},{1,0,0});
 menu.update({1,2,2.9f});assert(menu.item==1);
 // Motion normal to the menu should not select another sector.
 menu.update({1.5f,2,2.9f});assert(menu.item==1);
 menu.update({1.5f,2,3});assert(menu.item==-1);
 menu.close();assert(menu.kind==0&&menu.item==-1);
 menu.open(1,{0,0,0},{1,0,0},{0,1,0},{0,0,1});menu.update({.1f,0,0});
 assert(menu.release(2)==-1&&menu.kind==1); // Other button cannot confirm.
 assert(menu.release(1)==1&&menu.kind==0);assert(menu.release(1)==-1);
 menu.open(2,{0,0,0},{1,0,0},{0,1,0},{0,0,1});menu.update({0,.1f,0});
 assert(menu.release(2)==0&&menu.kind==0);
 menu.open(1,{0,0,0},{1,0,0},{0,1,0},{0,0,1});assert(menu.release(1)==-1&&menu.kind==0);
 for(int i=0;i<commandMenuCount;i++){float a=i*6.283185307f/commandMenuCount;
  menu.open(1,{0,0,0},{1,0,0},{0,1,0},{0,0,1});menu.update({.1f*sinf(a),.1f*cosf(a),0});assert(menu.release(1)==i);assert(menu.release(1)==-1);
 }
 for(float scale:{.5f,.75f,1.f})for(int kind=1;kind<=4;kind++){
  int count=kind==1?commandMenuCount:10;
  for(int i=0;i<count;i++){float a=i*6.283185307f/count;
   menu.open(kind,{0,0,0},{1,0,0},{0,1,0},{0,0,1},scale);
   menu.update({.039f*scale*sinf(a),.039f*scale*cosf(a),0});assert(menu.item==-1);
   menu.update({.1f*scale*sinf(a),.1f*scale*cosf(a),0});assert(menu.item==i);
   auto p=menu.point(.1f,0);assert(fabsf(p.x-.1f*scale)<1e-6f&&fabsf(p.z+.12f)<1e-6f);
  }
 }
 assert(commandMenuActions[0]==4&&commandMenuActions[1]==2&&commandMenuActions[2]==1&&commandMenuActions[3]==0&&commandMenuActions[4]==5);
 puts("PASS radial dead zone, five command and ten formation sectors, tracking-space plane and cancellation");
}
