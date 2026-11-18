#include <cstdio>
#include <cassert>
static bool diagnosticEnabled=true;
#define LOG(...) do{printf(__VA_ARGS__);puts("");}while(0)
#include "../src/vr/sprite_api_profile.h"
int main(){spriteApi.enabled=true;unsigned selected=0;for(int i=0;i<34;i++)selected+=spriteApi.sample(0);assert(selected==2);int ran=0;spriteApi.measure(false,0,"off",0,0,[&]{ran++;});assert(ran==1&&spriteApi.calls==0);for(int i=0;i<12;i++)spriteApi.measure(true,1,"MythVR/test-sleep",i,6,[]{usleep(1500);});assert(spriteApi.calls==12&&spriteApi.slow==12&&spriteApi.stored==8);for(auto&e:spriteApi.events){assert(e.wall>=.5&&e.wall>e.cpu&&e.boot&&e.mono);}spriteApi.report();assert(!spriteApi.calls&&!spriteApi.slow&&!spriteApi.stored);spriteApi.enabled=false;assert(!spriteApi.sample(0));puts("PASS 1/17 sampling, OFF, bounded slow records, wall/thread CPU clocks, reset");}
