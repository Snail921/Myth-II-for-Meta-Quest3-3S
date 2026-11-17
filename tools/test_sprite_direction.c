#include "../src/vr/sprite_direction.h"
#include <stdio.h>
struct Case { float dx,dz,yaw; unsigned count,want; const char*label; };
int main(void){
 const struct Case cases[]={
 {-1,0,0,8,0,"original +x front maps to VR -x"},{1,0,0,8,4,"back"},
 {0,1,0,8,2,"original +x heading seen from +z points left"},
 {-1,1,0,8,1,"front oblique"},{1,1,0,8,3,"rear oblique"},
 {0,-1,0,8,6,"opposite side points right"},
 {-1,0,.25f,8,6,"quarter-turn unit"},{0,1,.25f,8,0,"quarter-turn front"},
 {-1,0,1,8,0,"full-turn wrap"},{-1,1,0,16,2,"sixteen views"},
 {1,1,0,1,0,"single view"},{1,1,0,0,0,"empty view guard"},
 {0,1,.5f,8,6,"VR right-facing unit"},{1,1,.5f,8,7,"move right: front oblique"},
 {-1,1,.5f,8,5,"move left: rear oblique"}
 };
 for(unsigned i=0;i<sizeof(cases)/sizeof(cases[0]);i++){
  const struct Case*c=&cases[i];unsigned got=myth_sprite_direction(c->dx,c->dz,c->yaw,c->count);
  if(got!=c->want){fprintf(stderr,"FAIL %s: %u != %u\n",c->label,got,c->want);return 1;}
 }
 // Reflect the angular lookup while preserving front/back and handedness.
 for(unsigned count=8;count<=16;count+=8)for(unsigned dir=0;dir<count;dir++){
  float angle=dir*6.2831853071795864769f/count;
  float dx=-cosf(angle),dz=sinf(angle);
  for(unsigned flip=0;flip<4;flip++){
   unsigned want=(flip&1)?(count-dir)%count:dir;
   if(myth_object_sprite_direction(dx,dz,0,count,flip)!=want)return 2;
  }
 }
 for(unsigned flip=0;flip<4;flip++){
  if(myth_object_sprite_direction(1,1,.3f,0,flip)!=0)return 3;
  if(myth_object_sprite_direction(1,1,.3f,1,flip)!=0)return 4;
 }
 puts("PASS 15 direction cases, 96 handedness cases, 8 single/empty view cases");return 0;
}
