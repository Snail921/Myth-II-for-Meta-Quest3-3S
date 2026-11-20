#include "../src/vr/camera_settings.h"
#include <cassert>
#include <sys/stat.h>
#include <unistd.h>
int main(){const char*root="/data/local/tmp/myth-map-setting-test";mkdir(root,0700);char path[256];snprintf(path,sizeof(path),"%s/minimap-rotation.enabled",root);unlink(path);loadCameraSpeeds(root);assert(!cameraSpeeds.mapRotation);for(int value:{1,0,2}){FILE*f=fopen(path,"wb");assert(f);fprintf(f,"%d",value);fclose(f);loadCameraSpeeds(root);assert(cameraSpeeds.mapRotation==(value==1));}unlink(path);loadCameraSpeeds(root);assert(!cameraSpeeds.mapRotation);puts("PASS map rotation missing=OFF, ON/OFF reload, invalid=OFF");}
