// Compare production indexed reflection submission to independently packed passes.
#include "../src/vr/main.cpp"
#include <cassert>
static std::vector<unsigned char> readPixels(){std::vector<unsigned char> p(64*64*4);glReadPixels(0,0,64,64,GL_RGBA,GL_UNSIGNED_BYTE,p.data());return p;}
int main(){
 auto d=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(d,0,0));EGLint attrs[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_DEPTH_SIZE,24,EGL_NONE};EGLConfig cfg;EGLint n;assert(eglChooseConfig(d,attrs,&cfg,1,&n)&&n);EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE},pa[]={EGL_WIDTH,64,EGL_HEIGHT,64,EGL_NONE};auto ctx=eglCreateContext(d,cfg,EGL_NO_CONTEXT,ca);auto surface=eglCreatePbufferSurface(d,cfg,pa);assert(eglMakeCurrent(d,surface,surface,ctx));
 enhancedSpritePath=true;enhancedLightingEnabled=enhancedShadowsEnabled=false;diagnosticEnabled=true;fogEnabled=false;fogCenter={0,0,2};camera={0,0,2};shadowLodEye={0,0,2};sprite_pbr::units=true;sprite_pbr::sceneObjects=true;
 auto make=[&](bool pbr){GLuint p=glCreateProgram();auto vs=spriteShaderSource(mythVertexShader,false,false,true,pbr),fs=spriteShaderSource(mythFragmentShader,false,false,true,pbr);auto v=shader(GL_VERTEX_SHADER,vs.c_str()),f=shader(GL_FRAGMENT_SHADER,fs.c_str());glAttachShader(p,v);glAttachShader(p,f);glLinkProgram(p);GLint ok;glGetProgramiv(p,GL_LINK_STATUS,&ok);assert(ok);glUseProgram(p);glUniform1i(glGetUniformLocation(p,"tex"),0);glUniform1i(glGetUniformLocation(p,"detailTex"),1);return p;};
 billboardProgram=make(false);billboardPbrProgram=make(true);
 GLuint tx[4];glGenTextures(4,tx);for(int i=0;i<4;i++){unsigned char pixels[16]={80,120,180,255,90,140,190,160,160,90,80,0,80,190,100,255};if(i==3)for(int j=0;j<4;j++){pixels[j*4]=128;pixels[j*4+1]=128;pixels[j*4+2]=255;pixels[j*4+3]=128;}glBindTexture(GL_TEXTURE_2D,tx[i]);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,2,2,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);}
 sprite_material::Binding mat{tx[3],1,1,0,0,true,5};for(int i=0;i<3;i++)sprite_material::assign(tx[i],mat);sprite_material::textureDimensions[tx[3]]={2,2};
 objects.resize(12);std::vector<std::array<Vertex,6>> geometry;std::vector<SpriteStream::Input> all;
 geometry.reserve(12);for(int i=0;i<12;i++){float x=-.9f+(i%4)*.35f,y=-.9f+(i/4)*.45f,z=-.3f+i*.04f;float u=i%2?1:0,v=1-u;geometry.push_back({Vertex{x,y,z,u,0},Vertex{x+.55f,y,z,v,0},Vertex{x,y+.65f,z,u,1},Vertex{x,y+.65f,z,u,1},Vertex{x+.55f,y,z,v,0},Vertex{x+.55f,y+.65f,z,v,1}});objects[i].unitID=i%2?0:i+1;objects[i].objectType=i%2?2:1;}
 for(int i=0;i<12;i++)all.push_back({tx[(i*7)%3],unsigned(i+1),geometry[i].data(),6});
 Mat m={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};glGenVertexArrays(1,&vao);glBindVertexArray(vao);glVertexAttrib4f(9,0,0,0,1);glViewport(0,0,64,64);glEnable(GL_DEPTH_TEST);glClearColor(0,0,0,0);
 auto check=[](const char*stage){auto error=glGetError();if(error){fprintf(stderr,"GL error %x at %s\n",error,stage);abort();}};check("setup");
 unsigned comparisons=0;
 for(bool atlas:{false,true})for(bool high:{false,true})for(bool cutout:{false,true}){
  sprite_atlas::enabled=atlas;spriteCutoutOnePass=cutout;hd::textures.clear();if(high)hd::textures.insert(tx[1]);for(int i=0;i<3;i++)sprite_atlas::registerTexture(tx[i],2,2,1);
  SpriteStream shared;shared.uploadReferences(all,true);check("shared upload");auto cursor=shared.cursor;auto packed=shared.packed;uint64_t uploads=streamUploadCalls;
  for(unsigned mask:{0u,1u,0x555u,0xaaau,0x123u,0xfffu,0x80fu})for(float height:{-100.f,0.f}){
   reflectionLevel=height;std::vector<size_t> selected;std::vector<SpriteStream::Input> subset;for(size_t i=0;i<all.size();i++)if(mask&(1u<<i)){selected.push_back(i);subset.push_back(all[i]);}
   SpriteStream reference;reference.uploadReferences(subset);check("reference upload");uploads=streamUploadCalls;
   beginSpritePrograms(m,true);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);reference.draw(true);check("reference draw");auto expected=readPixels();
   std::reverse(selected.begin(),selected.end());shared.selectReferences(selected);check("index upload");assert(shared.ranges.size()==reference.ranges.size());assert(shared.indices.size()==reference.packed.size());
   for(size_t j=0;j<shared.indices.size();j++){auto id=shared.indices[j];assert(!memcmp(&shared.packed[id],&reference.packed[j],sizeof(Vertex)));assert(shared.owners[id]==reference.owners[j]);assert(shared.lightBounds.bounds[id]==reference.lightBounds.bounds[j]);}
   beginSpritePrograms(m,true);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);shared.draw(true);check("indexed draw");auto actual=readPixels();assert(actual==expected);assert(glGetError()==GL_NO_ERROR);assert(streamUploadCalls==uploads&&shared.cursor==cursor&&shared.packed.size()==packed.size());
   auto indexUploads=streamIndexUploads;shared.selectReferences(selected);assert(streamIndexUploads==indexUploads);comparisons++;
  }
 }
 // Full production candidate -> stereo/plane union -> indexed draw path.
 reflectionPlanes.resize(2);activeReflectionPlane=0;reflectionLevel=-.2f;reflectionPlanes[1].level=.2f;reflectionFull=true;
 lightingEyeMatrices[0]=m;lightingEyeMatrices[1]=m;lightingEyeMatrices[0][12]=-.07f;lightingEyeMatrices[1][12]=.07f;
 beginSpriteFrame();auto&cache=spriteViews[1];cache.ready=true;
 for(size_t i=0;i<all.size();i++){PreparedSprite p;p.object=i;p.colored=all[i].texture;p.vertices=geometry[i];p.clipTop=2;p.clipHeight=2;p.plane=i==10?0:i==11?1:-1;for(auto&v:p.vertices)p.bounds.add(v.x,v.y,v.z);p.lightBounds=SpriteLightBoundsStream::calculate(p.vertices.data(),6);cache.sprites.push_back(p);}
 auto before=streamUploadCalls;
 for(int plane=0;plane<2;plane++)for(int eye=0;eye<2;eye++){
  activateReflectionPlane(plane);Mat mm=mul(lightingEyeMatrices[eye],reflectionMatrix());reflectionCull.set(mm,{-.75f,-.8f,.85f,.9f},reflectionLevel);
  std::vector<SpriteStream::Input> subset;
  for(auto&p:cache.sprites)if(reflection_selection::accepts(p.plane,p.clipTop,p.clipHeight,objects[p.object].unitID!=0,plane,reflectionLevel,true)&&reflectionCull.visible(p.bounds))subset.push_back({p.colored,spriteLightingOwner(objects[p.object],p.object),p.vertices.data(),6,&p.lightBounds});
  SpriteStream reference;reference.uploadReferences(subset);++before;
  beginSpritePrograms(mm,true);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);reference.draw(true);auto expected=readPixels();
  setSpriteLightView(true,reflectionMatrix());beginSpritePrograms(mm,true);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);drawReflectionSprites({0,2*reflectionLevel-2,2});assert(readPixels()==expected);check("production shared pass");assert(streamUploadCalls==before+1);
 }
 // Regression: the initial union excludes plane 1; a subsequent pass needs it.
 // This used to throw "reflection union missing candidate" and end VR playback.
 for(int pass=0;pass<3;pass++){
  if(pass==0){reflectionCandidates.ready=false;cache.uploaded=false;reflectionPlanes[1].visibility.render=false;prepareReflectionStream();}
  activateReflectionPlane(1);reflectionPlanes[1].visibility.render=true;
  Mat mm=mul(lightingEyeMatrices[0],reflectionMatrix());reflectionCull.set(mm,{-1,-1,1,1},reflectionLevel);
  std::vector<SpriteStream::Input> subset;
  for(auto&p:cache.sprites)if(reflection_selection::accepts(p.plane,p.clipTop,p.clipHeight,objects[p.object].unitID!=0,1,reflectionLevel,true)&&reflectionCull.visible(p.bounds))subset.push_back({p.colored,spriteLightingOwner(objects[p.object],p.object),p.vertices.data(),6,&p.lightBounds});
  SpriteStream reference;reference.uploadReferences(subset);
  beginSpritePrograms(mm,true);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);reference.draw(true);auto expected=readPixels();
  auto uploads=streamUploadCalls;setSpriteLightView(true,reflectionMatrix());beginSpritePrograms(mm,true);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);drawReflectionSprites({0,2*reflectionLevel-2,2});
  assert(readPixels()==expected);check("missing union recovery");assert(reflectionUnionRepairs==1);assert(reflectionUnionAdded>0);assert(streamUploadCalls==uploads+(pass==0?1:0));
 }
 // LOD integration: two eyes share each decision; only retained inputs upload.
 reflection_sprite_lod::enabled=true;reflection_sprite_lod::distance=50;reflection_sprite_lod::pixels=2;reflection_sprite_lod::history.clear();shadowLodEye={0,0,0};reflectionW=reflectionH=64;
 for(size_t i=0;i<cache.sprites.size();i++){
  auto&p=cache.sprites[i];p.plane=-1;p.clipTop=2;p.clipHeight=2;p.vertices=geometry[i];
  if(i!=8)for(auto&v:p.vertices){v.x=p.vertices[0].x+(v.x-p.vertices[0].x)*.01f;v.y=p.vertices[0].y+(v.y-p.vertices[0].y)*.01f;}
  p.bounds={};for(auto&v:p.vertices)p.bounds.add(v.x,v.y,v.z);p.lightBounds=SpriteLightBoundsStream::calculate(p.vertices.data(),6);
  objects[i].x=originX+(i==9?0:100);objects[i].y=originY;objects[i].z=originZ;objects[i].objectKey=i+1;
 }
 reflectionCandidates.ready=reflectionCandidates.passReady=false;
 for(int eye=0;eye<2;eye++){
  Mat mm=mul(lightingEyeMatrices[eye],reflectionMatrix());reflectionCull.set(mm,{-1,-1,1,1},reflectionLevel);
  assert(prepareReflectionSpritePass({0,2*reflectionLevel,0}));
  for(auto id:reflectionCandidates.selected)assert(id==8||id==9);
  assert(cache.inputs.size()==2); // all other subpixel candidates removed before upload
  std::vector<SpriteStream::Input> subset;for(size_t id:{size_t(8),size_t(9)}){auto&p=cache.sprites[id];if(reflectionCull.visible(p.bounds))subset.push_back({p.colored,spriteLightingOwner(objects[p.object],p.object),p.vertices.data(),6,&p.lightBounds});}
  SpriteStream reference;reference.uploadReferences(subset);beginSpritePrograms(mm,true);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);reference.draw(true);auto expected=readPixels();
  beginSpritePrograms(mm,true);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);drawReflectionSprites({0,2*reflectionLevel,0});assert(readPixels()==expected);check("LOD selected image");
 }
 // All candidates tiny and far: no stream draw and no sprite-program setup.
 for(auto&p:cache.sprites){for(auto&v:p.vertices){v.x=p.vertices[0].x+(v.x-p.vertices[0].x)*.01f;v.y=p.vertices[0].y+(v.y-p.vertices[0].y)*.01f;}p.bounds={};for(auto&v:p.vertices)p.bounds.add(v.x,v.y,v.z);objects[p.object].x=originX+100;}
 reflectionCandidates.ready=reflectionCandidates.passReady=false;auto draws=streamDrawCalls;auto oldProgram=program;
 assert(!prepareReflectionSpritePass({0,2*reflectionLevel,0}));assert(cache.inputs.empty());drawReflectionSprites({0,2*reflectionLevel,0});assert(streamDrawCalls==draws&&program==oldProgram);check("empty LOD pass");
 printf("PASS: %u exact GPU image comparisons; indexed/packed ordering, materials, atlas, HD edges, clipping, empty and changing selections; vertex upload reused\n",comparisons);
}
