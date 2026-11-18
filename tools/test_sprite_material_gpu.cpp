#include "../src/vr/main.cpp"
#include <cassert>
int main(){
 EGLDisplay d=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(d,0,0));EGLint a[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_NONE};EGLConfig cfg;EGLint n;assert(eglChooseConfig(d,a,&cfg,1,&n)&&n);EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE},pa[]={EGL_WIDTH,64,EGL_HEIGHT,64,EGL_NONE};auto ctx=eglCreateContext(d,cfg,EGL_NO_CONTEXT,ca);auto surf=eglCreatePbufferSurface(d,cfg,pa);assert(eglMakeCurrent(d,surf,surf,ctx));
 glGenVertexArrays(1,&vao);glBindVertexArray(vao);glGenBuffers(1,&vbo);program=glCreateProgram();glAttachShader(program,shader(GL_VERTEX_SHADER,mythVertexShader));glAttachShader(program,shader(GL_FRAGMENT_SHADER,mythFragmentShader));glLinkProgram(program);GLint linked=0;glGetProgramiv(program,GL_LINK_STATUS,&linked);assert(linked);glUseProgram(program);
 auto uniform=[&](const char*s){return glGetUniformLocation(program,s);};Mat m{};m[0]=m[5]=m[10]=m[15]=1;glUniformMatrix4fv(uniform("mvp"),1,GL_FALSE,m.data());glUniform1i(uniform("detailTex"),1);glUniform1i(uniform("tex"),0);glUniform3f(uniform("fogEye"),0,0,2);glUniform4f(uniform("tint"),1,1,1,1);glViewport(0,0,64,64);
 GLuint colorTex,normalTex;glGenTextures(1,&colorTex);glBindTexture(GL_TEXTURE_2D,colorTex);unsigned char gray[]={160,160,160,255};glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,gray);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
 glGenTextures(1,&normalTex);glBindTexture(GL_TEXTURE_2D,normalTex);unsigned char normals[]={24,128,200,255,232,128,200,255};glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,2,1,0,GL_RGBA,GL_UNSIGNED_BYTE,normals);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
 auto render=[&](bool mapped,bool mirror,bool shadows,bool lights,float owner=1.f){
  sprite_material::assign(colorTex,mapped?sprite_material::Binding{normalTex,1,1,0,0}:sprite_material::Binding{});sprite_material::bind(program,colorTex);
  glUniform1i(uniform("spriteShadows"),shadows);glUniform1f(uniform("spriteShadowAmount"),.8f);glUniform3f(uniform("spriteSunDirection"),1,0,0);Mat sun{};sun[12]=sun[13]=sun[14]=4;glUniformMatrix4fv(uniform("spriteSunMatrix"),1,GL_FALSE,sun.data());
  glUniform1i(uniform("spriteLightCount"),lights?1:0);glUniform4f(uniform("spriteLights[0]"),2,0,1,.01f);glUniform4f(uniform("spriteLightColors[0]"),1,1,1,1);
  float u=mirror?1:0;Vertex v[]={{-.8f,.8f,0,u,0},{.8f,.8f,0,1-u,0},{-.8f,-.8f,0,u,1},{.8f,.8f,0,1-u,0},{.8f,-.8f,0,1-u,1},{-.8f,-.8f,0,u,1}};
  attributes(vbo);glVertexAttrib1f(4,owner);glBufferData(GL_ARRAY_BUFFER,sizeof(v),v,GL_STREAM_DRAW);glBindTexture(GL_TEXTURE_2D,colorTex);glClear(GL_COLOR_BUFFER_BIT);glDrawArrays(GL_TRIANGLES,0,6);unsigned char l[4],r[4];glReadPixels(16,32,1,1,GL_RGBA,GL_UNSIGNED_BYTE,l);glReadPixels(48,32,1,1,GL_RGBA,GL_UNSIGNED_BYTE,r);assert(glGetError()==GL_NO_ERROR);return std::array<int,2>{l[0],r[0]};
 };
 auto sun=render(true,false,true,false),flip=render(true,true,true,false);assert(sun[1]>sun[0]+40);assert(abs(sun[0]-flip[0])<3&&abs(sun[1]-flip[1])<3);
 auto light=render(true,false,false,true);assert(light[1]>light[0]+15);
 auto off=render(true,false,false,false);assert(abs(off[0]-off[1])<2);
 auto fx=render(true,false,true,true,0);assert(fx==off);
 sprite_material::load("/data/local/tmp/material-test");assert(sprite_material::entries.size()==326);auto&e=sprite_material::entries[0];std::vector<unsigned char> source(size_t(e.w)*e.h*4);std::ifstream sourceFile(sprite_material::packs[e.pack],std::ios::binary);sourceFile.seekg(e.offset);sourceFile.read((char*)source.data(),source.size());auto found=sprite_material::match(e.w,e.h,source.data());assert(found.texture);auto bad=source;bad[0]^=1;assert(!sprite_material::match(e.w,e.h,bad.data()).texture);assert(!sprite_material::match(1,1,gray).texture);sprite_material::clear();assert(sprite_material::bindings.empty());
 printf("Normal GPU PASS sun=%d,%d flipped=%d,%d lighting=%d,%d disabled=%d,%d; 326-frame pack exact matching PASS\n",sun[0],sun[1],flip[0],flip[1],light[0],light[1],off[0],off[1]);
}
