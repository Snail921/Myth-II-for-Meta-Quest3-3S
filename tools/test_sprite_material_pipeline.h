#pragma once
#include <vector>
#include <map>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>
#define LOG(...) do { printf(__VA_ARGS__); puts(""); } while(0)
static bool enhancedSpritePath=true,diagnosticEnabled=false;
static GLuint program=0;
struct LoadingStub {bool destroyRequested=false;};static LoadingStub*loadingApp=nullptr;
static void pumpLoadingEvents(){}
static double seconds(){return 0;}
#include "../src/vr/sprite_pbr.h"
#include "../src/vr/sprite_material.h"
#include "../src/vr/sprite_atlas.h"
static void testMaterialPipeline(){
 std::string root="/data/local/tmp/myth-pbr-test-"+std::to_string(getpid());mkdir(root.c_str(),0700);mkdir((root+"/SpriteMaterials").c_str(),0700);
 unsigned char source[16],other[16],packed[16],legacy[16];
 for(int i=0;i<4;i++){unsigned char a[]={50,60,70,255},b[]={51,60,70,255},c[]={128,128,77,153},d[]={128,128,255,255};memcpy(source+4*i,a,4);memcpy(other+4*i,b,4);memcpy(packed+4*i,c,4);memcpy(legacy+4*i,d,4);}
 auto write=[&](const char*name,const char*magic,const unsigned char*image,const unsigned char*normal){std::ofstream f(root+"/SpriteMaterials/"+name,std::ios::binary);unsigned header[]={1,2,2};f.write(magic,8);f.write((char*)header,12);auto hash=material_hash::image(2,2,image);f.write((char*)hash.data(),32);f.write((char*)normal,16);};
 write("a-packed.mvrnm","MVRMAT03",source,packed);write("z-old.mvrnm","MVRNORM2",source,legacy);write("legacy.mvrnm","MVRNORM2",other,legacy);
 {std::ofstream bad(root+"/SpriteMaterials/truncated.mvrnm",std::ios::binary);bad.write("MVRMAT03",8);unsigned count=1;bad.write((char*)&count,4);}
 // V4 explicitly disables PBR for the second image, overriding a V3 pack.
 write("zz-packed-old.mvrnm","MVRMAT03",other,packed);
 {std::ofstream f(root+"/SpriteMaterials/aa-flags.mvrnm",std::ios::binary);f.write("MVRMAT04",8);unsigned count=2;f.write((char*)&count,4);
  for(int i=0;i<2;i++){unsigned head[]={2,2,unsigned(i==0)};f.write((char*)head,12);auto hash=material_hash::image(2,2,i?other:source);f.write((char*)hash.data(),32);f.write((char*)(i?legacy:packed),16);}}
 {std::ofstream f(root+"/SpriteMaterials/bad-flags.mvrnm",std::ios::binary);f.write("MVRMAT04",8);unsigned head[]={1,2,2,2};f.write((char*)head,16);auto hash=material_hash::image(2,2,source);f.write((char*)hash.data(),32);f.write((char*)packed,16);}
 sprite_material::load(root.c_str());assert(sprite_material::entries.size()==6);
 auto a=sprite_material::match(2,2,source),b=sprite_material::match(2,2,other);assert(a.texture&&b.texture&&a.packed&&!b.packed);
 GLuint colors[2];glGenTextures(2,colors);sprite_atlas::enabled=true;
 for(int i=0;i<2;i++){glBindTexture(GL_TEXTURE_2D,colors[i]);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,2,2,0,GL_RGBA,GL_UNSIGNED_BYTE,i?other:source);sprite_material::assign(colors[i],i?b:a);sprite_atlas::registerTexture(colors[i],2,2,123);}
 auto ra=sprite_atlas::resolve(colors[0],false),rb=sprite_atlas::resolve(colors[1],false);assert(ra.atlas&&rb.atlas&&ra.material.packed&&!rb.material.packed);assert(ra.material.effects==a.effects&&a.effects==7&&rb.material.effects==0);assert(ra.material.texture!=rb.material.texture);assert(ra.texture==rb.texture);
 GLuint fbo;glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,ra.material.texture,0);assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);
 unsigned char result[4];auto rect=ra.attributes.normal;int x=int(rect.ox*2048),y=int(rect.oy*2048);
 for(auto xy:{std::pair<int,int>{x,y},{x-1,y-1},{x+2,y+2}}){glReadPixels(xy.first,xy.second,1,1,GL_RGBA,GL_UNSIGNED_BYTE,result);assert(memcmp(result,packed,4)==0);}
 glBindFramebuffer(GL_FRAMEBUFFER,0);glDeleteFramebuffers(1,&fbo);
 // Exact 3x3 linear-RGB average, longitude wrapping, yaw and absent-sky reset.
 std::vector<unsigned char> sky(8*4*4,0);for(int y=1;y<=3;y++)for(int x:{7,0,1})for(int c=0;c<3;c++)sky[(y*8+x)*4+c]=255;
 float direction[]={0,0,1};sprite_pbr::setSky(colors[0],8,4,sky.data(),0);sprite_pbr::sampleSun(direction);for(float v:sprite_pbr::sun)assert(fabsf(v-1)<1e-5f);
 sprite_pbr::setSky(colors[0],8,4,sky.data(),3.14159265359f);sprite_pbr::sampleSun(direction);for(float v:sprite_pbr::sun)assert(v==0);
 sprite_pbr::clearSky();assert(!sprite_pbr::sky&&sprite_pbr::pixels.empty());
 sprite_atlas::clear();sprite_atlas::enabled=false;sprite_material::clear();glDeleteTextures(2,colors);assert(glGetError()==GL_NO_ERROR);
 for(const char*name:{"a-packed.mvrnm","z-old.mvrnm","legacy.mvrnm","truncated.mvrnm","aa-flags.mvrnm","zz-packed-old.mvrnm","bad-flags.mvrnm"})unlink((root+"/SpriteMaterials/"+name).c_str());rmdir((root+"/SpriteMaterials").c_str());rmdir(root.c_str());
 puts("PASS runtime pack validation, V4 per-image flags/priority, atlas format separation/gutters, sun sampling seam/yaw/reset");
}
