#include "../src/vr/camera_settings.h"
#include <cassert>
#include <cmath>
int main(){
 const char*root="/data/local/tmp";const char*path="/data/local/tmp/camera.speeds";
 auto check=[&](const char*text,float degrees){FILE*f=fopen(path,"wb");assert(f);fputs(text,f);fclose(f);loadCameraSpeeds(root);assert(fabsf(cameraSpeeds.mapPitch-degrees*.01745329252f)<.00001f);};
 check("100 100 100 100 -30 15 100 1 1",-30);assert(cameraSpeeds.aimOnJump&&cameraSpeeds.reuseFollowDistance);
 check("100 100 100 100 -30 15 100 1 0",-30);assert(!cameraSpeeds.aimOnJump&&cameraSpeeds.reuseFollowDistance);
 check("100 100 100 100 -30 15 100 1",-30);assert(!cameraSpeeds.aimOnJump);
 check("100 100 100 100 -30 15 100 1",-30);assert(cameraSpeeds.reuseFollowDistance);
 check("100 100 100 100 -30 15 100 0",-30);assert(!cameraSpeeds.reuseFollowDistance);
 check("100 100 100 100 -30 15 100",-30);assert(!cameraSpeeds.reuseFollowDistance);
 check("100 100 100 100 -30 15 50",-30);assert(cameraSpeeds.radialSize==.5f);
 check("100 100 100 100 -30 15 75",-30);assert(cameraSpeeds.radialSize==.75f);
 check("100 100 100 100 -30 15 999",-30);assert(cameraSpeeds.radialSize==1);
 check("100 100 100 100 -30 15 -1",-30);assert(cameraSpeeds.radialSize==.5f);
 check("100 100 100 100 -30 15",-30);assert(cameraSpeeds.radialSize==1);
 check("100 100 100 100 -30 0",-30);assert(cameraSpeeds.orbitDistance==0);
 check("100 100 100 100 -30 100",-30);assert(cameraSpeeds.orbitDistance==100);
 check("100 100 100 100 -30 150",-30);assert(cameraSpeeds.orbitDistance==100);
 check("100 100 100 100 -30 -5",-30);assert(cameraSpeeds.orbitDistance==0);
 check("100 100 100 100 -30",-30);assert(cameraSpeeds.orbitDistance==15);check("100 100 100 100 0",0);check("100 100 100 100 -75",-75);check("100 100 100 100 15",15);check("100 100 100 100 -99",-75);check("100 100 100 100 45",15);check("100 100 100",-30);remove(path);loadCameraSpeeds(root);assert(fabsf(cameraSpeeds.mapPitch+.5235987756f)<.00001f);puts("PASS pitch default -30, legacy angle preservation, offset limits -75/+15");
}
