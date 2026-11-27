#pragma once
#include <string>
static bool enhancedShadowsEnabled=true;
static bool unifiedShadowsEnabled=false;
static bool unifiedBaseReady=false;
static int dynamicShadowResolution=2048;
static unsigned spriteLiveDepth=0;
static bool unifiedGroundShadows(){return enhancedShadowsEnabled&&unifiedShadowsEnabled;}
// Inject only into the terrain program: other materials retain their existing path.
static std::string unifiedTerrainShader(std::string source,bool mesh=false){
 const char* helper=R"(
uniform sampler2D unifiedDepth,unifiedStaticDepth;
uniform mat4 unifiedSunMatrix;
uniform float unifiedBias,unifiedAmount;
uniform int unifiedSamples;
uniform bool unifiedBase;
uniform vec3 unifiedSun;
uniform sampler2D unifiedTerrainNormals;
uniform bool unifiedSmoothTerrain;
uniform vec4 unifiedNormalUV;
float unifiedGroundVisibility(){
 vec3 n=cross(dFdx(fogPosition),dFdy(fogPosition));
 if(dot(n,n)<1e-12)n=vec3(0,1,0);else n=normalize(n);
 UNIFIED_NORMAL_ORIENTATION
 UNIFIED_SMOOTH_NORMAL
 vec3 p=(unifiedSunMatrix*vec4(fogPosition,1.)).xyz*.5+.5;
 float diffuse=max(0.,dot(n,normalize(unifiedSun)));
 float visibility=1.;
 if(all(greaterThan(p,vec3(0)))&&all(lessThan(p,vec3(1)))){
 const vec2 offsets[9]=vec2[9](vec2(0),vec2(1,1),vec2(-1,-1),vec2(1,-1),vec2(-1,1),vec2(1,0),vec2(-1,0),vec2(0,1),vec2(0,-1));
 vec2 texel=1./vec2(textureSize(unifiedDepth,0));float blocked=0.;
 int count=clamp(unifiedSamples,1,9);
 float depthBias=unifiedBias*(1.+2.*(1.-diffuse));
 for(int i=0;i<9;i++){
  if(i>=count)break;vec2 uv=p.xy+offsets[i]*texel;bool shadow=false;
  if(all(greaterThanEqual(uv,vec2(0)))&&all(lessThanEqual(uv,vec2(1))))shadow=p.z-depthBias>texture(unifiedDepth,uv).r;
  if(unifiedBase){uv=p.xy+offsets[i]/vec2(textureSize(unifiedStaticDepth,0));
   if(all(greaterThanEqual(uv,vec2(0)))&&all(lessThanEqual(uv,vec2(1))))shadow=shadow||p.z-depthBias>texture(unifiedStaticDepth,uv).r;
  }
  if(shadow)blocked+=1.;
 }
 visibility=1.-blocked/float(count);
 }
 // Ambient remains in shadow. Remove original baked terrain shading only when
 // the exported base texture is present; old packs keep stage-one behavior.
 return 1.-unifiedAmount+unifiedAmount*visibility*(unifiedBase?diffuse:1.);
}
)";
 std::string function=helper;const std::string normal="UNIFIED_NORMAL_ORIENTATION";
 function.replace(function.find(normal),normal.size(),mesh?"if(dot(n,fogEye-fogPosition)<0.)n=-n;":"if(n.y<0.)n=-n;");
 const std::string smooth="UNIFIED_SMOOTH_NORMAL";
 function.replace(function.find(smooth),smooth.size(),mesh?"":"if(unifiedSmoothTerrain)n=normalize(texture(unifiedTerrainNormals,fogPosition.xz*unifiedNormalUV.xy+unifiedNormalUV.zw).xyz*2.-1.);");
 source.insert(source.find("void main()"),function);
 const std::string marker="    if(fogColorDensity.a>0.0){";
 source.insert(source.find(marker),"    encoded.rgb*=unifiedGroundVisibility();\n");
 return source;
}
