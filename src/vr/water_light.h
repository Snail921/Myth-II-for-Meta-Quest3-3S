#pragma once
static void loadWaterLight(android_app*a){
 // Shared default calibrated against original ground shadows on Quest.
 // Azimuth 139.0854 deg, elevation 18.4764 deg (2026-11-19).
 waterSun[0]=.6211737f;waterSun[1]=.3169140f;waterSun[2]=-.7167347f;
 sunAngleAdjustment.init(waterSun,a->activity->externalDataPath,levelAssetPrefix);
 LOG("Water sunlight direction=%g,%g,%g",waterSun[0],waterSun[1],waterSun[2]);
}

static void loadWaterNormalAsset(android_app*a){
 AAsset*f=AAssetManager_open(a->activity->assetManager,"water-normal.pack",AASSET_MODE_BUFFER);if(!f){LOG("Water normal: procedural fallback");return;}
 bool loaded=uploadWaterNormal((const unsigned char*)AAsset_getBuffer(f),AAsset_getLength(f));AAsset_close(f);if(!loaded)LOG("Water normal: invalid asset, procedural fallback");
}
