#pragma once
#include <fstream>
// Shared scene lighting, sampled on CPU only when the sky or sun direction changes.
namespace sprite_pbr {
static float reflection=.5f,specular=8.f,sss=2.f,minimumSss=.35f;
static GLuint sky=0;
static int localLights=0;
static bool debugSpecular=false,debugSss=false;
static bool meshes=false,units=false,sceneObjects=false;
static float lodStart=20,lodWidth=4;
static bool excludeMeshLod=false;
static float yaw=0,maxMip=0,sun[3]{},lastDirection[3]{};
static unsigned width=0,height=0;
static std::vector<unsigned char> pixels;
static bool sampled=false;
static bool enabled(){return debugSpecular||debugSss||reflection>0||specular>0||sss>0;}
static void load(const char*root){
 auto read=[&](const char*name,int fallback,int maximum){std::ifstream in(std::string(root)+"/"+name);int v=fallback;if(!(in>>v)||v<0||v>maximum)v=fallback;return v/100.f;};
 auto meters=[&](const char*key,int minimum,int maximum){std::ifstream in(std::string(root)+"/"+key);int value;return (in>>value)?std::clamp(value,minimum,maximum):minimum;};
 lodStart=meters("sprite-pbr.lod-start",20,1000);lodWidth=meters("sprite-pbr.lod-width",4,100);
 excludeMeshLod=read("sprite-pbr.exclude-mesh-lod",0,1)>0;
 units=read("sprite-pbr.units",0,1)>0;sceneObjects=read("sprite-pbr.scene-objects",0,1)>0;
 meshes=read("sprite-pbr.meshes",0,1)>0;
 debugSss=read("sprite-pbr.debug-sss",0,1)>0;
 debugSpecular=!debugSss&&read("sprite-pbr.debug-specular",0,1)>0;
 reflection=read("sprite-pbr.reflection",50,100);specular=read("sprite-pbr.specular",800,1600);sss=read("sprite-pbr.sss",200,400);minimumSss=read("sprite-pbr.sss-minimum",35,100);
}
static void clearSky(){sky=0;sampled=false;width=height=0;std::vector<unsigned char>().swap(pixels);sun[0]=sun[1]=sun[2]=0;}
static float linear(unsigned char c){float v=c/255.f;return v<=.04045f?v/12.92f:powf((v+.055f)/1.055f,2.4f);}
static void setSky(GLuint texture,unsigned w,unsigned h,const unsigned char*rgba,float angle){
 clearSky();if(!enabled())return;sky=texture;width=w;height=h;yaw=angle;maxMip=floorf(log2f(float(std::max(w,h))));
 pixels.resize(size_t(w)*h*3);for(size_t i=0;i<size_t(w)*h;i++)for(int c=0;c<3;c++)pixels[i*3+c]=rgba[i*4+c];
}
static void sampleSun(const float*direction){
 if(!sky||pixels.empty())return;
 if(sampled&&std::equal(direction,direction+3,lastDirection))return;
 std::copy(direction,direction+3,lastDirection);sampled=true;
 float length=sqrtf(direction[0]*direction[0]+direction[1]*direction[1]+direction[2]*direction[2]);
 sun[0]=sun[1]=sun[2]=0;if(length<1e-6f)return;
 // waterSun points FROM the surface TOWARD the light, opposite shadow travel.
 float u=.5f+(atan2f(direction[0],-direction[2])-yaw)/6.28318530718f;u-=floorf(u);
 float v=.5f-asinf(std::clamp(direction[1]/length,-1.f,1.f))/3.14159265359f;
 int x=int(floorf(u*width)),y=std::clamp(int(floorf(v*height)),0,int(height)-1);
 for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++){
  size_t at=(size_t(std::clamp(y+dy,0,int(height)-1))*width+(x+dx+int(width))%width)*3;
  for(int c=0;c<3;c++)sun[c]+=linear(pixels[at+c])/9.f;
 }
 LOG("Sprite PBR sun pixel=%d,%d linear=%g,%g,%g intensity=%g",x,y,sun[0],sun[1],sun[2],.2126f*sun[0]+.7152f*sun[1]+.0722f*sun[2]);
}
static void bind(GLuint program,const float*direction,bool mesh=false){
 sampleSun(direction);
 glUniform2f(glGetUniformLocation(program,"pbrLodRange"),lodStart,(mesh&&excludeMeshLod)?0.f:lodWidth);
 glUniform1i(glGetUniformLocation(program,"spritePbrEnabled"),enabled());
 glUniform1i(glGetUniformLocation(program,"spriteSpecularDebug"),debugSpecular);
 glUniform1i(glGetUniformLocation(program,"spriteSssDebug"),debugSss);
 glUniform1f(glGetUniformLocation(program,"spriteSssMinimum"),minimumSss);
 glUniform3f(glGetUniformLocation(program,"spritePbrStrength"),reflection,specular,sss);
 glUniform3fv(glGetUniformLocation(program,"spriteSunRadiance"),1,sun);
 glUniform1i(glGetUniformLocation(program,"spriteSkyAvailable"),sky!=0);
 glUniform2f(glGetUniformLocation(program,"spriteSkyInfo"),yaw,maxMip);
 glUniform1i(glGetUniformLocation(program,"spriteSkyMap"),12);
 glActiveTexture(GL_TEXTURE12);glBindTexture(GL_TEXTURE_2D,sky);glActiveTexture(GL_TEXTURE0);
}
}
