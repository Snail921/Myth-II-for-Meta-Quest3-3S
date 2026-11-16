#pragma once
#include "interaction_math.h"
struct ScreenPoint {float x=0,y=0;};
struct SelectionView {
 Vec3 origin{},right{1,0,0},up{0,1,0},forward{0,0,-1};float left=-1,r=1,bottom=-1,top=1;
 bool project(Vec3 p,ScreenPoint&out)const{auto d=sub(p,origin);float depth=dot(d,forward);if(depth<=.05f)return false;out={dot(d,right)/depth,dot(d,up)/depth};return true;}
 bool contains(ScreenPoint p)const{return p.x>=left&&p.x<=r&&p.y>=bottom&&p.y<=top;}
 Vec3 point(ScreenPoint p,float depth=2)const{return add(origin,times(add(forward,add(times(right,p.x),times(up,p.y))),depth));}
};
struct SelectionGesture {
 bool pending=false,box=false,additive=false,allowBox=true;double began=0;ScreenPoint start{},end{};
 void cancel(){pending=box=false;}
 void press(ScreenPoint p,double time,bool add,bool hasSelection=false,bool routeDestination=false){allowBox=(!hasSelection||add)&&!routeDestination;pending=true;box=false;start=end=p;began=time;additive=add;}
 void update(ScreenPoint p,double time){if(!pending)return;end=p;if(allowBox&&time-began>=.35)box=true;}
 bool contains(ScreenPoint p)const{return box&&p.x>=std::min(start.x,end.x)&&p.x<=std::max(start.x,end.x)&&p.y>=std::min(start.y,end.y)&&p.y<=std::max(start.y,end.y);}
};
