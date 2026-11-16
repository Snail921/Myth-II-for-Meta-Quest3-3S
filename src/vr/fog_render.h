// Map density uses original fixed-point distance; scene positions use VR metres.
static float fogRGB[3]{},fogDensity=0;static bool fogEnabled=true;
static XrVector3f fogCenter{};static std::string fogPath;static double fogPoll=-1;
static void pollFog(){double now=seconds();if(fogPoll>=0&&now-fogPoll<1)return;fogPoll=now;FILE*f=fopen(fogPath.c_str(),"rb");if(f){int c=fgetc(f);fclose(f);if(c=='0'||c=='1')fogEnabled=c=='1';}}
static void loadFog(android_app*a){
 fogDensity=0;fogRGB[0]=fogRGB[1]=fogRGB[2]=0;fogEnabled=true;fogPoll=-1;fogPath=std::string(a->activity->internalDataPath)+"/fog.enabled";pollFog();
 auto data=asset(a,"fog.pack");const auto*b=data.data();size_t n=data.size();
 if(b&&n>=4&&u32(b)<=1024&&n==4+size_t(u32(b))*20)for(unsigned i=0;i<u32(b);i++){
  const auto*r=b+4+i*20;char hex[9];snprintf(hex,sizeof(hex),"%02x%02x%02x%02x",r[0],r[1],r[2],r[3]);
  if(levelAssetPrefix!="levels/"+std::string(hex)+"/")continue;
  float v[4];memcpy(v,r+4,16);bool valid=true;for(float x:v)valid&=std::isfinite(x)&&x>=0;
  if(valid&&v[0]<=1&&v[1]<=1&&v[2]<=1){std::copy(v,v+3,fogRGB);fogDensity=v[3]*512.f/scale;}
 }
 LOG("Map fog density_per_m=%g color=%g,%g,%g enabled=%d",fogDensity,fogRGB[0],fogRGB[1],fogRGB[2],fogEnabled);
}
static void applyFog(GLuint p,bool enabled=true){glUniform4f(glGetUniformLocation(p,"fogColorDensity"),fogRGB[0],fogRGB[1],fogRGB[2],enabled&&fogEnabled?fogDensity:0);glUniform3f(glGetUniformLocation(p,"fogEye"),fogCenter.x,fogCenter.y,fogCenter.z);}
