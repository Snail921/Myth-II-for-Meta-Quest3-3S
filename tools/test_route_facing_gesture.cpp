#include "../src/vr/selection_math.h"
#include <cassert>
#include <cstdio>
int main(){
 SelectionGesture g;
 g.press({0,0},0,true,true,true);g.update({1,1},1);
 assert(g.pending&&g.additive&&!g.box&&!g.allowBox);
 g.press({0,0},0,true,true);g.update({1,1},1);
 assert(g.additive&&g.box&&g.allowBox);
 g.press({0,0},0,false,true);g.update({1,1},1);
 assert(!g.additive&&!g.box&&!g.allowBox);
 g.press({0,0},0,false,false);g.update({1,1},1);assert(g.box);
 g.press({0,0},0,true,true,true);g.update({1,1},.1);assert(!g.box);g.cancel();assert(!g.pending);
 std::puts("PASS staged route facing, initial additive box, ordinary facing, unselected box, short click/cancel");
}
