// Exercise the production stream/program switching code on Quest GLES.
#include "../src/vr/main.cpp"
#include <cassert>
int main(){
 auto d=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(d,0,0));EGLint attrs[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};EGLConfig cfg;EGLint n;assert(eglChooseConfig(d,attrs,&cfg,1,&n)&&n);EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE},pa[]={EGL_WIDTH,32,EGL_HEIGHT,16,EGL_NONE};auto ctx=eglCreateContext(d,cfg,EGL_NO_CONTEXT,ca);auto surface=eglCreatePbufferSurface(d,cfg,pa);assert(eglMakeCurrent(d,surface,surface,ctx));
 assert(!sprite_pbr::meshes);sprite_pbr::meshes=true;
 enhancedSpritePath=true;enhancedLightingEnabled=enhancedShadowsEnabled=false;diagnosticEnabled=false;fogEnabled=false;fogCenter={0,0,2};camera={0,0,2};

 for(bool shadows:{false,true})for(bool lighting:{false,true}){
 auto make=[&](bool mesh){auto vs=mesh?modelShaderSource(mythVertexShader,shadows,lighting):spriteShaderSource(mythVertexShader,false,false,false,false);auto fs=mesh?modelShaderSource(mythFragmentShader,shadows,lighting):spriteShaderSource(mythFragmentShader,false,false,false,false);if(mesh){assert(fs.find("float side=clamp")==std::string::npos);assert(fs.find("fract(uv)")!=std::string::npos);}GLuint p=glCreateProgram();glAttachShader(p,shader(GL_VERTEX_SHADER,vs.c_str()));glAttachShader(p,shader(GL_FRAGMENT_SHADER,fs.c_str()));glLinkProgram(p);GLint ok;glGetProgramiv(p,GL_LINK_STATUS,&ok);assert(ok);glUseProgram(p);glUniform1i(glGetUniformLocation(p,"detailTex"),1);return p;};
 lightweightProgram=make(false);modelPbrProgram=make(true);sprite_pbr::clearSky();sprite_pbr::reflection=sprite_pbr::specular=0;sprite_pbr::sss=1;sprite_pbr::debugSss=true;
 GLuint maps[2];glGenTextures(2,maps);unsigned char color[]={128,128,128,255},material[]={128,128,0,0};
 for(int i=0;i<2;i++){glBindTexture(GL_TEXTURE_2D,maps[i]);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,i?material:color);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);}
 sprite_material::assign(maps[0],{maps[1],1,1,0,0,true,2});
 glGenVertexArrays(1,&vao);glBindVertexArray(vao);glGenBuffers(1,&vbo);Vertex vertices[]={{-1,-1,0,0,0},{1,-1,0,1,0},{-1,1,0,0,1}};attributes(vbo);glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STATIC_DRAW);rendercull::Bounds bounds;bounds.add(-1,-1,0);bounds.add(1,1,0);Mat m={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
 beginModelMaterials(m,false);attributes(vbo);bindModelMaterial(maps[0],bounds,true);assert(program==modelPbrProgram);
 glUniform3f(glGetUniformLocation(program,"spriteSunRadiance"),1,0,0);glUniform3f(glGetUniformLocation(program,"spriteSunDirection"),0,0,-1);glUniform3f(glGetUniformLocation(program,"fogEye"),0,0,2);glUniform1i(glGetUniformLocation(program,"tex"),0);glUniform1i(glGetUniformLocation(program,"spriteShadows"),0);glViewport(0,0,16,16);glBindTexture(GL_TEXTURE_2D,maps[0]);glClear(GL_COLOR_BUFFER_BIT);glDrawArrays(GL_TRIANGLES,0,3);unsigned char px[4];glReadPixels(6,6,1,1,GL_RGBA,GL_UNSIGNED_BYTE,px);fprintf(stderr,"PIX %u %u %u %u err=%x\n",px[0],px[1],px[2],px[3],glGetError());assert(px[0]>0&&px[1]==0&&px[2]==0);

 // Off-axis backlight: zero restores the directional-only version; the floor
 // scales transmission, never opacity or the artist's thickness value.
 auto readRed=[&](){glClear(GL_COLOR_BUFFER_BIT);glDrawArrays(GL_TRIANGLES,0,3);glReadPixels(6,6,1,1,GL_RGBA,GL_UNSIGNED_BYTE,px);return int(px[0]);};
 glUniform3f(glGetUniformLocation(program,"spriteSunDirection"),1,0,-.5f);
 glUniform1f(glGetUniformLocation(program,"spriteSssMinimum"),0);assert(readRed()==0);
 glUniform1f(glGetUniformLocation(program,"spriteSssMinimum"),.35f);int floor35=readRed();assert(floor35>0);
 glUniform1f(glGetUniformLocation(program,"spriteSssMinimum"),1);int floor100=readRed();assert(floor100>floor35&&abs(floor35-floor100*.35f)<2);
 glActiveTexture(GL_TEXTURE11);glBindTexture(GL_TEXTURE_2D,maps[1]);material[2]=255;glTexSubImage2D(GL_TEXTURE_2D,0,0,0,1,1,GL_RGBA,GL_UNSIGNED_BYTE,material);glActiveTexture(GL_TEXTURE0);assert(readRed()==0);
 glActiveTexture(GL_TEXTURE11);material[2]=0;glTexSubImage2D(GL_TEXTURE_2D,0,0,0,1,1,GL_RGBA,GL_UNSIGNED_BYTE,material);glActiveTexture(GL_TEXTURE0);
 glUniform1f(glGetUniformLocation(program,"spriteSssMinimum"),.35f);
 glUniform3f(glGetUniformLocation(program,"spriteSunDirection"),0,0,-1);

 if(shadows){
  // A building fully occludes the receiver in sun space. Artistic shadow
  // darkness must not leave 55% transmission behind at the default setting.
  GLuint depth[2],fb;glGenTextures(2,depth);glGenFramebuffers(1,&fb);
  for(int i=0;i<2;i++){
   glActiveTexture(GL_TEXTURE8+i);glBindTexture(GL_TEXTURE_2D,depth[i]);
   glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT24,16,16,0,GL_DEPTH_COMPONENT,GL_UNSIGNED_INT,nullptr);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
   glBindFramebuffer(GL_FRAMEBUFFER,fb);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,depth[i],0);
   GLenum none=GL_NONE;glDrawBuffers(1,&none);glReadBuffer(GL_NONE);assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);
   glClearDepthf(i?1.f:.25f);glClear(GL_DEPTH_BUFFER_BIT);
  }
  glBindFramebuffer(GL_FRAMEBUFFER,0);glActiveTexture(GL_TEXTURE0);
  glUniform1i(glGetUniformLocation(program,"spriteStaticDepth"),8);glUniform1i(glGetUniformLocation(program,"spriteLiveDepth"),9);
  glUniformMatrix4fv(glGetUniformLocation(program,"spriteSunMatrix"),1,GL_FALSE,m.data());
  glUniform1i(glGetUniformLocation(program,"spriteShadows"),1);glUniform1i(glGetUniformLocation(program,"spriteShadowSamples"),9);
  glUniform1f(glGetUniformLocation(program,"spriteShadowBias"),.001f);
  for(float amount:{0.f,.45f,1.f}){
   glUniform1f(glGetUniformLocation(program,"spriteShadowAmount"),amount);
   glClear(GL_COLOR_BUFFER_BIT);glDrawArrays(GL_TRIANGLES,0,3);glReadPixels(6,6,1,1,GL_RGBA,GL_UNSIGNED_BYTE,px);
   assert(px[0]==0&&px[1]==0&&px[2]==0);
  }
  glUniform1i(glGetUniformLocation(program,"spriteShadows"),0);
  glClear(GL_COLOR_BUFFER_BIT);glDrawArrays(GL_TRIANGLES,0,3);glReadPixels(6,6,1,1,GL_RGBA,GL_UNSIGNED_BYTE,px);assert(px[0]>0);
  glUniform1i(glGetUniformLocation(program,"spriteShadows"),1);
  // Leave occlusion ON for the local-light test: sunlight shadows must not
  // suppress unrelated enhanced lights.
 }

 if(lighting){
  glUniform3f(glGetUniformLocation(program,"spriteSunRadiance"),0,0,0);
  glUniform1i(glGetUniformLocation(program,"spriteLightCount"),1);
  glUniform4f(glGetUniformLocation(program,"spriteLights[0]"),0,0,-2,.01f);
  glUniform4f(glGetUniformLocation(program,"spriteLightAxes[0]"),0,0,0,0);
  glUniform4f(glGetUniformLocation(program,"spriteLightColors[0]"),0,1,0,1);
  glClear(GL_COLOR_BUFFER_BIT);glDrawArrays(GL_TRIANGLES,0,3);glReadPixels(6,6,1,1,GL_RGBA,GL_UNSIGNED_BYTE,px);assert(px[0]==0&&px[1]>0&&px[2]==0);
 }
 // Changing shader programs does not change the view's light selection.
 unsigned selections=spriteLightSelections;
 program=lightweightProgram;glUseProgram(program);bindSpriteLighting();assert(spriteLightSelections==selections);
 program=modelPbrProgram;glUseProgram(program);bindSpriteLighting();assert(spriteLightSelections==selections);
 // A new simulation snapshot must invalidate it.
 ++snapshotRevision;bindSpriteLighting();assert(spriteLightSelections==selections+1);
 // The mesh switch bypasses flagged materials in both eyes/reflections and
 // moving/static paths; it does not modify the material's sprite PBR flag.
 sprite_pbr::meshes=false;
 for(bool reflected:{false,true})for(bool moving:{false,true}){
  beginModelMaterials(m,reflected);program=modelPbrProgram;glUseProgram(program);
  attributes(vbo);bindModelMaterial(maps[0],bounds,moving);assert(program==lightweightProgram);
  assert(!modelPrepared&&sprite_material::get(maps[0]).packed);
 }
 sprite_pbr::meshes=true;beginModelMaterials(m,false);attributes(vbo);bindModelMaterial(maps[0],bounds,true);assert(program==modelPbrProgram);
 sprite_material::assign(maps[0],{maps[1],1,1,0,0,false,0});attributes(vbo);bindModelMaterial(maps[0],bounds,true);assert(program==lightweightProgram);
 modelColorPass=false;program=123;bindModelMaterial(maps[0],bounds,true);assert(program==123);assert(glGetError()==GL_NO_ERROR);
 printf("PASS mesh shader shadows=%d lights=%d routing, SSS, geometric normal, repeat UV, shadow-pass isolation\n",shadows,lighting);
 }
}
