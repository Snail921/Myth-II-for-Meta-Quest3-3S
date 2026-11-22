#pragma once
// Per-instance material images. Geometry and draw batches remain unchanged.
struct AppearancePixels {unsigned w=0,h=0;std::vector<uint8_t> rgba;sprite_material::Binding material{};};
struct ModelAppearance {unsigned geometry;int32_t pos[3];unsigned current=~0u;std::vector<GLuint> targets;std::vector<std::vector<unsigned>> variants;};
static std::vector<ModelAppearance> modelAppearances;
static std::map<unsigned,AppearancePixels> appearancePixels;
static void applyModelAppearance(ModelAppearance& a,unsigned variant){
 if(variant>=a.variants.size()||a.current==variant)return;
 for(size_t i=0;i<a.targets.size();i++){auto&p=appearancePixels.at(a.variants[variant][i]);hd::modelKeys[a.targets[i]]=(uint64_t(1)<<32)|a.variants[variant][i];sprite_material::assign(a.targets[i],p.material);glBindTexture(GL_TEXTURE_2D,a.targets[i]);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,p.w,p.h,0,GL_RGBA,GL_UNSIGNED_BYTE,p.rgba.data());}
 LOG("Model appearance geometry=%08x pos=%d,%d,%d variant=%u previous=%u",a.geometry,a.pos[0],a.pos[1],a.pos[2],variant,a.current);a.current=variant;
}
static void updateModelAppearances(){
 if(!modelPosesValid||modelAppearances.empty())return;
 for(auto&a:modelAppearances)for(auto&p:modelPoses)if(p.owner==65535&&p.geometry==a.geometry&&!memcmp(p.pos,a.pos,sizeof(a.pos))){applyModelAppearance(a,p.permutation);break;}
}
static std::map<unsigned,unsigned> loadModelAppearances(const std::vector<unsigned char>&b,size_t offset,unsigned triangles,std::vector<GLuint>&tx,const std::vector<AppearancePixels>&pixels){
 std::map<unsigned,unsigned> overrides;
 auto take=[&](size_t n){if(offset>b.size()||n>b.size()-offset)throw std::runtime_error("appearance bounds");auto r=b.data()+offset;offset+=n;return r;};
 unsigned count=u32(take(4));if(count>1024)throw std::runtime_error("appearance count");
 for(unsigned i=0;i<count;i++){
  auto r=take(32);ModelAppearance a{};a.geometry=u32(r);memcpy(a.pos,r+4,12);unsigned first=u32(r+16),n=u32(r+20),nv=u32(r+24),nm=u32(r+28);
  if(first>triangles||n>triangles-first||!nv||nv>256||!nm||nm>32)throw std::runtime_error("appearance sizes");
  a.variants.resize(nv);for(auto&v:a.variants)for(unsigned j=0;j<nm;j++){unsigned id=u32(take(4));if(id>=pixels.size())throw std::runtime_error("appearance texture");v.push_back(id);if(!appearancePixels.count(id))appearancePixels.emplace(id,pixels[id]);}
  unsigned base=tx.size();a.targets.resize(nm);glGenTextures(nm,a.targets.data());for(auto t:a.targets){glBindTexture(GL_TEXTURE_2D,t);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);tx.push_back(t);}
  for(unsigned j=0;j<n;j++){unsigned material=u32(take(4));if(material>=nm||!overrides.emplace(first+j,base+material).second)throw std::runtime_error("appearance material");}
  applyModelAppearance(a,0);modelAppearances.push_back(std::move(a));
 }
 if(offset!=b.size())throw std::runtime_error("appearance trailing bytes");return overrides;
}
