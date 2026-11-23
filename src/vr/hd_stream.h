#pragma once
#include "hd_codec.h"
#include "alpha_edges.h"
#include "light_color.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <set>
#include <fstream>
namespace hd {
struct Entry {std::string path;uint64_t offset;unsigned size;std::array<uint8_t,128> meta;std::vector<uint8_t> hues;std::array<int32_t,8> hueMap;};
struct Ready {uint64_t key,entry;unsigned w,h;std::vector<uint8_t> rgba,alpha,materialSource;};
struct Cached {GLuint texture;size_t bytes;uint64_t used;std::array<float,3> lightColor{1,1,1};};
struct Job {uint64_t key,entry;std::array<uint8_t,84> palette;bool colored,matchMaterial;};
static std::map<uint64_t,Entry> entries;
static std::set<std::pair<unsigned,unsigned>> materialSizes; // immutable while the decoder runs
static std::array<uint32_t,4> tint{};
static std::map<uint64_t,Cached> cache;
static std::set<GLuint> textures;
static std::map<uint64_t,sprite_material::Binding> materials; // per HD bitmap, shared across palettes
struct Alpha {unsigned w,h;std::vector<uint8_t> pixels;};
static std::map<uint64_t,Alpha> alphas;static size_t alphaBytes=0;static uint64_t alphaRevision=0;
static std::map<GLuint,uint64_t> modelKeys;
static std::deque<Job> jobs;static std::deque<Ready> ready;static std::set<uint64_t> pending,failed;
static std::mutex mutex;static std::condition_variable cv;static std::thread worker;static bool quit=false;static uint64_t frame=0;static size_t gpuBytes=0;
static void stop(){ {std::lock_guard<std::mutex> lock(mutex);quit=true;cv.notify_all();}if(worker.joinable())worker.join();jobs.clear();ready.clear();pending.clear();failed.clear();for(auto&c:cache)glDeleteTextures(1,&c.second.texture);cache.clear();materialSizes.clear();materials.clear();textures.clear();gpuBytes=0;entries.clear();modelKeys.clear();alphas.clear();alphaBytes=0;alphaRevision++;}
static void run(){
 for(;;){Job job;{std::unique_lock<std::mutex> lock(mutex);cv.wait(lock,[]{return quit||(!jobs.empty()&&ready.size()<4);});if(quit)return;job=jobs.front();jobs.pop_front();}
  try{const auto&e=entries.at(job.entry);std::ifstream file(e.path,std::ios::binary);file.seekg(e.offset);std::vector<uint8_t> bytes(e.size);if(!file.read((char*)bytes.data(),bytes.size()))throw std::runtime_error("HD file read");auto image=decode(bytes);
   std::array<std::array<uint8_t,8>,8> controls{};
   if(job.colored){unsigned count=u32(job.palette.data()+80);for(unsigned i=0;i<8;i++){int source=e.hueMap[i];if(source<0||unsigned(source)>=std::min(count,8u)||i>=e.hues.size()/128)continue;auto raw=job.palette.data()+source*10;auto&c=controls[i];unsigned flags=u16(raw+6);c[0]=(flags&4)?0:1;if(be16(e.hues.data()+i*128+104)&4)c[0]|=2;c[1]=-(flags&1);c[2]=e.hues[i*128+102];c[3]=raw[1];memcpy(c.data()+4,raw+2,4);}}

   Ready out{job.key,job.entry,image.width,image.height,{}, {}, {}};
   if(job.matchMaterial&&materialSizes.count({image.width,image.height}))out.materialSource=hd::rgba(image,{});
   out.rgba=!out.materialSource.empty()&&!job.colored?out.materialSource:hd::rgba(image,controls);
   if(((job.entry>>32)==1||(job.entry>>32)==2)&&tint[0])for(size_t i=0;i<out.rgba.size();i+=4){unsigned r=out.rgba[i]*257,g=out.rgba[i+1]*257,b=out.rgba[i+2]*257,intensity=(((2*r+3*g+b)/5)*tint[0])>>16;for(int c=0;c<3;c++)out.rgba[i+c]=std::min(65535u,(out.rgba[i+c]*257u*(256-tint[0])+tint[c+1]*intensity)>>8)>>8;}out.alpha.reserve(image.pixels.size());for(const auto&p:image.pixels)out.alpha.push_back(p.alpha);
   extendAlphaEdges(out.rgba,out.w,out.h,(job.entry>>32)==1||(job.entry>>32)==2);
   {std::lock_guard<std::mutex> lock(mutex);ready.push_back(std::move(out));}
  }catch(const std::exception&e){LOG("HD decode rejected: %s",e.what());std::lock_guard<std::mutex> lock(mutex);pending.erase(job.key);failed.insert(job.key);}
 }
}
static void load(android_app*a){stop();std::ifstream f(std::string(a->activity->internalDataPath)+"/active-hd.pack",std::ios::binary);if(!f)return;
 try{auto read=[&](void*p,size_t n){if(!f.read((char*)p,n))throw std::runtime_error("HD index truncated");};auto word=[&](){uint32_t v;read(&v,4);return v;};char magic[8];read(magic,8);if(memcmp(magic,"MYTHHD02",8))throw std::runtime_error("HD index magic");unsigned count=word();for(auto&v:tint)v=word();if(tint[0]>256||tint[1]>65535||tint[2]>65535||tint[3]>65535)throw std::runtime_error("HD tint");if(count>100000)throw std::runtime_error("HD index count");
  for(unsigned i=0;i<count;i++){uint64_t kind=word(),index=word();unsigned len=word();if(kind>3||len>4096)throw std::runtime_error("HD key/path");Entry e;e.path.resize(len);read(&e.path[0],len);e.offset=word();e.offset|=uint64_t(word())<<32;e.size=word();read(e.meta.data(),128);len=word();if(e.size>32*1024*1024||len>1024||len%128)throw std::runtime_error("HD entry bounds");e.hues.resize(len);read(e.hues.data(),len);read(e.hueMap.data(),32);entries[(kind<<32)|index]=std::move(e);}
  for(auto& item:sprite_material::hashDimensions)materialSizes.insert(item.first);
  for(auto& item:sprite_material::dimensions)materialSizes.insert(item.first);
  quit=false;worker=std::thread(run);LOG("HD streaming bindings=%zu GPU budget=192MiB",entries.size());
 }catch(const std::exception&e){entries.clear();LOG("HD index unavailable: %s",e.what());}
}
static void begin(){frame++;unsigned uploads=0;size_t bytes=0;
 for(;;){Ready r;{std::lock_guard<std::mutex> lock(mutex);if(ready.empty()||uploads>=2||bytes>=4*1024*1024)break;r=std::move(ready.front());ready.pop_front();pending.erase(r.key);cv.notify_one();}
  if(!alphas.count(r.entry)&&alphaBytes+r.alpha.size()<=64*1024*1024){alphaBytes+=r.alpha.size();alphas[r.entry]={r.w,r.h,std::move(r.alpha)};alphaRevision++;}
  if(!r.materialSource.empty()&&!materials.count(r.entry)){
   auto binding=sprite_material::match(r.w,r.h,r.materialSource.data());
   materials.emplace(r.entry,binding);
   if(binding.texture)LOG("HD material matched entry=%llu size=%ux%u pbr=%d",(unsigned long long)r.entry,r.w,r.h,binding.packed);
  }
  GLuint tx;glGenTextures(1,&tx);glBindTexture(GL_TEXTURE_2D,tx);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,r.w,r.h,0,GL_RGBA,GL_UNSIGNED_BYTE,r.rgba.data());glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,((r.entry>>32)==1||(r.entry>>32)==2)?GL_REPEAT:GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,((r.entry>>32)==1||(r.entry>>32)==2)?GL_REPEAT:GL_CLAMP_TO_EDGE);textures.insert(tx);cache[r.key]={tx,r.rgba.size(),frame,sampleLightColor(r.w,r.h,[&](unsigned x,unsigned y){auto p=&r.rgba[(y*r.w+x)*4];return std::array<unsigned char,4>{p[0],p[1],p[2],p[3]};})};gpuBytes+=r.rgba.size();bytes+=r.rgba.size();uploads++;
 }
 while(gpuBytes>192*1024*1024){auto old=std::min_element(cache.begin(),cache.end(),[](auto&a,auto&b){return a.second.used<b.second.used;});if(old==cache.end()||old->second.used+2>=frame)break;gpuBytes-=old->second.bytes;textures.erase(old->second.texture);sprite_atlas::forget(old->second.texture);sprite_material::bindings.erase(old->second.texture);glDeleteTextures(1,&old->second.texture);cache.erase(old);}
}
static void report(){std::lock_guard<std::mutex> lock(mutex);LOG("HD cache textures=%zu GPU_MiB=%.2f alpha_MiB=%.2f queue=%zu ready=%zu failures=%zu",cache.size(),gpuBytes/1048576.,alphaBytes/1048576.,jobs.size(),ready.size(),failed.size());}
static GLuint request(uint64_t id,const std::array<uint8_t,84>*palette=nullptr){if(!entries.count(id))return 0;uint64_t hash=0;if(palette)for(auto b:*palette)hash=(hash^b)*1099511628211ull;uint64_t key=id^(hash<<34);auto c=cache.find(key);if(c!=cache.end()){c->second.used=frame;return c->second.texture;}
 std::lock_guard<std::mutex> lock(mutex);if(!pending.count(key)&&!failed.count(key)&&jobs.size()<64){bool lookup=!materials.count(id)&&!materialSizes.empty();
  Job j{key,id,{},palette!=nullptr,lookup};if(palette)j.palette=*palette;jobs.push_back(j);pending.insert(key);cv.notify_one();}return 0;}
}
