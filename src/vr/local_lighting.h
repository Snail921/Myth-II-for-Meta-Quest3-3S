#pragma once
// Original light masks provide timing/intensity/extent. Approximate their 3D
// reach with radial lights; no extra shadow pass or HD asset uploads.
static bool enhancedLightingEnabled=true;
static float enhancedLightingAmount=1.f;
static int enhancedLightingLimit=8,enhancedLightingPerObject=8;
static float enhancedLightingLod=10;
static std::array<Mat,2> lightingEyeMatrices{};
static std::array<Mat,2> lightingActiveMatrices{};
static rendercull::Frustum lightingFrusta[2];
static bool lightingViewValid=false,lightingViewDirty=true;
static unsigned lightingBorn=0,lightingGone=0,lightingPeak=0,lightingRaw=0,lightingBoltSegments=0,lightingVisible[2]{},lightingSelected[2]{};
static bool lightingReflected=false;
static bool visibleSpriteLight(float x,float y,float z,float radius,float ex,float ey,float ez,bool segment){
 rendercull::Bounds b;b.add(x-radius,y-radius,z-radius);b.add(x+radius,y+radius,z+radius);
 if(segment){b.add(ex-radius,ey-radius,ez-radius);b.add(ex+radius,ey+radius,ez+radius);}
 return !lightingViewValid||lightingFrusta[0].visible(b)||lightingFrusta[1].visible(b);
}

struct SpriteLight {float x,y,z,radius,intensity;uint64_t identity;std::array<float,3> color{1,1,1};float ex=0,ey=0,ez=0;bool segment=false;};
static std::vector<SpriteLight> spriteLights;
static uint64_t spriteLightRevision=~uint64_t(0);
struct SpriteLightBindingCache {
 bool valid=false,enabled=false;uint64_t revision=0;GLuint program=0;
 float x=0,y=0,z=0,amount=0;int limit=0;
 GLint count=-1,position=-1,color=-1,axis=-1;
};
static SpriteLightBindingCache spriteLightBinding;
static unsigned spriteLightSelections=0,spriteLightBindingReuses=0;
static void setSpriteLightView(bool reflected,const Mat&mirror){
 std::array<Mat,2> matrices=lightingEyeMatrices;if(reflected)for(auto&m:matrices)m=mul(m,mirror);
 if(!lightingViewValid||matrices!=lightingActiveMatrices){lightingActiveMatrices=matrices;lightingViewValid=true;lightingViewDirty=true;for(int i=0;i<2;i++)lightingFrusta[i].set(matrices[i],{-1,-1,1,1},-INFINITY);}
 lightingReflected=reflected;
}
static void reportSpriteLights(){if(!diagnosticEnabled)return;LOG("Lighting population raw_masks=%u raw_bolt_segments=%u candidates=%zu peak=%u born=%u gone=%u normal_visible=%u normal_selected=%u reflection_visible=%u reflection_selected=%u global_limit=%d object_limit=%d lod_start=%.1f",lightingRaw,lightingBoltSegments,spriteLights.size(),lightingPeak,lightingBorn,lightingGone,lightingVisible[0],lightingSelected[0],lightingVisible[1],lightingSelected[1],enhancedLightingLimit,enhancedLightingPerObject,enhancedLightingLod);lightingBorn=lightingGone=0;lightingPeak=spriteLights.size();}
static void loadEnhancedLighting(const char*root){
 lightingViewValid=false;lightingViewDirty=true;lightingBorn=lightingGone=lightingPeak=lightingRaw=lightingBoltSegments=0;lightingVisible[0]=lightingVisible[1]=lightingSelected[0]=lightingSelected[1]=0;
 spriteLightBinding={};spriteLightSelections=spriteLightBindingReuses=0;
 enhancedLightingEnabled=true;enhancedLightingAmount=1.f;enhancedLightingLimit=8;spriteLightRevision=~uint64_t(0);spriteLights.clear();
 std::ifstream f(std::string(root)+"/enhanced-lighting.enabled");char c;if(f.get(c)&&(c=='0'||c=='1'))enhancedLightingEnabled=c=='1';
 std::ifstream limit(std::string(root)+"/enhanced-lighting-limit");int maximum=8;if(limit>>maximum)enhancedLightingLimit=std::clamp(maximum,4,128);
 enhancedLightingPerObject=8;enhancedLightingLod=10;
 std::ifstream objectLimit(std::string(root)+"/enhanced-lighting-per-object");int per=8;if(objectLimit>>per)enhancedLightingPerObject=std::clamp(per,1,128);
 std::ifstream lod(std::string(root)+"/enhanced-lighting-lod");int distance=10;if(lod>>distance)enhancedLightingLod=std::clamp(distance,0,100);
 std::ifstream a(std::string(root)+"/enhanced-lighting-amount");int amount=100;if(a>>amount)enhancedLightingAmount=std::clamp(amount,0,100)/100.f;
}
static std::array<float,3> quadLightColor(const Quad&q,unsigned palette){
 // Only consult already decoded HD art; illumination must not request offscreen images.
 uint64_t id=(uint64_t(3)<<32)|q.texture;if((q.flags&1)&&hd::entries.count(id|(1u<<30)))id|=1u<<30;
 auto hue=liveHues.find(palette);uint64_t hash=0;if(hue!=liveHues.end())for(auto b:hue->second)hash=(hash^b)*1099511628211ull;
 auto high=hd::cache.find(id^(hash<<34));if(high!=hd::cache.end())return high->second.lightColor;
 if(q.texture>=particleTextures.size())return {1,1,1};const auto&t=particleTextures[q.texture];auto pal=livePalettes.find(palette);
 return sampleLightColor(t.w,t.h,[&](unsigned x,unsigned y){size_t i=y*t.w+x;auto p=&t.pixels[i*4];if(pal!=livePalettes.end()&&i<t.indices.size()){auto rgb=&pal->second[t.indices[i]*4];return std::array<unsigned char,4>{rgb[0],rgb[1],rgb[2],p[3]};}return std::array<unsigned char,4>{p[0],p[1],p[2],p[3]};});
}
static std::array<float,3> sourceLightColor(const LiveLight&l){
 unsigned col=mappedCollection(l.key>>16),seq=(l.key>>8)&255,frame=l.key&255,palette=col,collection=col;
 for(const auto&p:liveParticles)if((l.domain==1&&p[15]==1&&p[0]==l.source)||(l.domain==2&&p[15]==0&&p[0]==l.source)||(l.domain==3&&p[15]==0&&(p[0]>>16)==(l.source&65535)&&p[13]==col&&p[2]==seq&&p[3]==frame)) {col=p[13];seq=p[2];frame=p[3];palette=col|(p[14]<<8);collection=particleCollectionKey(p);break;}
 auto it=particleQuads.find({particleCanonical?col:collection,seq,frame,0});if(it==particleQuads.end())return {1,1,1};const Quad&q=it->second;
 return quadLightColor(q,palette);
}

