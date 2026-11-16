#pragma once
#include <queue>
// Parameters are set by the scene renderer; tests can exercise the same shader.
static bool waterWavesEnabled=true;
static GLuint waterNormalTexture=0,waterSkyTexture=0;
static float waterEye[3]{},waterSun[3]{.95f,.6f,-1.f},waterSkyYaw=0,waterFogDensity=0;
static double waterFrameTime=0;
static char waterSettingPath[1024]{};
static void pollWaterWaves(){FILE*f=fopen(waterSettingPath,"rb");if(!f)return;int c=fgetc(f);fclose(f);if(c=='0'||c=='1')waterWavesEnabled=c=='1';}
static void initWaterNormal(){
 // Periodic mathematical normal texture: no seam, mipmapped to suppress distant shimmer.
 const int size=128;std::vector<unsigned char> pixels(size*size*2);
 const float waves[][3]={{1,2,.24f},{3,-1,.17f},{-2,4,.11f},{5,3,.07f},{-7,2,.04f}};
 for(int y=0;y<size;y++)for(int x=0;x<size;x++){float dx=0,dz=0;for(auto&w:waves){float phase=6.283185307f*(w[0]*x+w[1]*y)/size;float len=sqrtf(w[0]*w[0]+w[1]*w[1]);dx+=w[2]*w[0]/len*cosf(phase);dz+=w[2]*w[1]/len*cosf(phase);}pixels[(y*size+x)*2]=(unsigned char)std::clamp(128+int(dx*127),0,255);pixels[(y*size+x)*2+1]=(unsigned char)std::clamp(128+int(dz*127),0,255);}
 glGenTextures(1,&waterNormalTexture);glBindTexture(GL_TEXTURE_2D,waterNormalTexture);glTexImage2D(GL_TEXTURE_2D,0,GL_RG8,size,size,0,GL_RG,GL_UNSIGNED_BYTE,pixels.data());glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glGenerateMipmap(GL_TEXTURE_2D);
}
// Artist texture replaces the procedural fallback. R/G encode tangent-plane slopes.
static bool uploadWaterNormal(const unsigned char*data,size_t length){
 if(!data||length<16||memcmp(data,"MYTHWN01",8))return false;unsigned w,h;memcpy(&w,data+8,4);memcpy(&h,data+12,4);if(!w||!h||w>2048||h>2048||length!=16+size_t(w)*h*2)return false;
 if(!waterNormalTexture)glGenTextures(1,&waterNormalTexture);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,waterNormalTexture);GLint alignment;glGetIntegerv(GL_UNPACK_ALIGNMENT,&alignment);glPixelStorei(GL_UNPACK_ALIGNMENT,1);glTexImage2D(GL_TEXTURE_2D,0,GL_RG8,w,h,0,GL_RG,GL_UNSIGNED_BYTE,data+16);glPixelStorei(GL_UNPACK_ALIGNMENT,alignment);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glGenerateMipmap(GL_TEXTURE_2D);LOG("Water normal artist texture=%ux%u linear RG8",w,h);return true;
}
// UV.x stores distance to the boundary of this plane's actual water triangles.
// Build once, never per frame. Sloped vertices retain their original heights.
static void waterShoreDistances(std::vector<Vertex>&vertices){
 std::map<std::array<float,3>,unsigned> ids;std::vector<unsigned> indices;std::vector<std::array<float,3>> points;
 for(auto&v:vertices){std::array<float,3> p{v.x,v.y,v.z};auto found=ids.emplace(p,(unsigned)ids.size());if(found.second)points.push_back(p);indices.push_back(found.first->second);}
 std::map<std::pair<unsigned,unsigned>,unsigned> edges;
 for(size_t i=0;i+2<indices.size();i+=3)for(int j=0;j<3;j++){unsigned a=indices[i+j],b=indices[i+(j+1)%3];if(a>b)std::swap(a,b);edges[{a,b}]++;}
 std::vector<std::vector<std::pair<unsigned,float>>> adjacency(points.size());std::vector<float> distance(points.size(),2*scale);
 using Entry=std::pair<float,unsigned>;std::priority_queue<Entry,std::vector<Entry>,std::greater<Entry>> queue;
 for(auto&e:edges){auto a=e.first.first,b=e.first.second;float dx=points[a][0]-points[b][0],dz=points[a][2]-points[b][2],length=sqrtf(dx*dx+dz*dz);adjacency[a].push_back({b,length});adjacency[b].push_back({a,length});if(e.second==1){distance[a]=distance[b]=0;queue.push({0,a});queue.push({0,b});}}
 while(!queue.empty()){auto [d,a]=queue.top();queue.pop();if(d!=distance[a])continue;for(auto [b,length]:adjacency[a])if(d+length<distance[b]){distance[b]=d+length;queue.push({distance[b],b});}}
 for(size_t i=0;i<vertices.size();i++){vertices[i].u=distance[indices[i]];vertices[i].v=0;}
}
static const char* waterVertexShader=R"(#version 300 es
layout(location=0) in vec3 pos;layout(location=1) in vec2 texcoord;
layout(location=2) in vec2 current;
uniform mat4 mvp;out vec3 world;out float shore;out vec2 flow;
void main(){world=pos;shore=texcoord.x;flow=current;gl_Position=mvp*vec4(pos,1);})";
static const char* waterFragmentShader=R"(#version 300 es
precision highp float;
uniform sampler2D tex,normalMap,sky;
uniform mat4 mvp;uniform vec4 viewport;uniform vec3 eye,sun;
uniform float waveTime,worldScale,skyYaw,fogDensity;uniform bool waves,hasSky;
in vec3 world;in float shore;in vec2 flow;out vec4 color;
void main(){
 vec2 uv=(gl_FragCoord.xy-viewport.xy)/viewport.zw;
 vec4 original=texture(tex,uv);if(!waves){color=original*.38;return;}
 vec2 p=world.xz/(worldScale*2.8);
 float animationTime=waveTime*3.5;
 // Two alternating advected phases prevent stretching at bends and time-wrap jumps.
 float phase=fract(animationTime/8.),other=fract(phase+.5);
 float blend=abs(phase*2.-1.);
 vec2 drift=flow*.055;
 vec2 idle=vec2(sin(animationTime*.25),cos(animationTime*.25))*.035;
 vec2 a0=texture(normalMap,p-drift*(phase*8.)+idle).rg;
 vec2 a1=texture(normalMap,p-drift*(other*8.)+idle).rg;
 vec2 b0=texture(normalMap,p*1.73-drift*(phase*8.)*1.73-idle).rg;
 vec2 b1=texture(normalMap,p*1.73-drift*(other*8.)*1.73-idle).rg;
 vec2 a=mix(a0,a1,blend)*2.-1.;
 vec2 b=mix(b0,b1,blend)*2.-1.;
 vec2 slope=(a+b)*.14;
 vec3 base=normalize(cross(dFdx(world),dFdy(world)));if(base.y<0.)base=-base;
 vec3 n=normalize(base+vec3(slope.x,0,slope.y));
 vec4 moved=mvp*vec4(world+vec3(slope.x,0,slope.y)*worldScale*.28,1);
 vec2 offset=clamp(moved.xy/max(moved.w,.001)*.5+.5-uv,vec2(-.004),vec2(.004));
 float edgePixels=shore/max(length(vec2(dFdx(shore),dFdy(shore))),.00001);
 float margin=length(offset*viewport.zw)+2.*max(viewport.z/float(textureSize(tex,0).x),viewport.w/float(textureSize(tex,0).y));
 float edge=smoothstep(0.,worldScale*1.5,shore)*smoothstep(margin,margin*2.+1.,edgePixels);
 offset*=edge;
 vec2 inset=.5/vec2(textureSize(tex,0));vec4 reflected=texture(tex,clamp(uv+offset,inset,1.-inset));
 // Empty masked texels must never introduce a black fringe.
 if(reflected.a<.00001&&original.a>.00001)reflected=original;
 vec3 light=normalize(sun),v=normalize(eye-world),halfway=normalize(light+v);
 float shine=pow(max(dot(n,halfway),0.),96.)*max(dot(n,light),0.);
 vec3 sunColor=vec3(0);float strength=0.;
 if(hasSky){vec2 su=vec2(fract(.5+(atan(light.x,-light.z)-skyYaw)/6.283185307),.5-asin(clamp(light.y,-1.,1.))/3.141592654);sunColor=textureLod(sky,su,4.).rgb;vec3 ambient=textureLod(sky,su,6.).rgb;float lum=dot(ambient,vec3(.2126,.7152,.0722));strength=clamp(lum*1.5,0.,1.);strength*=strength;}
 float haze=1.-exp(-fogDensity*distance(eye,world));
 vec3 specular=sunColor*shine*strength*.45*(1.-haze*haze)*smoothstep(0.,worldScale*.2,shore);
 color=vec4(reflected.rgb*.38+specular,reflected.a*.38);
})";
