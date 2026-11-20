#pragma once
#include "terrain_options.h"
#include "detail_materials.h"
// Optional external detail map. Original terrain, live stains/shade/light stay in tex unit 0.
static GLuint detailArray=0,detailBuffer=0;
static unsigned detailCount=0;
static std::vector<unsigned char> detailVertices,detailTriangles;
static std::vector<float> detailScales;
static void loadDetailTerrain(android_app*a){
 loadTerrainOptions(a->activity->internalDataPath);
 detailCount=0;detailVertices.clear();detailTriangles.clear();detailScales.clear();
 std::string path=std::string(a->activity->internalDataPath)+"/active-detail.pack";FILE*f=fopen(path.c_str(),"rb");if(!f)return;
 try{
  unsigned char h[24];if(fread(h,1,24,f)!=24||memcmp(h,"MYTHDT01",8))throw std::runtime_error("header");
  unsigned w=u32(h+8),ht=u32(h+12),count=u32(h+16),edge=u32(h+20);
  if(w!=levelWidth||ht!=levelHeight||!count||count>128||!edge||edge>2048||(edge&(edge-1))||uint64_t(edge)*edge*count*4>64*1024*1024)throw std::runtime_error("dimensions");
  detailVertices.resize(w*ht);detailTriangles.resize(w*ht*2);detailScales.resize(count*2);
  if(fread(detailVertices.data(),1,detailVertices.size(),f)!=detailVertices.size()||fread(detailTriangles.data(),1,detailTriangles.size(),f)!=detailTriangles.size()||fread(detailScales.data(),sizeof(float),count*2,f)!=count*2)throw std::runtime_error("map data");
  for(auto v:detailVertices)if(v>=count)throw std::runtime_error("vertex index");for(auto v:detailTriangles)if(v>=count)throw std::runtime_error("triangle index");
  for(float v:detailScales)if(!std::isfinite(v)||v<=0||v>256)throw std::runtime_error("UV scale");
  glGenTextures(1,&detailArray);glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D_ARRAY,detailArray);unsigned levels=1;for(unsigned n=edge;n>1;n>>=1)levels++;
  glTexStorage3D(GL_TEXTURE_2D_ARRAY,levels,GL_RGBA8,edge,edge,count);std::vector<unsigned char> pixels(edge*edge*4);
  for(unsigned i=0;i<count;i++){if(fread(pixels.data(),1,pixels.size(),f)!=pixels.size())throw std::runtime_error("texture truncated");glTexSubImage3D(GL_TEXTURE_2D_ARRAY,0,0,0,i,edge,edge,1,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());}
  if(fgetc(f)!=EOF)throw std::runtime_error("trailing data");
  glGenerateMipmap(GL_TEXTURE_2D_ARRAY);glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_S,GL_REPEAT);glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_T,GL_REPEAT);
  if(glGetError()!=GL_NO_ERROR)throw std::runtime_error("GPU allocation");detailCount=count;
  LOG("Detail terrain loaded %ux%u materials=%u array=%u mipmapped",w,ht,count,edge);
 }catch(const std::exception&e){LOG("Detail terrain ignored: %s",e.what());if(detailArray)glDeleteTextures(1,&detailArray);detailArray=0;detailCount=0;}
 fclose(f);glActiveTexture(GL_TEXTURE0);
}
static void buildDetailTerrain(const std::vector<Vertex>&terrain){
 if(!detailCount)return;
 std::vector<float> data;data.reserve(terrain.size()*6);size_t before=0,after=0;
 for(size_t i=0;i<terrain.size();i+=3){
  unsigned xs[3],ys[3];for(unsigned k=0;k<3;k++){xs[k]=unsigned(lroundf(terrain[i+k].u*levelWidth));ys[k]=unsigned(lroundf(terrain[i+k].v*levelHeight));}
  unsigned x=std::min({xs[0],xs[1],xs[2]}),y=std::min({ys[0],ys[1],ys[2]});
  // Original triangle override table swaps order on alternating diagonals.
  // Verified against 0x4299e3, including all four cells of its 2x2 fan.
  unsigned tri=(i/3)%2;unsigned overrideID=detailTriangles[(y*levelWidth+x)*2+(tri^(((x^y)&1)?0:1))];
  std::array<unsigned,3> ids;for(unsigned k=0;k<3;k++){ids[k]=overrideID?overrideID:detailVertices[ys[k]*levelWidth+xs[k]];before+=ids[k]>0;}
  auto compact=detail_materials::compact(ids);after+=compact.samples;
  for(unsigned k=0;k<3;k++){data.insert(data.end(),compact.ids.begin(),compact.ids.end());data.insert(data.end(),compact.weights[k].begin(),compact.weights[k].end());}
 }
 LOG("Terrain detail compact triangles=%zu samples_before=%zu samples_after=%zu",terrain.size()/3,before,after);
 glGenBuffers(1,&detailBuffer);glBindBuffer(GL_ARRAY_BUFFER,detailBuffer);glBufferData(GL_ARRAY_BUFFER,data.size()*sizeof(float),data.data(),GL_STATIC_DRAW);
}
static void useDetailTerrain(bool enable){
 enable=enable&&detailCount&&detailBuffer;
 glUniform1i(glGetUniformLocation(program,"detailEnabled"),enable?1:0);
 if(enable){
  glUniform2f(glGetUniformLocation(program,"detailLodRange"),terrainDetailLod?terrainDetailNear:0.f,terrainDetailLod?terrainDetailFar:0.f);
  glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D_ARRAY,detailArray);glActiveTexture(GL_TEXTURE0);
  glUniform1i(glGetUniformLocation(program,"detailTex"),1);glUniform2f(glGetUniformLocation(program,"detailDimensions"),levelWidth,levelHeight);glUniform2fv(glGetUniformLocation(program,"detailScales"),detailCount,detailScales.data());
  glBindBuffer(GL_ARRAY_BUFFER,detailBuffer);for(unsigned i=2;i<4;i++){glEnableVertexAttribArray(i);glVertexAttribPointer(i,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)(uintptr_t)((i-2)*3*sizeof(float)));}
 }else{glDisableVertexAttribArray(2);glDisableVertexAttribArray(3);}
}
