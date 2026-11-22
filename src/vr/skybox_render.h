#pragma once
// Fullscreen angular background: no camera translation, no depth writes.
static GLuint skyProgram=0,skyVAO=0,skyMap=0;
static float skyYaw=0;
// Relative to the existing water blend: 0=no sky reflection, 1=previous strength.
static float skyReflectionBlend=.25f;
static char skyBlendPath[1024]{};static double skyBlendPoll=-1;
static void pollSkyReflectionBlend(){
 double now=seconds();if(skyBlendPoll>=0&&now-skyBlendPoll<1)return;skyBlendPoll=now;
 FILE*f=fopen(skyBlendPath,"rb");if(!f)return;unsigned char value[4];size_t n=fread(value,1,4,f);fclose(f);
 if(n!=3||value[0]<'0'||value[0]>'1'||value[1]<'0'||value[1]>'9'||value[2]<'0'||value[2]>'9')return;
 int percent=(value[0]-'0')*100+(value[1]-'0')*10+value[2]-'0';if(percent>100)return;
 skyReflectionBlend=percent/100.f;
}
static void initSkyReflectionBlend(const char*root){skyReflectionBlend=.25f;skyBlendPoll=-1;snprintf(skyBlendPath,sizeof(skyBlendPath),"%s/sky-reflection.blend",root);pollSkyReflectionBlend();}
static void clearSkybox(){sprite_pbr::clearSky();if(skyMap)glDeleteTextures(1,&skyMap);skyMap=0;}
static void uploadSkybox(unsigned w,unsigned h,const unsigned char*rgba){
 clearSkybox();glGenTextures(1,&skyMap);glBindTexture(GL_TEXTURE_2D,skyMap);
 glTexImage2D(GL_TEXTURE_2D,0,GL_SRGB8_ALPHA8,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,rgba);
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glGenerateMipmap(GL_TEXTURE_2D);sprite_pbr::setSky(skyMap,w,h,rgba,skyYaw);sprite_pbr::sampleSun(waterSun);
}
static void initSkybox(){
 // Called once for each newly created EGL context; old GL names are not valid here.
 skyMap=0;
 const char*vs=R"(#version 300 es
out vec2 screen;
void main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);screen=p*2.0-1.0;gl_Position=vec4(screen,1,1);})";
 const char*fs=R"(#version 300 es
precision highp float;
in vec2 screen;out vec4 color;
uniform vec4 fogSky;uniform sampler2D sky;uniform vec4 tangents;uniform mat3 cameraToWorld;uniform float mirrored;uniform float yaw;uniform float blend;
void main(){
 vec2 t=mix(tangents.xz,tangents.yw,screen*.5+.5);
 vec3 d=normalize(cameraToWorld*vec3(t,-1));d.y*=mirrored;
 vec2 uv=vec2(fract(.5+(atan(d.x,-d.z)-yaw)/6.28318530718),.5-asin(clamp(d.y,-1.,1.))/3.14159265359);
 // textureGrad avoids selecting the lowest mip at the longitude seam.
 vec2 dx=dFdx(uv),dy=dFdy(uv);dx.x-=round(dx.x);dy.x-=round(dy.x);
 // Premultiplied RGBA: lowering sky opacity exposes original water, not black.
 vec3 rgb=textureGrad(sky,uv,dx,dy).rgb;
 float haze=fogSky.a*(1.0-smoothstep(0.0,.35,max(d.y,0.0)));
 color=vec4(mix(rgb,fogSky.rgb,haze*haze)*blend,blend);
})";
 skyProgram=glCreateProgram();auto v=shader(GL_VERTEX_SHADER,vs),f=shader(GL_FRAGMENT_SHADER,fs);glAttachShader(skyProgram,v);glAttachShader(skyProgram,f);glLinkProgram(skyProgram);glDeleteShader(v);glDeleteShader(f);
 GLint ok=0;glGetProgramiv(skyProgram,GL_LINK_STATUS,&ok);if(!ok)throw std::runtime_error("sky shader");glGenVertexArrays(1,&skyVAO);
}
static void drawSkybox(const XrView&eye,bool reflected){
 if(!skyMap)return;
 GLint oldProgram,oldVAO;glGetIntegerv(GL_CURRENT_PROGRAM,&oldProgram);glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&oldVAO);
 glUseProgram(skyProgram);
 float c[3];for(int i=0;i<3;i++)c[i]=fogRGB[i]<=.04045f?fogRGB[i]/12.92f:powf((fogRGB[i]+.055f)/1.055f,2.4f);
 glUniform4f(glGetUniformLocation(skyProgram,"fogSky"),c[0],c[1],c[2],fogEnabled?1-expf(-fogDensity*std::max(levelWidth,levelHeight)*scale):0);glBindVertexArray(skyVAO);glDisable(GL_DEPTH_TEST);glDepthMask(GL_FALSE);
 Mat r=view(eye.pose);float rotation[]={r[0],r[4],r[8],r[1],r[5],r[9],r[2],r[6],r[10]};
 glUniformMatrix3fv(glGetUniformLocation(skyProgram,"cameraToWorld"),1,GL_FALSE,rotation);
 glUniform4f(glGetUniformLocation(skyProgram,"tangents"),tanf(eye.fov.angleLeft),tanf(eye.fov.angleRight),tanf(eye.fov.angleDown),tanf(eye.fov.angleUp));
 glUniform1f(glGetUniformLocation(skyProgram,"blend"),reflected?skyReflectionBlend:1.f);
 glUniform1f(glGetUniformLocation(skyProgram,"mirrored"),reflected?-1.f:1.f);glUniform1f(glGetUniformLocation(skyProgram,"yaw"),skyYaw);
 glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,skyMap);glUniform1i(glGetUniformLocation(skyProgram,"sky"),0);glDrawArrays(GL_TRIANGLES,0,3);
 glDepthMask(GL_TRUE);glEnable(GL_DEPTH_TEST);glBindVertexArray(oldVAO);glUseProgram(oldProgram);
}
