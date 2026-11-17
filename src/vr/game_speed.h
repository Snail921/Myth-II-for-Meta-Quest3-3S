#pragma once
// Engine speed index, including film-only fractional rates. UI lifetime uses real time.
struct GameSpeedNotice {
 int value=-1; double until=0;
 void reset(){value=-1;until=0;}
 void accept(unsigned next,double now){if(next>11)return;if(value>=0&&value!=(int)next)until=now+5;value=(int)next;}
 const char* label()const{static const char* names[]={"Stopped","1/16","1/8","1/4","Half","Normal","Double","x4","x8","x16","x32","x64"};return value>=0&&value<12?names[value]:"";}
};
struct SpeedClick {int speed;bool warp,follow;};
static inline SpeedClick speedClick(bool left,bool right,bool lastLeft,bool lastRight,bool rightGrip){
 bool l=left&&!lastLeft,r=right&&!lastRight;
 bool slow=l&&rightGrip,fast=r&&rightGrip;
 return {slow==fast?-1:(fast?1:0),l&&!rightGrip,r&&!rightGrip};
}
