#include "../src/vr/facing_math.h"
#include <cassert>
#include <cstdio>
int main(){
 Vec3 p{10,4,20};
 assert(destinationFacing(p,{9,4,20})==0x10000);
 assert(destinationFacing(p,{10,4,21})==(0x10000|16384));
 assert(destinationFacing(p,{11,4,20})==(0x10000|32768));
 assert(destinationFacing(p,{10,4,19})==(0x10000|49152));
 assert(destinationFacing(p,{10.05f,40,20.05f})==0);
 assert(facingDistance(65530,6)==12);
 std::puts("PASS mirrored facing cardinal directions, horizontal dead zone, wraparound");
}
