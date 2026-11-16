#pragma once
#include "interaction_math.h"
static constexpr int commandMenuCount=5;
static constexpr unsigned commandMenuActions[]={4,2,1,0,5};
// Input remains in OpenXR tracking space, independent of virtual camera motion.
static int radialSector(float x,float y,int count){
 if(x*x+y*y<.04f*.04f)return -1;
 float a=atan2f(x,y);if(a<0)a+=6.283185307f;
 return int(floorf(a*count/6.283185307f+.5f))%count;
}
// Mode is latched at X press; grip wins over shift for additive recall.
static int formationMenuKind(bool grip,bool shift,bool selected){return grip?4:shift?3:selected?2:4;}
struct RadialMenu {
 float scale=1;int kind=0,item=-1;Vec3 anchor{},right{},up{},normal{},hand{};
 int release(int buttonKind){if(kind!=buttonKind)return -1;int chosen=item;close();return chosen;}
 void close(){kind=0;item=-1;}
 void open(int k,Vec3 p,Vec3 r,Vec3 u,Vec3 n,float size=1){scale=fmaxf(.5f,fminf(1.f,size));kind=k;anchor=p;right=r;up=u;normal=n;hand=p;item=-1;}
 void update(Vec3 p){hand=p;auto d=sub(p,anchor);item=radialSector(dot(d,right)/scale,dot(d,up)/scale,kind==1?commandMenuCount:10);}
 Vec3 point(float x,float y)const{return add(add(anchor,times(normal,-.12f)),add(times(right,x*scale),times(up,y*scale)));}
};
