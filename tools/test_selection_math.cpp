#include "../src/vr/selection_math.h"
#include <cassert>
#include <cstdio>
int main(){
 SelectionView view;ScreenPoint p;
 assert(view.project({1,2,-4},p)&&fabsf(p.x-.25f)<.001f&&fabsf(p.y-.5f)<.001f);
 assert(view.contains(p));assert(!view.project({0,0,1},p));
 assert(view.project({5,0,-1},p)&&!view.contains(p));
 SelectionGesture gesture;gesture.press({.5f,.5f},1,true);
 gesture.update({-.5f,-.5f},1.2);assert(!gesture.box);
 gesture.update({-.5f,-.5f},1.36);assert(gesture.box&&gesture.additive);
 assert(gesture.contains({0,0})&&gesture.contains({-.5f,.5f}));
 assert(!gesture.contains({.6f,0}));
 gesture.cancel();assert(!gesture.pending&&!gesture.box);
 // A translated/rotated view selects the same screen-space point.
 view.origin={10,0,5};view.forward={-1,0,0};view.right={0,0,-1};
 assert(view.project({6,2,4},p)&&fabsf(p.x-.25f)<.001f&&fabsf(p.y-.5f)<.001f);
 auto world=view.point(p,4);assert(fabsf(world.x-6)<.001f&&fabsf(world.z-4)<.001f);
 // A selected group reserves an unmodified hold for commands.
 gesture.press({0,0},2,false,true);gesture.update({1,1},3);assert(gesture.pending&&!gesture.box);
 gesture.press({0,0},2,true,true);gesture.update({1,1},3);assert(gesture.box&&gesture.additive);
 gesture.press({0,0},2,false,false);gesture.update({1,1},3);assert(gesture.box);
 // Frozen drag projection keeps its start at the world anchor despite later head motion.
 SelectionView frozen=view;ScreenPoint anchorScreen;Vec3 anchor{6,2,4};assert(frozen.project(anchor,anchorScreen));
 view.origin={20,3,1};view.forward={0,0,-1};view.right={1,0,0};
 auto fixed=frozen.point(anchorScreen,4);assert(fabsf(fixed.x-anchor.x)<.001f&&fabsf(fixed.y-anchor.y)<.001f&&fabsf(fixed.z-anchor.z)<.001f);
 // Hover endpoint is on the billboard, not the terrain behind it.
 assert(fabsf(rayUnitFrame({0,1,5},{0,0,-1},{0,0,0},{0,0,1},-.5f,.5f,0,2)-5)<.001f);
 assert(rayUnitFrame({1,1,5},{0,0,-1},{0,0,0},{0,0,1},-.5f,.5f,0,2)<0);
 assert(rayUnitFrame({0,3,5},{0,0,-1},{0,0,0},{0,0,1},-.5f,.5f,0,2)<0);
 assert(rayUnitFrame({0,1,5},{1,0,0},{0,0,0},{0,0,1},-.5f,.5f,0,2)<0);
 assert(rayUnitFrame({0,1,5},{0,0,1},{0,0,0},{0,0,1},-.5f,.5f,0,2)<0);
 assert(fabsf(rayUnitFrame({5,1,0},{-1,0,0},{0,0,0},{1,0,0},-.5f,.5f,0,2)-5)<.001f);
 puts("PASS selection projection, view bounds, hold threshold, reverse drag, cancellation and camera transform");
}
