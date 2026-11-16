#pragma once
#include <string>
static std::string levelExternalRoot;
static std::string levelAssetPrefix,levelName="Willow Creek";
static unsigned levelWidth=224,levelHeight=192;
static float originX=187.2f,originY=156.3f,originZ=-1.82f,levelSkyYaw=.7597628f;
static bool levelInitialCamera=false;static float levelCamera[4]{};
static bool levelMapping=false;static unsigned short levelCollections[256]{};
static unsigned mappedCollection(unsigned col){return levelMapping?(col<256?levelCollections[col]:255):col;}
static void loadLevelConfig(const char*root){
 levelExternalRoot.clear();
 char external[1024];snprintf(external,sizeof(external),"%s/active-level-assets.path",root);FILE* ef=fopen(external,"rb");if(ef){char data[1024]{};size_t length=fread(data,1,sizeof(data),ef);fclose(ef);if(length){std::string expected=std::string(root)+"/external-level";if(length!=expected.size()||memcmp(data,expected.data(),length))throw std::runtime_error("external level directory");levelExternalRoot=expected+"/";}}
 char path[1024];snprintf(path,sizeof(path),"%s/active-level.bin",root);FILE*f=fopen(path,"rb");if(!f)throw std::runtime_error("No active level descriptor");
 unsigned char b[633];size_t n=fread(b,1,sizeof(b),f);fclose(f);if((n!=616&&n!=632)||memcmp(b,"MYTHLV01",8))throw std::runtime_error("level descriptor");
 for(int i=8;i<16;i++)if(!((b[i]>='0'&&b[i]<='9')||(b[i]>='a'&&b[i]<='f')))throw std::runtime_error("level asset key");
 uint32_t w,h;memcpy(&w,b+16,4);memcpy(&h,b+20,4);float values[4];memcpy(values,b+24,16);
 if(w<32||h<32||w>512||h>512||w%32||h%32||!memchr(b+40,0,64))throw std::runtime_error("level dimensions/name");
 for(float v:values)if(!std::isfinite(v))throw std::runtime_error("level coordinates");
 for(unsigned i=0;i<256;i++){unsigned short v;memcpy(&v,b+104+i*2,2);if(v>255)throw std::runtime_error("level collection index");levelCollections[i]=v;}
 levelInitialCamera=n==632;if(levelInitialCamera){memcpy(levelCamera,b+616,16);for(float v:levelCamera)if(!std::isfinite(v))throw std::runtime_error("initial camera");if(levelCamera[0]<0||levelCamera[0]>w||levelCamera[1]<0||levelCamera[1]>h)throw std::runtime_error("initial camera bounds");}
 levelMapping=true;levelAssetPrefix="levels/"+std::string((char*)b+8,8)+"/";levelName=(char*)b+40;
 levelWidth=w;levelHeight=h;originX=values[0];originY=values[1];originZ=values[2];levelSkyYaw=values[3]*3.14159265359f/180;
}
