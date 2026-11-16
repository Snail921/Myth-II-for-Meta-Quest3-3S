#pragma once
#include <cmath>
struct TutorialCameraGate {
 unsigned stage=0;bool armed=false;float held=0;unsigned previous=0;
 unsigned update(unsigned next,unsigned mask,float dt){
  if(next!=stage){stage=next;armed=false;held=0;previous=0;}
  if(!mask){armed=true;held=0;previous=0;return 0;}
  if(!armed)return 0;
  if(mask!=previous){previous=mask;held=0;}
  held+=dt;return held>=.12f?mask:0;
 }
};
static unsigned tutorialStickBits(float x,float y,unsigned horizontal,unsigned vertical){
 if(std::fabs(x)>=.5f&&std::fabs(x)>std::fabs(y)*1.25f)return 1u<<(horizontal+(x>0?1:0));
 if(std::fabs(y)>=.5f&&std::fabs(y)>std::fabs(x)*1.25f)return 1u<<(vertical+(y>0?0:1));
 return 0;
}
