#pragma once
// Physical order: left trigger, right trigger, A, X, Y, B.
struct ChatButtons {
 bool previous[6]{},owned[6]{};
 struct Result {bool blocked[6]{};bool left=false,right=false,backspace=false,enter=false;};
 Result step(const bool*raw,bool leftHover,bool rightHover){
  Result r;bool any=leftHover||rightHover;
  for(int i=0;i<6;i++){if(!raw[i])owned[i]=false;else if(any)owned[i]=true;r.blocked[i]=owned[i];}
  if(any){
   r.enter=raw[5]&&!previous[5];
   if(!r.enter){
    r.backspace=raw[4]&&!previous[4];
    r.right=rightHover&&((raw[1]&&!previous[1])||(raw[2]&&!previous[2]));
    r.left=leftHover&&((raw[0]&&!previous[0])||(raw[3]&&!previous[3]));
   }
  }
  for(int i=0;i<6;i++)previous[i]=raw[i];
  return r;
 }
};
