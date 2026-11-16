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
 puts("PASS 15 sprite-direction cases");return 0;
}
