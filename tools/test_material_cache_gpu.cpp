#include "../src/vr/main.cpp"
#include <cassert>
GLuint compile(GLenum type,const char*s){GLuint x=glCreateShader(type);glShaderSource(x,1,&s,0);glCompileShader(x);GLint ok;glGetShaderiv(x,GL_COMPILE_STATUS,&ok);if(!ok){char b[8192];glGetShaderInfoLog(x,sizeof(b),0,b);puts(b);}assert(ok);return x;}
int main(){auto d=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(d,0,0));EGLint a[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};EGLConfig cfg;EGLint n;assert(eglChooseConfig(d,a,&cfg,1,&n)&&n);EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE},pa[]={EGL_WIDTH,16,EGL_HEIGHT,16,EGL_NONE};auto ctx=eglCreateContext(d,cfg,EGL_NO_CONTEXT,ca);auto surface=eglCreatePbufferSurface(d,cfg,pa);assert(eglMakeCurrent(d,surface,surface,ctx));
for(bool enhanced:{false,true}){GLuint p=glCreateProgram();auto vs=spriteShaderSource(mythVertexShader,enhanced),fs=spriteShaderSource(mythFragmentShader,enhanced);glAttachShader(p,compile(GL_VERTEX_SHADER,vs.c_str()));glAttachShader(p,compile(GL_FRAGMENT_SHADER,fs.c_str()));glLinkProgram(p);GLint ok;glGetProgramiv(p,GL_LINK_STATUS,&ok);assert(ok);glUseProgram(p);auto u=[&](const char*s){return glGetUniformLocation(p,s);};GLint count;glGetProgramiv(p,GL_ACTIVE_UNIFORMS,&count);int unit=1;for(int i=0;i<count;i++){char name[128];GLint size;GLenum type;glGetActiveUniform(p,i,128,0,&size,&type,name);if(type==GL_SAMPLER_2D||type==GL_SAMPLER_2D_ARRAY)glUniform1i(u(name),strcmp(name,"tex")==0?0:unit++);}
float ident[]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};glUniformMatrix4fv(u("mvp"),1,GL_FALSE,ident);glUniform4f(u("tint"),1,1,1,1);glUniform1f(u("reflectionHeight"),-100);GLuint vao,vbo,tx;glGenVertexArrays(1,&vao);glBindVertexArray(vao);glGenBuffers(1,&vbo);glBindBuffer(GL_ARRAY_BUFFER,vbo);float verts[]={-1,-1,0,0,0, 1,-1,0,1,0, -1,1,0,0,1, -1,1,0,0,1, 1,-1,0,1,0, 1,1,0,1,1};glBufferData(GL_ARRAY_BUFFER,sizeof(verts),verts,GL_STATIC_DRAW);glEnableVertexAttribArray(0);glVertexAttribPointer(0,3,GL_FLOAT,0,20,0);glEnableVertexAttribArray(1);glVertexAttribPointer(1,2,GL_FLOAT,0,20,(void*)12);glGenTextures(1,&tx);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,tx);unsigned char pixels[]={255,255,255,64,255,255,255,192};glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,2,1,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);glViewport(0,0,16,16);
for(int reflection=0;reflection<2;reflection++){glUniform1f(u("reflectionClip"),reflection);for(int mode=0;mode<3;mode++){glClearColor(0,0,0,0);glClear(GL_COLOR_BUFFER_BIT);glUniform1i(u("spriteCutout"),mode==0);glUniform1i(u("softOnly"),mode==2);glUniform1f(u("alphaCutoff"),mode==0?.5f:mode==1?.995f:.003f);glDrawArrays(GL_TRIANGLES,0,6);unsigned char lo[4],hi[4];glReadPixels(4,8,1,1,GL_RGBA,GL_UNSIGNED_BYTE,lo);glReadPixels(12,8,1,1,GL_RGBA,GL_UNSIGNED_BYTE,hi);assert(glGetError()==GL_NO_ERROR);if(mode==0){assert(lo[3]==0&&hi[3]==255&&hi[0]>250);}if(mode==1)assert(lo[3]==0&&hi[3]==0);if(mode==2)assert(lo[3]>=63&&lo[3]<=65&&hi[3]>=191&&hi[3]<=193);}}
if(enhanced){
 sprite_material::bindings.clear();sprite_material::uniforms={};
 auto revision=sprite_material::bindingRevision;sprite_material::assign(100,{tx,1,1,0,0});assert(sprite_material::bindingRevision!=revision);sprite_material::assign(101,{tx,.5f,.75f,.1f,.2f});
 sprite_material::StreamBinding state;
 for(GLuint source:{100u,100u,101u,102u,102u,101u,100u}){
  auto b=sprite_material::get(source);state.bindResolved(p,b);GLint enabled;glGetUniformiv(p,u("spriteNormalEnabled"),&enabled);assert(enabled==int(b.texture!=0));
  if(b.texture){float uv[4];glGetUniformfv(p,u("spriteNormalUV"),uv);assert(uv[0]==b.sx&&uv[1]==b.sy&&uv[2]==b.ox&&uv[3]==b.oy);glActiveTexture(GL_TEXTURE11);GLint bound;glGetIntegerv(GL_TEXTURE_BINDING_2D,&bound);assert(GLuint(bound)==tx);glActiveTexture(GL_TEXTURE0);}
 }
 sprite_material::unbind(p);
 // New draw scope must restore state after another render path changed it.
 glUniform4f(u("spriteNormalUV"),9,9,9,9);sprite_material::StreamBinding next;next.bind(p,100);float uv[4];glGetUniformfv(p,u("spriteNormalUV"),uv);assert(uv[0]==1&&uv[2]==0);assert(glGetError()==GL_NO_ERROR);
 ::program=p;diagnosticEnabled=true;enhancedSpritePath=true;
 std::map<std::pair<GLuint,unsigned>,std::vector<Vertex>> batches;
 for(GLuint x=100;x<113;x++)batches[{x,1}]=std::vector<Vertex>((Vertex*)verts,(Vertex*)verts+6);
 sprite_material::bindings.clear();++sprite_material::bindingRevision;
 SpriteStream stream;stream.upload(batches);assert(stream.lightBounds.bounds.size()==78);
 // Empty pixels are irrelevant here: verify CPU cache transitions and grouping.
 streamCandidateRanges=streamCandidateGroups6=0;stream.draw(false);
 assert(streamCandidateRanges==13&&streamCandidateGroups6==3);auto rev=stream.materialRevision;
 stream.draw(true);assert(stream.materialRevision==rev&&streamCandidateRanges==13);
 for(GLuint x=100;x<113;x++)sprite_material::assign(x,{x,1,1,0,0});
 streamCandidateRanges=streamCandidateGroups6=0;stream.draw(false);
 assert(stream.materialRevision!=rev&&streamCandidateRanges==13&&streamCandidateGroups6==13);
 for(auto&r:stream.ranges)assert(r.material.texture==r.texture);
 sprite_material::assign(100,{});stream.draw(true);assert(stream.ranges[0].material.texture==0);
 assert(glGetError()==GL_NO_ERROR);
 puts("PASS material cache invalidation, stereo reuse, grouping estimate 3/13, shadow LOD retained");
 puts("PASS stream-local normal binding: repeat, UV change, disable/re-enable, external state reset");
}
glDeleteProgram(p);glDeleteBuffers(1,&vbo);glDeleteVertexArrays(1,&vao);glDeleteTextures(1,&tx);}
puts("PASS full/bypass shader: cutout discard + opaque alpha, reflection, original opaque/soft alpha retained");}
