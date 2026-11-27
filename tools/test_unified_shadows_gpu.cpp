#include "../src/vr/main.cpp"
#include <cassert>
int main(){
 EGLDisplay display=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(display,0,0));EGLint aa[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_DEPTH_SIZE,24,EGL_NONE};EGLConfig cfg;EGLint n;assert(eglChooseConfig(display,aa,&cfg,1,&n)&&n);
 EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE},pa[]={EGL_WIDTH,512,EGL_HEIGHT,512,EGL_NONE};auto context=eglCreateContext(display,cfg,EGL_NO_CONTEXT,ca);auto surface=eglCreatePbufferSurface(display,cfg,pa);assert(eglMakeCurrent(display,surface,surface,context));glViewport(0,0,512,512);

 setbuf(stdout,nullptr);
 auto errors=[](const char* stage){GLenum e;while((e=glGetError())){printf("GL error %s: %x\n",stage,e);assert(false);}};
 glGenVertexArrays(1,&vao);glBindVertexArray(vao);
 glGenTextures(1,&whiteTexture);glBindTexture(GL_TEXTURE_2D,whiteTexture);unsigned char white[]={255,255,255,255};glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,white);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
 auto make=[&](bool unified,bool correction){auto vs=spriteShaderSource(mythVertexShader,false,false,false);auto fs=spriteShaderSource(mythFragmentShader,false,false,false);if(unified)fs=unifiedTerrainShader(fs);fs=scene_correction::fragment(fs,correction);GLuint p=glCreateProgram();glAttachShader(p,shader(GL_VERTEX_SHADER,vs.c_str()));glAttachShader(p,shader(GL_FRAGMENT_SHADER,fs.c_str()));glLinkProgram(p);GLint ok;glGetProgramiv(p,GL_LINK_STATUS,&ok);if(!ok){char log[4096];glGetProgramInfoLog(p,sizeof(log),0,log);puts(log);}assert(ok);glUseProgram(p);glUniform1i(glGetUniformLocation(p,"detailTex"),1);glUniform1i(glGetUniformLocation(p,"tex"),0);return p;};
 lightweightProgram=make(false,false);unifiedTerrainProgram=make(true,false);glDeleteProgram(make(true,true));glUseProgram(lightweightProgram);
 // Geometry is viewed from above; the upright blended scenery card casts onto it.
 originX=originY=originZ=0;waterSun[0]=0;waterSun[1]=.6;waterSun[2]=1;fogEnabled=false;reflectionEnabled=waterWavesEnabled=false;
 Vertex plane[]={{-2,0,-2,0,0},{2,0,-2,1,0},{-2,0,2,0,1},{2,0,-2,1,0},{2,0,2,1,1},{-2,0,2,0,1}};
 glGenBuffers(1,&terrainVBO);glBindBuffer(GL_ARRAY_BUFFER,terrainVBO);glBufferData(GL_ARRAY_BUFFER,sizeof(plane),plane,GL_STATIC_DRAW);terrainCount=6;rendercull::Range range;range.bounds.add(-2,0,-2);range.bounds.add(2,0,2);range.count=6;terrainRanges={range};
 Quad q{};q.col=1;q.views=1;q.w=q.h=32;q.rx=16;q.ry=32;q.scale=3.2;q.texture=0;q.flags=4;quads[key(1,0,0,0)]=q;textures={whiteTexture};Object tree{};tree.col=1;tree.objectType=2;tree.scale=1;tree.objectKey=17;objects={tree};
 Mat top={.5,0,0,0, 0,0,.1,0, 0,.5,0,0, 0,0,0,1};
 auto sample=[&](bool unified,bool enhanced,int px,int py){unifiedShadowsEnabled=unified;enhancedShadowsEnabled=enhanced;glBindFramebuffer(GL_FRAMEBUFFER,0);glViewport(0,0,512,512);glDisable(GL_DEPTH_TEST);glDisable(GL_BLEND);glDisable(GL_CULL_FACE);useSceneProgram(lightweightProgram,top,false);beginUnifiedTerrain(top,false);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,whiteTexture);attributes(terrainVBO);glVertexAttrib4f(9,1,0,0,0);errors("before draw");glDrawArrays(GL_TRIANGLES,0,6);errors("draw");unsigned char pixel[4];glReadPixels(px,py,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);assert(glGetError()==GL_NO_ERROR);return pixel[0];};
 for(int size:{512,1024,2048,4096,8192}){
  if(spriteLiveFBO)glDeleteFramebuffers(1,&spriteLiveFBO);if(spriteLiveDepth)glDeleteTextures(1,&spriteLiveDepth);if(spriteLiveOwner)glDeleteTextures(1,&spriteLiveOwner);spriteLiveFBO=spriteLiveDepth=spriteLiveOwner=0;
  dynamicShadowResolution=size;enhancedShadowsEnabled=true;snapshotRevision++;updateSunShadows();assert(spriteCasterCount==1);errors("shadow generation");
  int lit=sample(false,true,256,128),shadow=sample(true,true,256,128),off=sample(true,false,256,128),side=sample(true,true,500,128);
  printf("resolution=%d tree shadow=%d legacy=%d disabled=%d outside=%d\n",size,shadow,lit,off,side);assert(lit>250&&off==lit&&shadow<lit-60&&side>250);
  // Static geometry depth must never be applied again by the unified terrain path.
  objects.clear();snapshotRevision++;enhancedShadowsEnabled=true;updateSunShadows();assert(sample(true,true,256,128)>250);objects={tree};
 }
 // A non-coplanar terrain quad must not reveal its diagonal in diffuse lighting.
 {
  auto savedLive=spriteLiveDepth,savedStatic=waterStaticDepth;auto savedMatrix=waterSunMatrix;
  spriteLiveDepth=waterStaticDepth=whiteTexture;waterSunMatrix={};waterSun[0]=waterSun[2]=0;waterSun[1]=1;
  unifiedBaseReady=true;terrain.width=terrain.height=5;originX=originY=2;
  Vertex folded[6];memcpy(folded,plane,sizeof(plane));folded[0].y=2;
  glBindBuffer(GL_ARRAY_BUFFER,terrainVBO);glBufferData(GL_ARRAY_BUFFER,sizeof(folded),folded,GL_STATIC_DRAW);
  int flatA=sample(true,true,254,254),flatB=sample(true,true,258,258);
  assert(abs(flatA-flatB)>10);
  std::vector<float> smoothHeight(25);for(int y=0;y<5;y++)for(int x=0;x<5;x++)smoothHeight[y*5+x]=.125f*x*y;
  buildTerrainNormals(5,5,smoothHeight);
  int smoothA=sample(true,true,254,254),smoothB=sample(true,true,258,258);
  printf("terrain diagonal flat=%d/%d smooth=%d/%d\n",flatA,flatB,smoothA,smoothB);
  assert(abs(smoothA-smoothB)<=3);
  assert(sample(false,true,254,254)>250); // OFF retains original rendering.
  releaseTerrainNormals();spriteLiveDepth=savedLive;waterStaticDepth=savedStatic;waterSunMatrix=savedMatrix;
  originX=originY=0;waterSun[1]=.6;waterSun[2]=1;unifiedBaseReady=false;
  glBindBuffer(GL_ARRAY_BUFFER,terrainVBO);glBufferData(GL_ARRAY_BUFFER,sizeof(plane),plane,GL_STATIC_DRAW);
 }
 // Full base-color mode receives fixed geometry and terrain lighting too.
 unifiedBaseReady=true;objects.clear();enhancedShadowsEnabled=true;unifiedShadowsEnabled=true;
 std::vector<Vertex> roof;for(auto v:plane){v.x*=.25f;v.z*=.25f;v.y=.6f;roof.push_back(v);}GLuint roofBuffer;glGenBuffers(1,&roofBuffer);glBindBuffer(GL_ARRAY_BUFFER,roofBuffer);glBufferData(GL_ARRAY_BUFFER,sizeof(plane),roof.data(),GL_STATIC_DRAW);normalModels={{whiteTexture,roofBuffer,6,range.bounds}};waterSunShadowReady=false;snapshotRevision++;updateSunShadows();
 int fixedShadow=sample(true,true,256,128),fixedLit=sample(true,true,500,128);assert(fixedShadow<fixedLit-20&&fixedLit<250);
 unifiedBaseReady=false;assert(sample(true,true,256,128)>250);normalModels.clear();waterSunShadowReady=false;objects={tree};snapshotRevision++;updateSunShadows();
 // Mesh receiver path works without PBR textures or PBR enabled.
 auto mvs=spriteShaderSource(mythVertexShader,false,false,false),mfs=unifiedTerrainShader(spriteShaderSource(mythFragmentShader,false,false,false),true);
 unifiedMeshProgram=glCreateProgram();glAttachShader(unifiedMeshProgram,shader(GL_VERTEX_SHADER,mvs.c_str()));glAttachShader(unifiedMeshProgram,shader(GL_FRAGMENT_SHADER,mfs.c_str()));glLinkProgram(unifiedMeshProgram);GLint meshLinked;glGetProgramiv(unifiedMeshProgram,GL_LINK_STATUS,&meshLinked);assert(meshLinked);
 sprite_pbr::meshes=false;beginModelMaterials(top,false);bindModelMaterial(whiteTexture,range.bounds,false);assert(program==unifiedMeshProgram);modelColorPass=false;
 // Cached receiver must be pixel-identical to a full setup, including program
 // switches, changed eye matrices and reflection passes.
 glUseProgram(unifiedMeshProgram);glUniform1i(glGetUniformLocation(unifiedMeshProgram,"detailTex"),1);glUniform1i(glGetUniformLocation(unifiedMeshProgram,"tex"),0);
 auto meshPixels=[&](bool reset,bool switchProgram){
  if(reset)unifiedMeshPrepared=false;
  if(switchProgram)useSceneProgram(lightweightProgram,modelMVP,modelReflected);
  bindModelMaterial(whiteTexture,range.bounds,false);
  glBindFramebuffer(GL_FRAMEBUFFER,0);glViewport(0,0,512,512);glDisable(GL_DEPTH_TEST);glDisable(GL_BLEND);glDisable(GL_CULL_FACE);glClearColor(0,0,0,0);glClear(GL_COLOR_BUFFER_BIT);
  glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,whiteTexture);attributes(terrainVBO);glVertexAttrib4f(9,0,0,0,1);glDrawArrays(GL_TRIANGLES,0,6);
  std::vector<unsigned char> pixels(512*512*4);glReadPixels(0,0,512,512,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());assert(glGetError()==GL_NO_ERROR);return pixels;
 };
 reflectionLevel=-1;std::vector<unsigned char> previousEye;
 for(bool reflected:{false,true})for(int eye=0;eye<2;eye++){
  Mat eyeMatrix=top;eyeMatrix[12]=eye*.3f;beginModelMaterials(eyeMatrix,reflected);
  auto expected=meshPixels(true,false);unsigned prepares=unifiedMeshPreparations;
  assert(meshPixels(false,false)==expected);assert(meshPixels(false,true)==expected);assert(unifiedMeshPreparations==prepares);
  for(int i=0;i<1000;i++)bindModelMaterial(whiteTexture,range.bounds,false);
  assert(unifiedMeshPreparations==prepares);
  if(eye==0)previousEye=expected;else assert(previousEye!=expected);
 }
 modelColorPass=false;puts("PASS unified model cache: identical pixels, material switches, two eyes/reflections, 1000 draws with no repeated setup");
 // Nonphysical blended effects stay excluded, physical scenery stays included.
 objects[0].objectType=0;snapshotRevision++;updateSunShadows();assert(spriteCasterCount==0);
 objects[0].objectType=2;snapshotRevision++;updateSunShadows();assert(spriteCasterCount==1);

 // Animated geometry uses the same live map and follows changed model poses.
 objects.clear();modelPosesValid=true;glGenBuffers(1,&vbo);
 ModelPose pose{};pose.geometry=99;pose.owner=1;pose.matrix[0]=pose.matrix[4]=pose.matrix[8]=512;
 for(auto v:plane){v.x*=256;v.y=v.z*256;v.z=307;animatedShapes[99][whiteTexture].push_back(v);}
 modelPoses={pose};snapshotRevision++;updateSunShadows();assert(sample(true,true,256,128)<180);
 modelPoses[0].pos[0]=4096;snapshotRevision++;updateSunShadows();assert(sample(true,true,256,128)>250);
 // Production surface updates remove original masks in unified mode, retain
 // persistent color/shade and additive lights, and restore masks when disabled.
 terrain.width=terrain.height=8;terrain.heights.assign(64,0);detailShadowsEnabled=false;
 auto&g=groundEffects;g={};g.w=g.h=64;g.indices.assign(4096,7);g.base.assign(4096,180);g.shade=g.base;g.lut.resize(256*256*4,255);g.initShadow();
 g.masks[99]={2,0,0,16,16,std::vector<uint8_t>(256,80)};
 g.masks[1]={0,0,0,16,16,std::vector<uint8_t>(256,100)};
 groundFrames[(uint64_t(1)<<32)]=1;tree.x=tree.y=1;objects={tree};modelPoses={pose};
 LiveLight light{};light.source=1;light.domain=1;light.key=1;light.w=light.h=1;light.alpha=65535;light.heightMode=1;light.pixels={20};liveLights={light};
 glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,whiteTexture);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,64,64,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
 glGenTextures(1,&waterShadowTexture);glBindTexture(GL_TEXTURE_2D,waterShadowTexture);glTexImage2D(GL_TEXTURE_2D,0,GL_R8,64,64,0,GL_RED,GL_UNSIGNED_BYTE,nullptr);
 unifiedShadowsEnabled=false;snapshotRevision++;updateSurfaceEffects();assert(g.previous.size()==3);
 unifiedShadowsEnabled=true;snapshotRevision++;updateSurfaceEffects();assert(g.previous.size()==1&&g.shade[8*64+8]==180&&g.shade[0]>180&&g.indices[0]==7&&g.base[0]==180);
 enhancedShadowsEnabled=false;snapshotRevision++;updateSurfaceEffects();assert(g.previous.size()==3&&g.shade[8*64+8]<180);
 // Base-color output cancels baked shade but preserves changed indices and light deltas.
 g.unlitPalette.assign(1024,120);g.referenceShade.assign(4096,180);for(int sh=0;sh<256;sh++)for(int ix=0;ix<256;ix++)for(int k=0;k<4;k++)g.lut[(sh*256+ix)*4+k]=k==3?255:sh;
 g.base.assign(4096,180);g.shade=g.base;g.shade[0]=200;g.shade[1]=140;std::vector<uint8_t> output;g.rgba(0,output);assert(output[0]==140&&output[4]==80&&output[8]==120&&output[3]==255);
 errors("surface modes");

 // Optional integration fixture: exercise the actual Android native loader with
 // the exported Willow Creek pack staged by UnlitLevelTest.
 const char* fixture="/data/local/tmp/unified2-level-test/external/LevelPacks/unlit/levels/6c312020/";
 FILE* fixtureCheck=fopen((std::string(fixture)+"terrain-base.pack").c_str(),"rb");
 if(fixtureCheck){fclose(fixtureCheck);levelExternalRoot=fixture;terrain.width=224;terrain.height=192;
  ANativeActivity activity{};activity.internalDataPath="/data/local/tmp/unified2-level-test/internal";android_app app{};app.activity=&activity;
  glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,textures[0]);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,1792,1536,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
  enhancedShadowsEnabled=unifiedShadowsEnabled=true;loadSurfaceEffects(&app);assert(unifiedBaseReady&&groundEffects.unlitPalette.size()==1024);
  GLuint readFbo;glGenFramebuffers(1,&readFbo);glBindFramebuffer(GL_FRAMEBUFFER,readFbo);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,textures[0],0);assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);unsigned char first[4];glReadPixels(0,0,1,1,GL_RGBA,GL_UNSIGNED_BYTE,first);assert(!memcmp(first,groundEffects.unlitPalette.data()+groundEffects.indices[0]*4,4));
  unifiedShadowsEnabled=false;loadSurfaceEffects(&app);assert(!unifiedBaseReady&&groundEffects.unlitPalette.empty());glBindFramebuffer(GL_FRAMEBUFFER,0);glDeleteFramebuffers(1,&readFbo);
  puts("PASS native Willow Creek base loader, GPU texture contents, mode OFF legacy selection");
 }
 assert(glGetError()==GL_NO_ERROR);puts("PASS unified ground receiver, mode gating, blended tree caster, all five resolutions, full static shadows, mesh routing, unlit surface updates, scene correction shader");
}

