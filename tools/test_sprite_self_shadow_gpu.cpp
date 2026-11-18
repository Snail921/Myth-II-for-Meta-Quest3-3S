#include "../src/vr/main.cpp"
#include <cassert>
int main(){
 EGLDisplay d=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(d,0,0));EGLint a[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};EGLConfig cfg;EGLint n;assert(eglChooseConfig(d,a,&cfg,1,&n)&&n);EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE},pa[]={EGL_WIDTH,128,EGL_HEIGHT,128,EGL_NONE};auto c=eglCreateContext(d,cfg,EGL_NO_CONTEXT,ca);auto s=eglCreatePbufferSurface(d,cfg,pa);assert(eglMakeCurrent(d,s,s,c));
 glGenVertexArrays(1,&vao);glBindVertexArray(vao);program=glCreateProgram();glAttachShader(program,shader(GL_VERTEX_SHADER,mythVertexShader));glAttachShader(program,shader(GL_FRAGMENT_SHADER,mythFragmentShader));glLinkProgram(program);
 GLuint t;glGenTextures(1,&t);glBindTexture(GL_TEXTURE_2D,t);unsigned char white[]={255,255,255,128};glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,white);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);

 glUseProgram(program);glGenBuffers(1,&vbo);Mat m{};m[0]=m[5]=m[10]=m[15]=1;glUniformMatrix4fv(glGetUniformLocation(program,"mvp"),1,GL_FALSE,m.data());glUniform1i(glGetUniformLocation(program,"tex"),0);glUniform1i(glGetUniformLocation(program,"detailTex"),1);glUniform1i(glGetUniformLocation(program,"detailEnabled"),0);glUniform1i(glGetUniformLocation(program,"softOnly"),0);fogEnabled=false;applyFog(program,false);glViewport(0,0,128,128);


 waterStaticDepth=spriteLiveDepth=spriteLiveOwner=t;waterSunShadowReady=enhancedShadowsEnabled=true;
 // White dummy depth makes every point unoccluded; isolate rounded-body shading.
 unsigned char opaque[]={255,255,255,255};glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,t);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,opaque);
 waterSunMatrix={};glUniform3f(glGetUniformLocation(program,"fogEye"),0,0,2);
 auto render=[&](bool mirror,float amount,Vec3 sun,bool on){
  enhancedShadowAmount=amount;enhancedShadowsEnabled=on;waterSun[0]=sun.x;waterSun[1]=sun.y;waterSun[2]=sun.z;bindSpriteShadows();
  float u0=mirror?1:0,u1=1-u0;Vertex a{-.8f,.8f,0,u0,0},b{.8f,.8f,0,u1,0},c{-.8f,-.8f,0,u0,1},dd{.8f,-.8f,0,u1,1},v[]={a,b,c,b,dd,c};
  attributes(vbo);glVertexAttrib1f(4,1);glBufferData(GL_ARRAY_BUFFER,sizeof(v),v,GL_STREAM_DRAW);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,t);glUniform4f(glGetUniformLocation(program,"tint"),1,1,1,1);glClear(GL_COLOR_BUFFER_BIT);glDrawArrays(GL_TRIANGLES,0,6);
  std::array<int,2> result{};unsigned char pixel[4];glReadPixels(32,64,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);result[0]=pixel[0];glReadPixels(96,64,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);result[1]=pixel[0];assert(glGetError()==GL_NO_ERROR);return result;
 };
 auto side=render(false,.45,{1,.2,0},true),mirror=render(true,.45,{1,.2,0},true);
 assert(side[0]<120&&side[1]>245);assert(abs(side[0]-mirror[0])<=1&&abs(side[1]-mirror[1])<=1);
 auto front=render(false,.45,{0,.2,1},true),back=render(false,.45,{0,.2,-1},true);
 assert(front[0]>245&&front[1]>245);assert(back[0]>=65&&back[0]<=69&&back[1]>=65&&back[1]<=69);
 for(auto value:{render(false,0,{0,0,-1},true),render(false,1,{0,0,-1},false)})assert(value[0]>250&&value[1]>250);
 auto black=render(false,1,{0,0,-1},true);assert(black[0]<2&&black[1]<2);
 // Stacked external and self shadow must remain at the configured maximum.
 unsigned char blocked[]={0,0,0,255};GLuint depth;glGenTextures(1,&depth);glBindTexture(GL_TEXTURE_2D,depth);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,blocked);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);waterStaticDepth=depth;
 auto stacked=render(false,.45,{0,0,-1},true);assert(stacked==back);
 printf("PASS self shading side=%d/%d mirror=%d/%d front=%d/%d back=%d/%d; 0/45/100%%, OFF, stacking cap, GL_NO_ERROR\n",side[0],side[1],mirror[0],mirror[1],front[0],front[1],back[0],back[1]);
}