static void prepareSpriteLights(){
 if(spriteLightRevision==snapshotRevision)return;spriteLightRevision=snapshotRevision;auto previous=std::move(spriteLights);spriteLights.clear();lightingRaw=liveLightsValid?liveLights.size():0;lightingBoltSegments=0;
 if(liveLightsValid)for(const auto&l:liveLights){
  if(l.pixels.empty()||!l.alpha)continue;
  int offset=0;float strength=surface::place(0,1,l.transfer,l.phase,offset,l.alpha)/65535.f;
  unsigned peak=*std::max_element(l.pixels.begin(),l.pixels.end());float intensity=strength*peak/255.f;if(intensity<=0)continue;
  // A light-mask texel covers 64 engine coordinates (512 per world unit).
  float footprint=std::max(l.w,l.h)/16.f;
  // Original light fades to zero at height 1024/512 = 2. Not a sphere
  // intersection: conservatively enclose the footprint plus vertical reach.
  float radius=sqrtf(footprint*footprint+4.f)*scale;if(radius<=0)continue;
  float x=mythWorldX(l.x/512.f,originX,scale),z=(l.y/512.f-originY)*scale,y=(l.z/512.f-originZ)*scale;
  if(l.heightMode==1&&terrain.contains(x,z))y=terrain.elevation(x,z);
  uint64_t identity=(uint64_t(l.domain)<<32)|l.source;auto color=sourceLightColor(l);
  for(auto&old:previous)if(old.identity==identity){for(int k=0;k<3;k++)color[k]=old.color[k]+.35f*(color[k]-old.color[k]);break;}
  spriteLights.push_back({x,y,z,radius,intensity,identity,color});
 }
 // Share the captured lightning geometry with the renderer; no new RNG.
 // Merge adjacent nearly straight segments within the same bolt/art sequence.
 std::map<std::array<unsigned,4>,size_t> tails;
 for(const auto&p:liveParticles){if(p[15]!=4||!p[7])continue;lightingBoltSegments++;
  auto q=particleQuads.find({particleCollectionKey(p),p[2],p[3],0});if(q==particleQuads.end())continue;
  Vec3 a{mythWorldX(int32_t(p[4])/512.f,originX,scale),(int32_t(p[6])/512.f-originZ)*scale,(int32_t(p[5])/512.f-originY)*scale};
  Vec3 b{mythWorldX(int32_t(p[10])/512.f,originX,scale),(int32_t(p[12])/512.f-originZ)*scale,(int32_t(p[11])/512.f-originY)*scale};
  Vec3 axis=sub(b,a);float length2=dot(axis,axis);if(length2<.01f*scale*scale)continue;
  float width=q->second.h*q->second.scale*(10.f/512.f)*p[8]/256.f*.5f;
  float intensity=p[7]/65535.f*std::clamp(width/.2f,.25f,1.f);
  float radius=std::clamp(.5f+width*4,.75f,3.f)*scale;
  auto color=quadLightColor(q->second,p[13]|(p[14]<<8));
  std::array<unsigned,4> group{p[0],p[13],p[2],p[14]};auto old=tails.find(group);bool merged=false;
  if(old!=tails.end()){auto&l=spriteLights[old->second];Vec3 end{l.ex,l.ey,l.ez},start{l.x,l.y,l.z},gap=sub(a,end),prev=sub(end,start),whole=sub(b,start);
   float plen=dot(prev,prev),product=dot(prev,axis);
   if(dot(gap,gap)<1e-8f&&product>0&&product*product>.95f*.95f*plen*length2&&dot(whole,whole)<16*scale*scale){
    l.ex=b.x;l.ey=b.y;l.ez=b.z;l.radius=std::max(l.radius,radius);l.intensity=std::max(l.intensity,intensity);merged=true;
   }
  }
  if(!merged){tails[group]=spriteLights.size();spriteLights.push_back({a.x,a.y,a.z,radius,intensity,(uint64_t(4)<<32)|p[0],color,b.x,b.y,b.z,true});}
 }

 if(diagnosticEnabled){std::map<uint64_t,int> delta;for(auto&l:previous)--delta[l.identity];for(auto&l:spriteLights)++delta[l.identity];for(auto&d:delta)if(d.second>0)lightingBorn+=d.second;else lightingGone-=d.second;lightingPeak=std::max(lightingPeak,(unsigned)spriteLights.size());}
}
static void bindSpriteLighting(){
 auto&cache=spriteLightBinding;
 if(!lightingViewDirty&&cache.valid&&cache.program==program&&cache.revision==snapshotRevision&&(!enhancedLightingEnabled||enhancedLightingAmount<=0||spriteLightRevision==snapshotRevision)&&cache.x==camera.x&&cache.y==camera.y&&cache.z==camera.z&&cache.enabled==enhancedLightingEnabled&&cache.amount==enhancedLightingAmount&&cache.limit==enhancedLightingLimit){spriteLightBindingReuses++;return;}
 if(!cache.valid||cache.program!=program){cache.count=glGetUniformLocation(program,"spriteLightCount");cache.position=glGetUniformLocation(program,"spriteLights");cache.color=glGetUniformLocation(program,"spriteLightColors");cache.axis=glGetUniformLocation(program,"spriteLightAxes");}
 lightingViewDirty=false;cache.valid=true;cache.program=program;cache.revision=snapshotRevision;cache.x=camera.x;cache.y=camera.y;cache.z=camera.z;cache.enabled=enhancedLightingEnabled;cache.amount=enhancedLightingAmount;cache.limit=enhancedLightingLimit;spriteLightSelections++;
 int count=0;float position[512]{},colors[512]{},ends[512]{};
 if(enhancedLightingEnabled&&enhancedLightingAmount>0){
  prepareSpriteLights();std::vector<std::pair<float,size_t>> ranked;ranked.reserve(spriteLights.size());
  for(size_t i=0;i<spriteLights.size();i++){const auto&l=spriteLights[i];if(!visibleSpriteLight(l.x,l.y,l.z,l.radius,l.ex,l.ey,l.ez,l.segment))continue;float dx=l.x-camera.x,dy=l.y-camera.y,dz=l.z-camera.z;
   if(l.segment){float ax=l.ex-l.x,ay=l.ey-l.y,az=l.ez-l.z,t=std::clamp(-(dx*ax+dy*ay+dz*az)/std::max(1e-8f,ax*ax+ay*ay+az*az),0.f,1.f);dx+=t*ax;dy+=t*ay;dz+=t*az;}ranked.push_back({l.intensity/(1+(dx*dx+dy*dy+dz*dz)/(l.radius*l.radius)),i});}
  lightingVisible[lightingReflected]=ranked.size();
  std::stable_sort(ranked.begin(),ranked.end(),[](auto&a,auto&b){return a.first>b.first;});
  for(auto&r:ranked){if(count==enhancedLightingLimit)break;const auto&l=spriteLights[r.second];position[count*4]=l.x;position[count*4+1]=l.y;position[count*4+2]=l.z;position[count*4+3]=1/(l.radius*l.radius);for(int k=0;k<3;k++)colors[count*4+k]=l.color[k];colors[count*4+3]=l.intensity*enhancedLightingAmount;
   if(l.segment){float ax=l.ex-l.x,ay=l.ey-l.y,az=l.ez-l.z;ends[count*4]=ax;ends[count*4+1]=ay;ends[count*4+2]=az;ends[count*4+3]=1/std::max(1e-8f,ax*ax+ay*ay+az*az);}count++;}
 }
 lightingSelected[lightingReflected]=count;
 glUniform1i(glGetUniformLocation(program,"spriteObjectLightLimit"),enhancedLightingPerObject);
 glUniform1f(glGetUniformLocation(program,"spriteLightingLodDistance"),enhancedLightingLod);
 glUniform1f(glGetUniformLocation(program,"spriteVertexLodDistance"),shadowDebugEnabled?0.f:20.f);
 glUniform3f(glGetUniformLocation(program,"spriteLightingEye"),camera.x,camera.y,camera.z);
 glUniform1i(cache.count,count);
 if(count){glUniform4fv(cache.position,count,position);glUniform4fv(cache.color,count,colors);glUniform4fv(cache.axis,count,ends);}
}
