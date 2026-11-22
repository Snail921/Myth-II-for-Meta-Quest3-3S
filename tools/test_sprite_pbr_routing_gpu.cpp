// Exercise the production stream/program switching code on Quest GLES.
#include "../src/vr/main.cpp"
#include <cassert>
int main(){
 auto d=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(d,0,0));EGLint attrs[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};EGLConfig cfg;EGLint n;assert(eglChooseConfig(d,attrs,&cfg,1,&n)&&n);EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE},pa[]={EGL_WIDTH,32,EGL_HEIGHT,16,EGL_NONE};auto ctx=eglCreateContext(d,cfg,EGL_NO_CONTEXT,ca);auto surface=eglCreatePbufferSurface(d,cfg,pa);assert(eglMakeCurrent(d,surface,surface,ctx));
 assert(!sprite_pbr::units&&!sprite_pbr::sceneObjects);
 objects.resize(3);objects[0].objectType=1;objects[0].unitID=42;objects[1].objectType=2;objects[2].objectType=4;
 sprite_pbr::units=true;sprite_pbr::sceneObjects=false;
 assert(spritePbrCategoryEnabled(1)&&spritePbrCategoryEnabled(3)&&!spritePbrCategoryEnabled(2)&&!spritePbrCategoryEnabled(0)&&!spritePbrCategoryEnabled(99));
 sprite_pbr::sceneObjects=true;sprite_pbr::minimumSss=0; // Directional-only cases below.
 enhancedSpritePath=true;enhancedLightingEnabled=enhancedShadowsEnabled=false;diagnosticEnabled=false;fogEnabled=false;fogCenter={0,0,2};camera={0,0,2};
 for(bool full:{false,true}){
 auto make=[&](bool pbr){GLuint p=glCreateProgram();auto vs=spriteShaderSource(mythVertexShader,full,full,true,pbr),fs=spriteShaderSource(mythFragmentShader,full,full,true,pbr);auto v=shader(GL_VERTEX_SHADER,vs.c_str()),f=shader(GL_FRAGMENT_SHADER,fs.c_str());glAttachShader(p,v);glAttachShader(p,f);glLinkProgram(p);GLint ok;glGetProgramiv(p,GL_LINK_STATUS,&ok);assert(ok);glUseProgram(p);glUniform1i(glGetUniformLocation(p,"tex"),0);glUniform1i(glGetUniformLocation(p,"detailTex"),1);return p;};
 billboardProgram=make(false);billboardPbrProgram=make(true);assert(glGetUniformLocation(billboardProgram,"spriteSkyMap")==-1);assert(glGetUniformLocation(billboardProgram,"spritePbrStrength")==-1);
 GLuint maps[5];glGenTextures(5,maps);auto upload=[&](int index,unsigned char r,unsigned char g,unsigned char b,unsigned char a){unsigned char px[]={r,g,b,a};glBindTexture(GL_TEXTURE_2D,maps[index]);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,px);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);};
 upload(0,128,128,128,255);upload(1,128,128,128,255);upload(2,128,128,255,255);upload(3,128,128,255,255);upload(4,0,0,255,255);
 sprite_material::Binding packed{maps[2],1,1,0,0,true,5},legacy{maps[3],1,1,0,0,false,0};sprite_material::assign(maps[0],legacy);sprite_material::assign(maps[1],packed);sprite_material::textureDimensions[maps[2]]={1,1};sprite_material::textureDimensions[maps[3]]={1,1};
 auto sky=[&](){unsigned char rgb[]={0,0,255,255};sprite_pbr::reflection=1;sprite_pbr::specular=sprite_pbr::sss=0;sprite_pbr::setSky(maps[4],1,1,rgb,0);};sky();
 Mat m={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};Vertex left[]={{-1,-1,0,0,0},{0,-1,0,1,0},{-1,1,0,0,1},{-1,1,0,0,1},{0,-1,0,1,0},{0,1,0,1,1}},right[]={{0,-1,0,0,0},{1,-1,0,1,0},{0,1,0,0,1},{0,1,0,0,1},{1,-1,0,1,0},{1,1,0,1,1}};
 glGenVertexArrays(1,&vao);glBindVertexArray(vao);glVertexAttrib4f(9,0,0,0,1);glViewport(0,0,32,16);
 auto pixels=[&](bool blue){unsigned char a[4],b[4];glReadPixels(8,8,1,1,GL_RGBA,GL_UNSIGNED_BYTE,a);glReadPixels(24,8,1,1,GL_RGBA,GL_UNSIGNED_BYTE,b);assert(a[0]>45&&a[0]<65);if(blue)assert(b[0]<3&&b[2]>45);else assert(abs(int(a[0])-int(b[0]))<=1);assert(glGetError()==GL_NO_ERROR);};
 for(bool atlas:{false,true}){
 sprite_atlas::enabled=atlas;for(int i=0;i<2;i++)sprite_atlas::registerTexture(maps[i],1,1,1);
 // Same source and atlas page, different categories: never merge opposite
 // permissions into one PBR draw. Exercise all four toggle combinations.
 for(bool units:{false,true})for(bool scenery:{false,true})for(bool reflected:{false,true}){
  sprite_pbr::units=units;sprite_pbr::sceneObjects=scenery;sky();reflectionLevel=-100;
  SpriteStream categories;std::vector<SpriteStream::Input> mixed{{maps[1],1,left,6},{maps[1],2,right,6}};categories.uploadReferences(mixed);
  beginSpritePrograms(m,reflected);glClear(GL_COLOR_BUFFER_BIT);categories.draw(reflected);
  unsigned char a[4],b[4];glReadPixels(8,8,1,1,GL_RGBA,GL_UNSIGNED_BYTE,a);glReadPixels(24,8,1,1,GL_RGBA,GL_UNSIGNED_BYTE,b);
  assert(units?a[0]<3:(a[0]>45&&a[0]<65));assert(scenery?b[0]<3:(b[0]>45&&b[0]<65));
  assert(units==scenery||categories.ranges.size()==2);assert(glGetError()==GL_NO_ERROR);
 }
 sprite_pbr::units=sprite_pbr::sceneObjects=true;
 SpriteStream sprites;std::vector<SpriteStream::Input> input{{maps[0],1,left,6},{maps[1],2,right,6}};sprites.uploadReferences(input);
 for(int mode=0;mode<3;mode++){sky();if(mode==1)sprite_pbr::reflection=0;if(mode==2)sprite_pbr::clearSky();diagnosticEnabled=true;sprite_material::drawStats[0]={};beginSpritePrograms(m,false);glClear(GL_COLOR_BUFFER_BIT);sprites.draw(false);pixels(mode==0);assert(sprite_material::drawStats[0].pbr==(mode==0?1u:0u));assert(sprite_material::drawStats[0].bypass==(mode==0?1u:2u));assert(sprite_material::drawStats[0].vertices==(mode==0?6u:0u));diagnosticEnabled=false;if(mode){assert(program==billboardProgram);assert(!spriteProgramReady[1]);}}
 for(bool reflected:{false,true}){sky();reflectionLevel=-100;beginSpritePrograms(m,reflected);glClear(GL_COLOR_BUFFER_BIT);glEnable(GL_BLEND);glBlendFunc(reflected?GL_ONE:GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);ParticleStream particles;particles.append(maps[1],.5f,right,6,1);particles.append(maps[0],.5f,left,6,1);particles.append(maps[1],.5f,right,6,1);particles.upload();particles.draw(glGetUniformLocation(program,"tint"),reflected);unsigned char a[4],b[4];glReadPixels(8,8,1,1,GL_RGBA,GL_UNSIGNED_BYTE,a);glReadPixels(24,8,1,1,GL_RGBA,GL_UNSIGNED_BYTE,b);assert(a[0]>20&&a[0]<35&&b[0]<3&&b[2]>35&&b[2]<50);assert(glGetError()==GL_NO_ERROR);glDisable(GL_BLEND);}
 // Local specular works without a sky, and respects the light sphere and shader toggle.
 {sprite_pbr::clearSky();sprite_pbr::reflection=sprite_pbr::sss=0;
 SpriteStream localStream;std::vector<SpriteStream::Input> one{{maps[1],2,right,6}};localStream.uploadReferences(one);
 auto sample=[&](float strength,float inverseRadius2,float x,bool debug=false){
  sprite_pbr::specular=strength;beginSpritePrograms(m,false);selectSpriteProgram(true);sprite_pbr::localLights=1;
  glUniform1i(glGetUniformLocation(program,"spriteSpecularDebug"),debug);
  glUniform1i(glGetUniformLocation(program,"spriteLightCount"),1);
  glUniform4f(glGetUniformLocation(program,"spriteLights[0]"),x,0,2,inverseRadius2);
  glUniform4f(glGetUniformLocation(program,"spriteLightAxes[0]"),0,0,0,0);
  glUniform4f(glGetUniformLocation(program,"spriteLightColors[0]"),1,0,0,1);
  glUniform1i(glGetUniformLocation(program,"spriteObjectLightLimit"),8);
  glClear(GL_COLOR_BUFFER_BIT);localStream.draw(false);unsigned char px[4];glReadPixels(24,8,1,1,GL_RGBA,GL_UNSIGNED_BYTE,px);assert(glGetError()==GL_NO_ERROR);if(debug){assert(px[1]==0&&px[2]==0);assert((px[0]>0)==(full&&inverseRadius2<1));}return int(px[0]);
 };
 // Tiny nonzero strength keeps the same PBR/diffuse path for the baseline.
 int base=sample(.00001f,.01f,.5f),lit=sample(4.f,.01f,.5f);
 sample(4.f,.01f,.5f,true);sample(4.f,4.f,.5f,true);
 int outsideBase=sample(.00001f,4.f,.5f),outside=sample(4.f,4.f,.5f);
 fprintf(stderr,"LOCAL full=%d atlas=%d base=%d lit=%d outside=%d/%d\n",full,atlas,base,lit,outsideBase,outside);assert(full?lit>base+5:abs(lit-base)<=1);assert(abs(outside-outsideBase)<=1);
 sprite_pbr::localLights=0;sky();sprite_pbr::sampleSun(waterSun);
 printf("PASS local specular full=%d atlas=%d base=%d lit=%d outside=%d/%d\n",full,atlas,base,lit,outsideBase,outside);
 }
 // Zero M&S retains diffuse lighting but contributes no sun/local specular.
 {sprite_atlas::enabled=false;sprite_pbr::clearSky();sprite_pbr::reflection=sprite_pbr::sss=0;sprite_pbr::specular=4;
  upload(2,128,128,255,0);sprite_material::assign(maps[1],{maps[2],1,1,0,0,true,0});
  SpriteStream z;std::vector<SpriteStream::Input> inputZero{{maps[1],2,right,6}};z.uploadReferences(inputZero);
  assert(!sprite_material::needsPbr(sprite_material::get(maps[1])));
  sprite_pbr::debugSpecular=true;assert(sprite_material::needsPbr(sprite_material::get(maps[1])));
  beginSpritePrograms(m,false);selectSpriteProgram(true);glUniform3f(glGetUniformLocation(program,"spriteSunRadiance"),1,1,1);glUniform3f(glGetUniformLocation(program,"spriteSunDirection"),0,0,1);
  glUniform1i(glGetUniformLocation(program,"spriteLightCount"),1);glUniform4f(glGetUniformLocation(program,"spriteLights[0]"),.5f,0,2,.01f);glUniform4f(glGetUniformLocation(program,"spriteLightColors[0]"),1,1,1,1);
  glClear(GL_COLOR_BUFFER_BIT);z.draw(false);unsigned char px[4];glReadPixels(24,8,1,1,GL_RGBA,GL_UNSIGNED_BYTE,px);assert(px[0]==0&&px[1]==0&&px[2]==0&&px[3]>0);
  sprite_pbr::debugSpecular=false;upload(2,128,128,255,255);sprite_material::assign(maps[1],packed);sprite_atlas::enabled=atlas;sky();sprite_pbr::sampleSun(waterSun);
 }
 // SSS debug retains true colored transmission and strength, with thick material black.
 {sprite_atlas::enabled=false;sprite_pbr::debugSss=true;sprite_pbr::reflection=0;sprite_pbr::specular=4;
  auto sampleSss=[&](unsigned char thickness,float strength,float lightX=0){
   upload(2,128,128,thickness,0);sprite_material::assign(maps[1],{maps[2],1,1,0,0,true,0});
   assert(sprite_material::needsPbr(sprite_material::get(maps[1])));
   sprite_pbr::sss=strength;SpriteStream z;std::vector<SpriteStream::Input> inputs{{maps[1],2,right,6}};z.uploadReferences(inputs);
   beginSpritePrograms(m,false);selectSpriteProgram(true);
   glUniform3f(glGetUniformLocation(program,"spriteSunRadiance"),1,0,0);glUniform3f(glGetUniformLocation(program,"spriteSunDirection"),lightX,0,-1);
   glClear(GL_COLOR_BUFFER_BIT);z.draw(false);unsigned char px[4];glReadPixels(24,8,1,1,GL_RGBA,GL_UNSIGNED_BYTE,px);
   assert(px[1]==0&&px[2]==0&&px[3]>0);assert(glGetError()==GL_NO_ERROR);return int(px[0]);
  };
  int thin=sampleSss(0,.25f),strong=sampleSss(0,.5f),thick=sampleSss(255,2),off=sampleSss(0,0);
  assert(thin>0&&strong>thin&&thick==0&&off==0);
  assert(sampleSss(0,4,2)==0); // Off-axis despite a back-facing normal and maximum strength.
  printf("PASS SSS debug full=%d atlas=%d thin=%d strong=%d thick=%d off=%d\n",full,atlas,thin,strong,thick,off);
  sprite_pbr::debugSss=false;upload(2,128,128,255,255);sprite_material::assign(maps[1],packed);sprite_atlas::enabled=atlas;sky();sprite_pbr::sampleSun(waterSun);
 }
 // Local SSS: backlights only, true color/strength, range and material gating, no sky.
 {sprite_atlas::enabled=false;sprite_pbr::clearSky();sprite_pbr::reflection=sprite_pbr::specular=0;
  auto sampleLocalSss=[&](unsigned char thickness,unsigned char metal,float strength,float invRange,float z,bool debug,bool segment=false,float lightX=.5f){
   upload(2,128,128,thickness,metal);sprite_material::assign(maps[1],{maps[2],1,1,0,0,true,2});
   sprite_pbr::sss=strength;sprite_pbr::debugSss=debug;
   SpriteStream stream;std::vector<SpriteStream::Input> inputs{{maps[1],2,right,6}};stream.uploadReferences(inputs);
   beginSpritePrograms(m,false);selectSpriteProgram(true);sprite_pbr::localLights=1;
   if(strength>0)assert(sprite_material::needsPbr(sprite_material::get(maps[1])));
   glUniform3f(glGetUniformLocation(program,"spriteSunRadiance"),0,0,0);
   glUniform1i(glGetUniformLocation(program,"spriteLightCount"),1);
   glUniform4f(glGetUniformLocation(program,"spriteLights[0]"),segment?-2.f:lightX,0,z,invRange);
   glUniform4f(glGetUniformLocation(program,"spriteLightAxes[0]"),segment?4.f:0.f,0,0,segment?1.f/16.f:0.f);
   glUniform4f(glGetUniformLocation(program,"spriteLightColors[0]"),0,1,0,1);
   glUniform1i(glGetUniformLocation(program,"spriteObjectLightLimit"),8);
   glClear(GL_COLOR_BUFFER_BIT);stream.draw(false);unsigned char px[4];glReadPixels(24,8,1,1,GL_RGBA,GL_UNSIGNED_BYTE,px);
   if(debug)assert(px[0]==0&&px[2]==0);assert(glGetError()==GL_NO_ERROR);return int(px[1]);
  };
  int thin=sampleLocalSss(0,0,.25f,.01f,-2,true),strong=sampleLocalSss(0,0,.5f,.01f,-2,true);
  assert(full?(thin>0&&strong>thin):(thin==0&&strong==0));
  assert(sampleLocalSss(255,0,2,.01f,-2,true)==0);assert(sampleLocalSss(0,255,2,.01f,-2,true)==0);
  assert(sampleLocalSss(0,0,0,.01f,-2,true)==0);assert(sampleLocalSss(0,0,2,4,-2,true)==0);
  assert(sampleLocalSss(0,0,2,.01f,2,true)==0);
  assert(sampleLocalSss(0,0,4,.001f,-2,true,false,5)==0);
  int segment=sampleLocalSss(0,0,1,.01f,-2,true,true);assert(full?segment>0:segment==0);
  int baseline=sampleLocalSss(0,0,.00001f,.01f,-2,false),lit=sampleLocalSss(0,0,1,.01f,-2,false);
  assert(full?lit>baseline:lit==baseline);
  printf("PASS local SSS full=%d atlas=%d thin=%d strong=%d segment=%d normal=%d/%d\n",full,atlas,thin,strong,segment,baseline,lit);
  sprite_pbr::debugSss=false;sprite_pbr::localLights=0;upload(2,128,128,255,255);sprite_material::assign(maps[1],packed);sprite_atlas::enabled=atlas;sky();sprite_pbr::sampleSun(waterSun);
 }
 // Material summary tests: no reflection for all-zero M&S, no SSS for thick material.
 auto zero=packed;zero.effects=0;assert(!sprite_material::needsPbr(zero));sprite_pbr::reflection=0;sprite_pbr::sss=1;assert(!sprite_material::needsPbr(packed));sprite_pbr::specular=1;assert(sprite_material::needsPbr(packed));assert(!sprite_material::needsPbr(packed,false));assert(!sprite_material::needsPbr(legacy));
 printf("PASS production sprite/particle switching, atlas=%d full=%d, disabled/missing-sky/material bypass, alpha/reflection order\n",atlas,full);
 }
}
}
