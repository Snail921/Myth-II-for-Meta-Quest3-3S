#include <string>
#include <utility>
#include <cstring>
#pragma once
// Original palette/filter/tint arithmetic is in display-encoded RGB. Decode
// only at the output boundary. The sRGB swapchain encodes on write and OpenXR
// decodes it for composition. Alpha is always linear.
static constexpr const char* mythVertexShader = R"GLSL(#version 300 es
precision highp float;
precision highp int;
layout(location=0) in vec3 pos;layout(location=1) in vec2 uv;
layout(location=2) in vec3 detailIds;layout(location=3) in vec3 detailWeights;
layout(location=4) in float shadowOwner;layout(location=5) in vec4 lightBounds;
flat out float spriteOwner;flat out vec3 materialIds;out vec3 materialWeights;
flat out highp uvec4 spriteLightMask;
uniform mat4 mvp;out vec2 tc;out float worldHeight;out vec3 fogPosition;
uniform bool spriteLightCull;uniform int spriteLightCount;
uniform int spriteObjectLightLimit;uniform float spriteLightingLodDistance;uniform vec3 spriteLightingEye;
uniform int spriteShadowSamples;uniform float spriteShadowLodDistance;uniform vec3 spriteShadowLodEye;
flat out float shadowSampleBudget;
uniform vec4 spriteLights[128];uniform vec4 spriteLightAxes[128];
void main(){
 spriteOwner=shadowOwner;gl_Position=mvp*vec4(pos,1);tc=uv;materialIds=detailIds;materialWeights=detailWeights;worldHeight=pos.y;fogPosition=pos;
 shadowSampleBudget=float(spriteShadowSamples>0?spriteShadowSamples:9);
 if(spriteLightCull&&spriteShadowLodDistance>0.0){
  float d=distance(lightBounds.xyz,spriteShadowLodEye),start=spriteShadowLodDistance;
  float reduction=smoothstep(start,start*2.5,d);
  shadowSampleBudget=mix(shadowSampleBudget,1.0,reduction);
  float small=1.0-smoothstep(.003,.015,lightBounds.w/max(d,.001));
  shadowSampleBudget=mix(shadowSampleBudget,1.0,small*smoothstep(start,start*1.5,d));
 }
 spriteLightMask=uvec4(0);
 int budget=spriteObjectLightLimit>0?spriteObjectLightLimit:128,accepted=0;
 if(spriteLightCull&&spriteLightingLodDistance>0.0){float d=max(0.0,distance(lightBounds.xyz,spriteLightingEye)-lightBounds.w);float factor=exp2(floor(log2(max(1.0,2.0*d/spriteLightingLodDistance))));budget=max(1,int(float(budget)/factor));}

 if(shadowOwner>0.0)for(int i=0;i<128;i++){if(i>=spriteLightCount)break;
  bool keep=true;
  if(spriteLightCull){
   vec3 d=lightBounds.xyz-spriteLights[i].xyz;vec4 axis=spriteLightAxes[i];d-=axis.xyz*clamp(dot(d,axis.xyz)*axis.w,0.0,1.0);
   float radius=lightBounds.w+inversesqrt(max(spriteLights[i].w,1e-20));
   keep=dot(d,d)<=radius*radius; // Conservative sphere vs sphere/capsule.
  }
  if(keep){spriteLightMask[i/32]|=1u<<uint(i%32);if(++accepted>=budget)break;}
 }
}
)GLSL";
static constexpr const char* mythFragmentShader = R"GLSL(#version 300 es
precision highp float;
precision highp int;
flat in highp uvec4 spriteLightMask;
flat in float shadowSampleBudget;
in vec2 tc;
in float worldHeight;
in vec3 fogPosition;
uniform vec4 fogColorDensity;
uniform vec3 fogEye;
uniform float reflectionClip;
uniform float reflectionHeight;
uniform sampler2D tex;
uniform highp sampler2DArray detailTex;
uniform bool detailEnabled;
uniform vec2 detailDimensions;
uniform vec2 detailScales[128];
flat in vec3 materialIds;
in vec3 materialWeights;
uniform vec4 tint;
uniform float alphaCutoff;
uniform bool softOnly;
uniform bool spriteCutout;
flat in float spriteOwner;
uniform bool spriteShadows;
uniform bool spriteSkipShadowLookup;
uniform bool spriteNormalEnabled;
uniform sampler2D spriteNormalMap;
uniform vec4 spriteNormalUV;
uniform mat4 spriteSunMatrix;
uniform sampler2D spriteStaticDepth, spriteLiveDepth, spriteLiveOwner;
uniform float spriteShadowBias;
uniform float spriteShadowAmount;uniform vec3 spriteSunDirection;
uniform int spriteLightCount;
uniform vec4 spriteLights[128];
uniform vec4 spriteLightColors[128];
uniform vec4 spriteLightAxes[128];
out vec4 color;
vec4 spriteLocalLight(vec3 normal){
 vec3 colorLight=vec3(0);float light=0.0;
 for(int block=0;block<4;block++){
 uint mask=spriteLightMask[block];
 for(int bit=0;bit<32;bit++,mask>>=1u){if(mask==0u)break;if((mask&1u)==0u)continue;int i=block*32+bit;
  vec3 d=fogPosition-spriteLights[i].xyz;vec4 axis=spriteLightAxes[i];d-=axis.xyz*clamp(dot(d,axis.xyz)*axis.w,0.0,1.0);float distance2=dot(d,d);if(distance2*spriteLights[i].w>=1.0)continue;float reach=max(0.0,1.0-distance2*spriteLights[i].w);
  float facing=max(0.0,dot(normal,-d)*inversesqrt(max(dot(d,d),1e-8)));
  float power=reach*reach*spriteLightColors[i].a*facing;light+=power;colorLight+=spriteLightColors[i].rgb*power;
 }
 }
 return vec4(colorLight/max(1.0,light),clamp(light,0.0,1.0));
}
float spriteVisibility(){
    if(spriteSkipShadowLookup)return 1.0;
    vec3 p=(spriteSunMatrix*vec4(fogPosition,1.0)).xyz*.5+.5;
    if(any(lessThanEqual(p,vec3(0)))||any(greaterThanEqual(p,vec3(1))))return 1.0;
    vec2 texel=1.0/vec2(textureSize(spriteStaticDepth,0));float blocked=0.0;
    // Prefix-stable pattern: all nine reproduce the original 3x3 footprint.
    const vec2 offsets[9]=vec2[9](vec2(0),vec2(1,1),vec2(-1,-1),vec2(1,-1),vec2(-1,1),vec2(1,0),vec2(-1,0),vec2(0,1),vec2(0,-1));
    float budget=clamp(shadowSampleBudget,1.0,9.0);
    for(int i=0;i<9;i++){
        float weight=clamp(budget-float(i),0.0,1.0);if(weight<=0.0)break;
        vec2 uv=p.xy+offsets[i]*texel;
        if(any(lessThan(uv,vec2(0)))||any(greaterThan(uv,vec2(1))))continue;
        if(p.z-spriteShadowBias>texture(spriteStaticDepth,uv).r){blocked+=weight;continue;}
        vec3 bytes=floor(texture(spriteLiveOwner,uv).rgb*255.0+.5);
        float owner=bytes.r+bytes.g*256.0+bytes.b*65536.0;
        if(owner>0.0&&abs(owner-spriteOwner)>.5&&p.z-spriteShadowBias>texture(spriteLiveDepth,uv).r)blocked+=weight;
    }
    return 1.0-spriteShadowAmount*blocked/budget;
}
// Treat the sprite as a rounded upright body. Derive the increasing-U axis
// from geometry so mirrored artwork does not mirror the world light direction.
vec3 spriteBodyNormal(){
    vec3 px=dFdx(fogPosition),py=dFdy(fogPosition);vec2 ux=dFdx(tc),uy=dFdy(tc);
    float det=ux.x*uy.y-uy.x*ux.y;vec3 face=cross(px,py);
    if(abs(det)<1e-10||dot(face,face)<1e-12)return vec3(0);
    vec3 tangent=(px*uy.y-py*ux.y)/det;tangent.y=0.0;face.y=0.0;
    if(dot(tangent,tangent)<1e-12||dot(face,face)<1e-12)return vec3(0);
    tangent=normalize(tangent);face=normalize(face);
    if(dot(face,fogEye-fogPosition)<0.0)face=-face;
    float side=clamp(tc.x*2.0-1.0,-1.0,1.0);
    return normalize(face*sqrt(max(.04,1.0-side*side))+tangent*side+vec3(0,.25,0));
}
// Authoring Y is image-up; UV V grows down. Derivative basis follows all
// artwork flips, including HD crops, without reversing world-space lighting.
vec3 spriteMappedNormal(vec3 fallback){
 vec2 uv=tc*spriteNormalUV.xy+spriteNormalUV.zw;
 vec3 px=dFdx(fogPosition),py=dFdy(fogPosition);vec2 ux=dFdx(uv),uy=dFdy(uv);
 float det=ux.x*uy.y-uy.x*ux.y;
 if(abs(det)<1e-10)return fallback;
 vec3 t=(px*uy.y-py*ux.y)/det,b=(py*ux.x-px*uy.x)/det;
 if(dot(t,t)<1e-12||dot(b,b)<1e-12)return fallback;
 t=normalize(t);b=normalize(b-t*dot(t,b));vec3 face=normalize(cross(t,b));
 if(dot(face,fogEye-fogPosition)<0.0)face=-face;
 vec4 packed=texture(spriteNormalMap,uv);
 if(packed.a<.01||any(lessThan(uv,vec2(0)))||any(greaterThan(uv,vec2(1))))return fallback;
 vec3 n=packed.rgb*2.0-1.0;n.xy=(packed.rg*255.0-128.0)/127.0;
 return normalize(t*n.x-b*n.y+face*max(.001,n.z));
}
float spriteSelfVisibility(vec3 n){
    if(dot(n,n)<1e-8||dot(spriteSunDirection,spriteSunDirection)<1e-8)return 1.0;
    float shade=1.0-smoothstep(-.4,.25,dot(n,normalize(spriteSunDirection)));
    return 1.0-spriteShadowAmount*shade;
}
vec3 displayToLinear(vec3 c) {
    c = clamp(c, 0.0, 1.0);
    return mix(pow((c + 0.055) / 1.055, vec3(2.4)), c / 12.92,
               lessThanEqual(c, vec3(0.04045)));
}
void main() {
    // Derivatives must be evaluated before alpha discard, once for all lights.
    vec3 bodyNormal=vec3(0);
    if(spriteOwner>0.0&&(any(notEqual(spriteLightMask,uvec4(0)))||(spriteShadows&&spriteShadowAmount>0.0))){bodyNormal=spriteBodyNormal();if(spriteNormalEnabled)bodyNormal=spriteMappedNormal(bodyNormal);}
    float selfVisibility=1.0;
    if(spriteShadows && spriteOwner>0.0 && spriteShadowAmount>0.0)selfVisibility=spriteSelfVisibility(bodyNormal);
    if(reflectionClip>0.5 && worldHeight<reflectionHeight) discard;
    vec4 sampleColor = texture(tex, tc);
    if (sampleColor.a < alphaCutoff || (softOnly && sampleColor.a>=.995)) discard;
    vec4 encoded = sampleColor * tint;
    if(spriteCutout)encoded.a=1.0;
    if(detailEnabled){
        vec3 factor=vec3(1.0);
        for(int i=0;i<3;i++){
            int id=int(materialIds[i]+0.5);
            if(id>0)factor+=(texture(detailTex,vec3(tc*detailDimensions*detailScales[id],float(id))).rgb*2.0-1.0)*materialWeights[i];
        }
        encoded.rgb*=factor;
    }
    float visibility=1.0;
    if(spriteShadows && spriteOwner>0.0 && spriteShadowAmount>0.0)visibility=min(spriteVisibility(),selfVisibility);
    vec4 local=vec4(0);if(spriteOwner>0.0&&any(notEqual(spriteLightMask,uvec4(0))))local=spriteLocalLight(bodyNormal);
    visibility=mix(visibility,1.0,local.a);encoded.rgb*=visibility;
    if(fogColorDensity.a>0.0){
        float transmission=exp(-fogColorDensity.a*distance(fogPosition,fogEye));
        float fogAmount=1.0-transmission;
        encoded.rgb=mix(encoded.rgb,fogColorDensity.rgb,fogAmount*fogAmount);
    }
    // Offscreen reflection color uses premultiplied alpha, including FX.
    vec3 linearColor=displayToLinear(encoded.rgb);
    // Bounded diffuse addition, independently usable with enhanced shadows OFF.
    // Apply fog transmission to new light as well as the original artwork.
    float localFog=1.0;if(fogColorDensity.a>0.0){float f=1.0-exp(-fogColorDensity.a*distance(fogPosition,fogEye));localFog=1.0-f*f;}
    linearColor+= (vec3(1)-linearColor)*local.rgb*.35*localFog;
    color = vec4(linearColor * (reflectionClip>0.5 ? encoded.a : 1.0), encoded.a);
}
)GLSL";

