
#include <cassert>
#include <cstdio>
#include "../src/vr/chat_controls.h"
int main(){
 ChatButtons b;bool raw[6]={};
 auto r=b.step(raw,true,true);assert(!r.enter&&!r.left&&!r.right);
 raw[0]=raw[1]=true;r=b.step(raw,true,true);assert(r.left&&r.right&&r.blocked[0]&&r.blocked[1]);
 r=b.step(raw,true,true);assert(!r.left&&!r.right);
 r=b.step(raw,false,false);assert(r.blocked[0]&&r.blocked[1]); // hold ownership outside
 raw[0]=raw[1]=false;r=b.step(raw,false,false);assert(!r.blocked[0]&&!r.blocked[1]);
 raw[2]=true;r=b.step(raw,false,true);assert(r.right&&r.blocked[2]);
 raw[2]=false;b.step(raw,false,false);
 raw[3]=true;r=b.step(raw,true,false);assert(r.left&&r.blocked[3]);
 raw[3]=false;b.step(raw,false,false);
 raw[4]=true;r=b.step(raw,false,true);assert(r.backspace&&r.blocked[4]);
 raw[4]=false;b.step(raw,false,false);
 raw[5]=raw[0]=true;r=b.step(raw,true,false);assert(r.enter&&!r.left&&r.blocked[5]);
 r=b.step(raw,false,false);assert(!r.enter&&r.blocked[5]);
 raw[5]=raw[0]=false;b.step(raw,false,false);
 raw[5]=true;r=b.step(raw,false,false);assert(!r.enter&&!r.blocked[5]); // ordinary B preserved
 r=b.step(raw,true,true);assert(!r.enter&&r.blocked[5]); // entering while held does not submit
 puts("PASS dual-hand edges, A/X, Y/B, Enter precedence, no repeat, release ownership and ordinary-button passthrough");
}
