#pragma once
// Shared physical weather geometry for both eyes and all reflected water planes.
static GLuint weatherProgram=0;static SpriteStream weatherStream;static bool weatherReady=false;
static XrVector3f weatherCenter{};static float weatherNear=28,weatherFar=36;
static unsigned weatherDrops=0;static float weatherOpacity=.3f;
static bool weatherVR=true,snowVR=true;static char weatherSettingPath[1024]{},snowSettingPath[1024]{};static double weatherPoll=-1;
static void pollWeatherSetting(){
 double now=seconds();if(weatherPoll>=0&&now-weatherPoll<1)return;weatherPoll=now;
 auto read=[&](const char*path,bool&value){FILE*f=fopen(path,"rb");if(!f)return;int c=fgetc(f),tail=fgetc(f);fclose(f);if((c=='0'||c=='1')&&tail==EOF){bool next=c=='1';if(next!=value)weatherReady=false;value=next;}};read(weatherSettingPath,weatherVR);read(snowSettingPath,snowVR);
}
static void initWeatherSetting(const char*root){weatherVR=snowVR=true;weatherPoll=-1;snprintf(weatherSettingPath,sizeof(weatherSettingPath),"%s/weather.mode",root);snprintf(snowSettingPath,sizeof(snowSettingPath),"%s/snow.mode",root);pollWeatherSetting();}