// Share the exact shadow/light equations between vertex and fragment stages.
// A flat per-billboard weight keeps both triangles and both eyes on the same
// LOD. The conservative bounding sphere protects large objects at any distance.
static std::string spriteVertexLodSource(const char*source){
 std::string s=source;
 auto replace=[&](const char*a,const char*b){auto at=s.find(a);if(at!=std::string::npos)s.replace(at,strlen(a),b);};
 bool vertex=source==mythVertexShader;
 if(vertex){
  std::string fragment=mythFragmentShader;
  auto section=[&](const char*a,const char*b){auto start=fragment.find(a);return fragment.substr(start,fragment.find(b,start)-start);};
  std::string helpers=R"GLSL(
uniform float spriteVertexLodDistance;
uniform vec3 fogEye;
uniform bool spriteShadows;uniform bool spriteSkipShadowLookup;
uniform mat4 spriteSunMatrix;
uniform sampler2D spriteStaticDepth, spriteLiveDepth, spriteLiveOwner;
uniform float spriteShadowBias, spriteShadowAmount;
uniform vec3 spriteSunDirection;
uniform vec4 spriteLightColors[128];
flat out float vertexShadeWeight;
out float vertexVisibility;
out vec4 vertexLocalLight;
)GLSL";
  helpers+=section("vec4 spriteLocalLight(","// Treat the sprite");
  helpers+=section("float spriteSelfVisibility(","vec3 displayToLinear(");
  auto main=s.find("void main(){");s.insert(main,helpers);
  auto end=s.rfind("}");s.insert(end,R"GLSL(
 vertexShadeWeight=0.0;vertexVisibility=1.0;vertexLocalLight=vec4(0);
 if(spriteLightCull&&shadowOwner>0.0&&lightBounds.w>0.0&&spriteVertexLodDistance>0.0){
  float d=distance(lightBounds.xyz,spriteLightingEye);
  // Radius / nearest sphere distance is conservative for projected extent.
  float extent=lightBounds.w/max(d-lightBounds.w,.001);
  vertexShadeWeight=smoothstep(spriteVertexLodDistance,spriteVertexLodDistance*1.2,d-lightBounds.w)
                   *(1.0-smoothstep(.02,.03,extent));
 }
 if(vertexShadeWeight>0.0){
  // A rounded world-space billboard approximation, independent of artwork
  // UV mirroring and transparent corner normals. No normal-map access.
  vec3 face=fogEye-lightBounds.xyz;face.y=0.0;
  face=dot(face,face)>1e-8?normalize(face):vec3(0,0,1);
  vec3 tangent=cross(vec3(0,1,0),face);
  float side=clamp(dot(pos-lightBounds.xyz,tangent)/max(lightBounds.w,.001),-.8,.8);
  vec3 n=normalize(face*sqrt(1.0-side*side)+tangent*side+vec3(0,.25,0));
  if(spriteShadows&&spriteShadowAmount>0.0)vertexVisibility=min(spriteVisibility(),spriteSelfVisibility(n));
  if(any(notEqual(spriteLightMask,uvec4(0))))vertexLocalLight=spriteLocalLight(n);
 }
)GLSL");
 }else{
  replace("in vec2 tc;","in vec2 tc;\nflat in float vertexShadeWeight;\nin float vertexVisibility;\nin vec4 vertexLocalLight;");
  replace("if(spriteOwner>0.0&&(any", "if(vertexShadeWeight<1.0&&spriteOwner>0.0&&(any");
  replace("if(spriteShadows && spriteOwner>0.0 && spriteShadowAmount>0.0)selfVisibility", "if(vertexShadeWeight<1.0&&spriteShadows && spriteOwner>0.0 && spriteShadowAmount>0.0)selfVisibility");
  replace("if(spriteShadows && spriteOwner>0.0 && spriteShadowAmount>0.0)visibility", "if(vertexShadeWeight<1.0&&spriteShadows && spriteOwner>0.0 && spriteShadowAmount>0.0)visibility");
  replace("if(spriteOwner>0.0&&any(notEqual(spriteLightMask,uvec4(0))))local", "if(vertexShadeWeight<1.0&&spriteOwner>0.0&&any(notEqual(spriteLightMask,uvec4(0))))local");
  replace("visibility=mix(visibility,1.0,local.a);", "visibility=mix(visibility,vertexVisibility,vertexShadeWeight);local=mix(local,vertexLocalLight,vertexShadeWeight);\n    visibility=mix(visibility,1.0,local.a);");
 }
 return s;
}

// Compile the OFF path with literal constants so lighting/shadow branches and
// their attributes/samplers can be eliminated by the driver, not merely skipped.
static std::string spriteShaderSource(const char*source,bool shadows,bool lighting){
 std::string s=(shadows||lighting)?spriteVertexLodSource(source):std::string(source);
 auto replace=[&](const char*a,const char*b){auto at=s.find(a);if(at!=std::string::npos)s.replace(at,strlen(a),b);};
 if(!lighting){replace("uniform int spriteLightCount;","const int spriteLightCount=0;");replace("flat in highp uvec4 spriteLightMask;","const uvec4 spriteLightMask=uvec4(0);");}
 if(!shadows){replace("uniform bool spriteShadows;","const bool spriteShadows=false;");replace("uniform float spriteShadowLodDistance;","const float spriteShadowLodDistance=0.0;");}
 if(!shadows&&!lighting)replace("uniform bool spriteNormalEnabled;","const bool spriteNormalEnabled=false;");
 return s;
}
static std::string spriteShaderSource(const char*source,bool enhanced){return spriteShaderSource(source,enhanced,enhanced);}
