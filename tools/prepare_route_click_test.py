"""Compile the actual click dispatcher with transport/scene stubs for route transactions."""
from pathlib import Path
s=Path('src/vr/touch_input.h').read_text();body=s[s.index('static void clickPointer('):s.index('static void updateInput(')]
preamble=r'''
#include <vector>
#include <algorithm>
#include <cmath>
#include <cassert>
#include <cstdio>
struct Vec3 {float x=0,y=0,z=0;};struct ScreenPoint{};
struct Object {unsigned unitID=1;bool controllable=true,selected=true;int family=0;};
std::vector<Object> objects(1);std::vector<unsigned> moveMarkerUnits;std::vector<Vec3> moveMarkerPoints;
unsigned hoverItem=0,hoverID=0,routePoints=0,lastTapID=0,moveFacing=0,moveMarkerRevision=0;
float originX=0,originY=0,originZ=0,scale=1,movePressDiameter=1,lastMoveDiameter=1;
double lastTapTime=0,lastMoveTime=0;int originalMode=0;bool groundValid=true,lastMoveGroundAttack=false;
Vec3 groundHit{},lastMove{};struct Cmd {unsigned op,id,flags,facing;int x,y;};std::vector<Cmd> commands;
double seconds(){return 10;}float mythGameX(float x,float origin,float scale){return origin-x/scale;}
bool command(unsigned op,unsigned id=0,unsigned flags=0,int x=0,int y=0,unsigned facing=0){commands.push_back({op,id,flags,facing,x,y});return true;}
void clearMoveMarkers(){moveMarkerPoints.clear();moveMarkerUnits.clear();}
void captureMoveMarkerUnits(){moveMarkerUnits.push_back(1);}float selectedUnitDiameter(){return 1;}
bool visibleUnit(const Object&,ScreenPoint&){return true;}void selectGroup(const std::vector<unsigned>&,bool){}
'''
tests=r'''
int main(){
 clickPointer(true,false,true,{-10,0,20});assert(routePoints==1&&commands.back().op==13&&commands.back().flags==1);
 clickPointer(true,false,true,{-20,0,20});assert(routePoints==2&&commands.back().op==13&&commands.back().flags==0);
 moveFacing=0x14000;clickPointer(false,false,true,{-30,0,20});assert(routePoints==0&&commands[2].op==13&&commands[2].facing==0x14000&&commands[3].op==14);assert(moveMarkerPoints.size()==3);
 commands.clear();moveFacing=0;
 for(int i=0;i<4;i++)clickPointer(true,false,true,{-float(i+1),0,20});
 assert(routePoints==4&&commands.size()==4);clickPointer(true,false,true,{-99,0,20});assert(commands.size()==4);
 moveFacing=0x18000;clickPointer(false,false,true,{-99,0,20});assert(routePoints==0&&commands.size()==6&&commands[4].id==1&&commands[4].x==4*512&&commands[4].facing==moveFacing&&commands[5].op==14);
 clickPointer(true,false,true,{-10,0,20});clickPointer(false,true,true,{-11,0,20});assert(commands.back().op==5&&routePoints==0);
 puts("PASS actual VR click dispatcher: staged append, final commit/facing, four-point limit/update, attack override");
}
'''
Path('build/test_route_click.cpp').write_text(preamble+body+tests)
assert 'if(routePoints&&!extend&&!select)' not in s
