#pragma once
static void loadWaterLight(android_app*a){
 waterSun[0]=.95f;waterSun[1]=.6f;waterSun[2]=-1.f;
 auto data=asset(a,"water-light.pack");const auto*b=data.data();size_t n=data.size();
 if(b&&n>=4&&u32(b)<=1024&&n==4+size_t(u32(b))*16)for(unsigned i=0;i<u32(b);i++){const auto*r=b+4+i*16;char hex[9];snprintf(hex,sizeof(hex),"%02x%02x%02x%02x",r[0],r[1],r[2],r[3]);if(levelAssetPrefix!="levels/"+std::string(hex)+"/")continue;float v[3];memcpy(v,r+4,12);if(std::isfinite(v[0])&&std::isfinite(v[1])&&std::isfinite(v[2])&&v[1]>0)std::copy(v,v+3,waterSun);}
 LOG("Water sunlight direction=%g,%g,%g",waterSun[0],waterSun[1],waterSun[2]);
}

static void loadWaterNormalAsset(android_app*a){
 AAsset*f=AAssetManager_open(a->activity->assetManager,"water-normal.pack",AASSET_MODE_BUFFER);if(!f){LOG("Water normal: procedural fallback");return;}
 bool loaded=uploadWaterNormal((const unsigned char*)AAsset_getBuffer(f),AAsset_getLength(f));AAsset_close(f);if(!loaded)LOG("Water normal: invalid asset, procedural fallback");
}
