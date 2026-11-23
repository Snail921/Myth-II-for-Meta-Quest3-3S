// Original fixed-point model poses; no independent animation clock.
// MYTHAM01 stores source geometry (v,u); original 1.8.1 0x4ba76f swaps this pair.
static std::vector<std::pair<unsigned,unsigned>> animatedRanges;
static std::map<unsigned,std::map<GLuint,std::vector<Vertex>>> animatedShapes;
static bool isAnimatedTriangle(unsigned i){for(auto r:animatedRanges)if(i>=r.first&&i-r.first<r.second)return true;return false;}
static void loadAnimatedModels(android_app*a){
 auto b=asset(a,"animated-models.pack");size_t p=0;
 auto take=[&](size_t n){if(n>b.size()-p)throw std::runtime_error("animated model bounds");auto r=b.data()+p;p+=n;return r;};
 auto magic=take(8);bool identities=!memcmp(magic,"MYTHAM02",8);if(!identities&&memcmp(magic,"MYTHAM01",8))throw std::runtime_error("animated model magic");
 auto h=take(12);unsigned nt=u32(h),ns=u32(h+4),nr=u32(h+8);
 if(nt>4096||ns>1024||nr>1024)throw std::runtime_error("animated model sizes");
 animatedRanges.clear();animatedShapes.clear();models.clear();animatedFallback.clear();
 for(unsigned i=0;i<nr;i++){auto r=take(8);animatedRanges.emplace_back(u32(r),u32(r+4));}
 std::vector<GLuint> tx(nt);glGenTextures(nt,tx.data());for(unsigned i=0;i<nt;i++)hd::modelKeys[tx[i]]=(uint64_t(2)<<32)|i;
 for(auto t:tx){auto wh=take(8);unsigned w=u32(wh),h=u32(wh+4);if(!w||!h||w>4096||h>4096)throw std::runtime_error("animated texture size");auto identity=identities?take(32):nullptr;auto pixels=take(size_t(w)*h*4);sprite_material::assign(t,identity?sprite_material::matchIdentity(w,h,identity):sprite_material::match(w,h,pixels));if(sprite_material::get(t).packed)LOG("Model PBR animated texture=%u size=%ux%u matched=1",t,w,h);glBindTexture(GL_TEXTURE_2D,t);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);}
 for(unsigned i=0;i<ns;i++){auto sh=take(8);unsigned key=u32(sh),n=u32(sh+4);if(n>1000000)throw std::runtime_error("animated triangles");auto&shape=animatedShapes[key];for(unsigned j=0;j<n;j++){auto r=take(64);unsigned t=u32(r);if(t>=nt)throw std::runtime_error("animated texture index");for(int k=0;k<3;k++){Vertex v;memcpy(&v,r+4+k*20,20);std::swap(v.u,v.v);shape[tx[t]].push_back(v);}}}
 if(p!=b.size())throw std::runtime_error("animated trailing bytes");
 LOG("Animated shapes=%zu textures=%u fallback ranges=%zu",animatedShapes.size(),nt,animatedRanges.size());
}
static void drawAnimatedModels(bool reflected=false){
 if(!modelPosesValid){for(auto&m:(reflected?animatedFallback:normalFallback)){if(reflected){reflectModelsTested++;if(!reflectionCull.visible(m.bounds))continue;reflectModelsDrawn++;}attributes(m.buffer);GLuint colorTexture=hdModel(m.texture);bindModelMaterial(colorTexture,m.bounds,true);glBindTexture(GL_TEXTURE_2D,colorTexture);glDrawArrays(GL_TRIANGLES,0,m.count);}return;}
 std::map<GLuint,std::vector<Vertex>> batches;
 for(const auto&pose:modelPoses){if(pose.owner==65535)continue;auto shape=animatedShapes.find(pose.geometry);if(shape==animatedShapes.end())continue;
  for(const auto&part:shape->second){std::vector<Vertex> out;out.reserve(part.second.size());rendercull::Bounds bounds;for(auto v:part.second){int32_t local[3]={(int32_t)v.x,(int32_t)v.y,(int32_t)v.z};float xyz[3];for(int k=0;k<3;k++){int64_t sum=0;for(int j=0;j<3;j++)sum+=int64_t(pose.matrix[k+j*3])*local[j];int64_t shifted=sum>=0?sum/512:-((-sum+511)/512);int16_t rotated=(int16_t)shifted;xyz[k]=(pose.pos[k]+int32_t(rotated))/512.f;}out.push_back({mythWorldX(xyz[0],originX,scale),(xyz[2]-originZ)*scale,(xyz[1]-originY)*scale,v.u,v.v});auto&world=out.back();bounds.add(world.x,world.y,world.z);}if(reflected){reflectModelsTested++;if(!reflectionCull.visible(bounds))continue;reflectModelsDrawn++;}auto&batch=batches[part.first];batch.insert(batch.end(),out.begin(),out.end());}
 }
 for(auto&part:batches){attributes(vbo);rendercull::Bounds bounds;for(auto&v:part.second)bounds.add(v.x,v.y,v.z);GLuint colorTexture=hdModel(part.first);bindModelMaterial(colorTexture,bounds,true);glBindTexture(GL_TEXTURE_2D,colorTexture);glBufferData(GL_ARRAY_BUFFER,part.second.size()*sizeof(Vertex),part.second.data(),GL_STREAM_DRAW);glDrawArrays(GL_TRIANGLES,0,(GLsizei)part.second.size());}
}
