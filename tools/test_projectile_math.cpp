#include "../src/vr/projectile_math.h"
#include <cassert>
#include <cstdio>
int main(){
 Vec3 flights[]={{1,0,0},{-1,0,0},{0,0,1},{0,0,-1},{0,1,0},{0,-1,0},{1,.5f,1},{-1,-.5f,-1}};
 Vec3 cameras[]={{0,0,1},{0,0,-1},{1,0,0},{0,1,0},{1,2,3}};
 for(auto f:flights)for(auto eye:cameras){auto r=arrowRight(f),u=arrowUp(r,eye);
  assert(fabsf(dot(r,r)-1)<1e-5f&&fabsf(dot(u,u)-1)<1e-5f&&fabsf(dot(r,u))<1e-5f);
  assert(dot(times(r,-1),f)>0); // Left-hand arrow tip points along motion.
 }
 auto front=arrowRight({0,0,1}),back=arrowRight({0,0,-1});assert(front.z==-1&&back.z==1);
 puts("PASS arrow axis: 8 flight directions x 5 views, tip orientation, orthogonality, end-on stability");
}