static void initWeather(){
 if(weatherProgram)return;
 std::string vs=mythVertexShader,fs=mythFragmentShader;
 auto replace=[](std::string&s,const char*a,const char*b){auto p=s.find(a);if(p==std::string::npos)throw std::runtime_error("weather shader template");s.replace(p,strlen(a),b);};
 replace(vs,"out float worldHeight;","out float worldHeight;out vec3 worldPosition;");
 replace(vs,"worldHeight=pos.y;","worldHeight=pos.y;worldPosition=pos;");
 replace(fs,"in float worldHeight;","in float worldHeight;in vec3 worldPosition;uniform vec3 weatherEye;uniform vec2 weatherFade;");
 replace(fs,"vec4 encoded = sampleColor * tint;","vec4 encoded = sampleColor * tint;encoded.a *= clamp((weatherFade.y-distance(worldPosition,weatherEye))/(weatherFade.y-weatherFade.x),0.0,1.0);");
 weatherProgram=glCreateProgram();GLuint v=shader(GL_VERTEX_SHADER,vs.c_str()),f=shader(GL_FRAGMENT_SHADER,fs.c_str());glAttachShader(weatherProgram,v);glAttachShader(weatherProgram,f);glLinkProgram(weatherProgram);glDeleteShader(v);glDeleteShader(f);GLint ok;glGetProgramiv(weatherProgram,GL_LINK_STATUS,&ok);if(!ok)throw std::runtime_error("weather shader");
}
static void drawWeather(const Mat&mvp,bool reflected){
 pollWeatherSetting();
 if(!weatherReady){
  weatherReady=true;weatherDrops=0;std::map<GLuint,std::vector<Vertex>> batches;
  float cx=originX-weatherCenter.x/scale,cy=originY+weatherCenter.z/scale;
  for(const auto&p:liveParticles){if(p[15]!=2)continue;auto it=particleQuads.find({particleCollectionKey(p),p[2],p[3],(p[11]>>8)&63});if(it==particleQuads.end())continue;auto q=it->second;
   bool streak=(q.flags&16)!=0,vr=streak?weatherVR:snowVR;weatherOpacity=streak?.30f:1.f;
   float period=p[9],radius=vr?(streak?18.f:8.f):(p[7]>>16)/512.f;weatherNear=(vr?(streak?6.f:5.f):(p[7]&65535)/512.f)*scale;weatherFar=radius*scale;
   // Three translated copies preserve the original velocity, size and weather transitions.
   // Fixed sub-tile offsets avoid overlapping flakes and frame-to-frame random flicker.
   for(unsigned copy=0;copy<(streak?1u:3u);copy++){
   float bx=int32_t(p[4])/512.f+copy*period*.381966011f,by=int32_t(p[5])/512.f+copy*period*.618033989f,bz=int32_t(p[6])/512.f;
   float size=q.scale*(10.f/512.f)*scale*p[8]/256.f,l=-q.rx*size,r=(q.w-q.rx)*size,b=(q.ry-q.h)*size,t=q.ry*size;
   float u0=(q.flags&1)?1:0,u1=1-u0,v0=(q.flags&2)?1:0,v1=1-v0;
   float zPeriod=(p[11]>>16)/512.f;if(zPeriod<.1f)zPeriod=5.f;
   float cz=originZ+weatherCenter.y/scale;
   int bottomTile=vr?(int)floorf((cz-radius-bz)/zPeriod):0,topTile=vr?(int)ceilf((cz+radius-bz)/zPeriod):0;
   GLuint texture=particleTexture(q.texture,255); // weather uses its own original palette, not a team variant
   for(int ty=(int)floorf((cy-radius-by)/period);ty<=(int)ceilf((cy+radius-by)/period);ty++)for(int tx=(int)floorf((cx-radius-bx)/period);tx<=(int)ceilf((cx+radius-bx)/period);tx++)for(int tz=bottomTile;tz<=topTile;tz++){
    float x=bx+tx*period,y=by+ty*period;if(!vr&&(x<0||y<0||x>=levelWidth-1||y>=levelHeight-1))continue;
    Vec3 pos{mythWorldX(x,originX,scale),(bz+tz*zPeriod-originZ)*scale,(y-originY)*scale},toward{weatherCenter.x-pos.x,weatherCenter.y-pos.y,weatherCenter.z-pos.z};if(dot(toward,toward)>weatherFar*weatherFar||weatherDrops>=20000)continue;
    Vec3 right,up;if(q.flags&16){right=arrowRight({-float(int32_t(p[10])),float(int32_t(p[14])),float(int32_t(p[12]))});up=arrowUp(right,toward);}else{float n=sqrtf(toward.x*toward.x+toward.z*toward.z);if(n<.001f)continue;right={toward.z/n,0,-toward.x/n};float length=sqrtf(dot(toward,toward));up={-toward.y*right.z/length,n/length,toward.y*right.x/length};}
    auto vertex=[&](float a,float b,float u,float v){auto w=add(pos,add(times(right,a),times(up,b)));return Vertex{w.x,w.y,w.z,u,v};};
    Vertex a=vertex(l,t,u0,v0),bb=vertex(r,t,u1,v0),c=vertex(l,b,u0,v1),d=vertex(r,b,u1,v1);auto&out=batches[texture];out.insert(out.end(),{a,bb,c,bb,d,c});weatherDrops++;
   }
  }
  } // density copies
  weatherStream.upload(batches);
 }
 if(!weatherDrops)return;initWeather();glUseProgram(weatherProgram);applyFog(weatherProgram);
 glUniformMatrix4fv(glGetUniformLocation(weatherProgram,"mvp"),1,GL_FALSE,mvp.data());glUniform4f(glGetUniformLocation(weatherProgram,"tint"),1,1,1,weatherOpacity);glUniform1f(glGetUniformLocation(weatherProgram,"alphaCutoff"),.003f);glUniform1i(glGetUniformLocation(weatherProgram,"tex"),0);
 // GLES forbids different sampler types sharing a unit, even with detailEnabled=false.
 glUniform1i(glGetUniformLocation(weatherProgram,"detailTex"),1);glUniform1i(glGetUniformLocation(weatherProgram,"detailEnabled"),0);glUniform1i(glGetUniformLocation(weatherProgram,"softOnly"),0);glActiveTexture(GL_TEXTURE0);
 glUniform1f(glGetUniformLocation(weatherProgram,"reflectionClip"),reflected?1:0);glUniform1f(glGetUniformLocation(weatherProgram,"reflectionHeight"),reflectionLevel);glUniform3f(glGetUniformLocation(weatherProgram,"weatherEye"),weatherCenter.x,weatherCenter.y,weatherCenter.z);glUniform2f(glGetUniformLocation(weatherProgram,"weatherFade"),weatherNear,weatherFar);
 glEnable(GL_BLEND);if(reflected)glBlendFuncSeparate(GL_ONE,GL_ONE_MINUS_SRC_ALPHA,GL_ONE,GL_ONE_MINUS_SRC_ALPHA);else glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glDepthMask(GL_FALSE);// SpriteStream::draw is a cutout/HD pass tied to program, not weatherProgram.
 attributes(weatherStream.buffers[weatherStream.cursor]);for(auto&r:weatherStream.ranges){glBindTexture(GL_TEXTURE_2D,r.texture);glDrawArrays(GL_TRIANGLES,r.first,r.count);}glDepthMask(GL_TRUE);glDisable(GL_BLEND);glUseProgram(program);
}
