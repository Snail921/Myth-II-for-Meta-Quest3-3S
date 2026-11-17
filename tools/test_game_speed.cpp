#include "../src/vr/game_speed.h"
#include <cassert>
#include <cstring>
#include <cstdio>
int main(){
 auto a=speedClick(1,0,0,0,1);assert(a.speed==0&&!a.warp&&!a.follow);
 a=speedClick(0,1,0,0,1);assert(a.speed==1&&!a.warp&&!a.follow);
 a=speedClick(1,1,0,0,1);assert(a.speed==-1&&!a.warp&&!a.follow);
 a=speedClick(1,0,1,0,1);assert(a.speed==-1&&!a.warp);
 a=speedClick(1,0,0,0,0);assert(a.speed==-1&&a.warp);
 a=speedClick(0,1,0,0,0);assert(a.speed==-1&&a.follow);
 // Releasing the modifier while the stick remains held never generates a jump.
 a=speedClick(1,0,1,0,0);assert(a.speed==-1&&!a.warp&&!a.follow);
 a=speedClick(0,1,0,1,0);assert(a.speed==-1&&!a.warp&&!a.follow);
 GameSpeedNotice n;n.accept(5,1);assert(n.until==0);n.accept(4,2);assert(n.until==7);n.accept(4,3);assert(n.until==7);n.accept(0,4);assert(n.until==9&&!strcmp(n.label(),"Stopped"));n.accept(12,5);assert(n.value==0&&n.until==9);n.reset();assert(n.value==-1&&n.until==0);
 puts("PASS right-grip click routing, held/simultaneous suppression, authoritative notice and five-second real-time lifetime");
}
