#include "../src/vr/script_camera_gate.h"
#include <cassert>
#include <cstdio>
int main(){
 ScriptCameraGate g;
 assert(!g.update(false,false,1));
 assert(g.update(true,true,2));assert(g.update(true,false,20));
 assert(g.update(false,false,21));assert(g.update(false,false,21.499));assert(!g.update(false,false,21.5));
 assert(g.update(false,true,22));assert(g.update(false,false,22.499));assert(!g.update(false,false,22.5));
 assert(g.update(true,true,23));assert(g.update(false,false,24));assert(g.update(true,true,24.25));assert(g.update(false,false,26));assert(!g.update(false,false,26.5));
 puts("PASS script duration, end+0.5s, instant event, consecutive events");
}
