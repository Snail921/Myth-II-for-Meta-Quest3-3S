#include "../src/vr/fps_display.h"
#include <cassert>
#include <cmath>
int main(){
 FpsDisplay interval;interval.enabled=true;interval.frame(0,true);interval.frame(.099,true);assert(!interval.ready);interval.frame(.1,true);assert(interval.ready);
 FpsDisplay f;f.enabled=true;
 for(int i=0;i<=120;i++)f.frame(i/120.0,true);
 assert(f.ready&&std::abs(f.value-120)<.01);assert(f.label()=="FPS 120.0");
 f.frame(1.1,false);assert(!f.ready);assert(f.label()=="FPS --.-");
 for(int i=0;i<=60;i++)f.frame(2+i/60.0,true);
 assert(f.ready&&std::abs(f.value-60)<.01);
 f.frame(8,true);assert(!f.ready);
 f.reset();for(int i=0;i<=43;i++)f.frame(10+i/85.5,true);
 assert(f.label()=="FPS 85.5");
 f.load("/data/local/tmp/fps-test-missing");assert(!f.enabled&&!f.ready);
 puts("PASS 120/60/85.5 FPS, one decimal, suspend/reset, default OFF");
}
