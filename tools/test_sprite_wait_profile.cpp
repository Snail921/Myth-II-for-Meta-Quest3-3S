#include <cstdio>
#include <ctime>
#include <chrono>
#include <algorithm>
#include <cassert>
#include <unistd.h>
static bool diagnosticEnabled=true;
static double seconds(){return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();}
#define LOG(...) do{printf(__VA_ARGS__);puts("");}while(0)
#include "../src/vr/sprite_wait_profile.h"
int main(){for(int i=0;i<34;i++){SpriteWaitScope s(0,2);s.next(3);}assert(spriteWait.stats[0][2].calls==34&&spriteWait.stats[0][3].calls==34);{SpriteWaitScope s(1,11);usleep(20000);}auto a=spriteWait.stats[1][11];assert(a.calls==1&&a.slow==1&&a.wall>=.02&&a.wall>a.cpu+.01);diagnosticEnabled=false;{SpriteWaitScope s(0,2);}assert(spriteWait.stats[0][2].calls==34);spriteWait.report(34);assert(spriteWait.stats[0][2].calls==0);puts("PASS all-pass counts, wall/thread CPU wait separation, peak/slow count, OFF, reset");}
