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
layout(location=9) in vec4 particleAlpha;flat out float vertexAlpha;
layout(location=2) in vec3 detailIds;layout(location=3) in vec3 detailWeights;
layout(location=6) in vec4 atlasColorRect;layout(location=7) in vec4 atlasNormalRect;layout(location=8) in vec4 atlasNormalMapping;
flat out vec4 atlasColor;flat out vec4 atlasNormal;flat out vec4 atlasMapping;
layout(location=4) in float shadowOwner;layout(location=5) in vec4 lightBounds;
flat out float spriteOwner;flat out vec3 materialIds;out vec3 materialWeights;
flat out highp uvec4 spriteLightMask;
uniform mat4 mvp;out vec2 tc;out float worldHeight;out vec3 fogPosition;
uniform bool spriteLightCull;uniform int spriteLightCount;
uniform int spriteObjectLightLimit;uniform float spriteLightingLodDistance;uniform vec3 spriteLightingEye;
uniform int spriteShadowSamples;uniform float spriteShadowLodDistance;uniform vec3 spriteShadowLodEye;
flat out float shadowSampleBudget;
#ifdef PBR_DETAIL_LOD
uniform vec2 pbrLodRange;
flat out float pbrDetail;
#endif
uniform vec4 spriteLights[128];uniform vec4 spriteLightAxes[128];
void main(){
 vertexAlpha=particleAlpha.w;atlasColor=atlasColorRect;atlasNormal=atlasNormalRect;atlasMapping=atlasNormalMapping;spriteOwner=shadowOwner;gl_Position=mvp*vec4(pos,1);tc=uv;materialIds=detailIds;materialWeights=detailWeights;worldHeight=pos.y;fogPosition=pos;
#ifdef PBR_DETAIL_LOD
 pbrDetail=1.0;
 if(pbrLodRange.y>0.0 && lightBounds.w>0.0 && shadowOwner>0.0){
  float d=distance(lightBounds.xyz,spriteShadowLodEye);
  pbrDetail=1.0-smoothstep(pbrLodRange.x,pbrLodRange.x+pbrLodRange.y,d);
 }
#endif
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
#ifdef PBR_DETAIL_LOD
flat in float pbrDetail;
#else
const float pbrDetail=1.0;
#endif
in vec2 tc;flat in float vertexAlpha;
flat in vec4 atlasColor;flat in vec4 atlasNormal;flat in vec4 atlasMapping;uniform bool spriteAtlas;
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
uniform vec2 detailLodRange;
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
uniform bool spritePackedMaterial;uniform bool spriteSplitMaterial;
uniform bool spritePbrEnabled;
uniform bool spriteSpecularDebug;
uniform bool spriteSssDebug;
vec3 spriteSssOnly=vec3(0);
vec3 spriteSpecularOnly=vec3(0);
uniform bool spriteSkyAvailable;
uniform sampler2D spriteSkyMap;
uniform vec2 spriteSkyInfo;
uniform vec3 spritePbrStrength, spriteSunRadiance;
uniform float spriteSssMinimum;
bool surfaceMaterialValid=false;
vec2 surfaceMaterial=vec2(0,1); // M&S, thickness; legacy defaults disable PBR.
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
        uv=p.xy+offsets[i]/vec2(textureSize(spriteLiveDepth,0));
        if(any(lessThan(uv,vec2(0)))||any(greaterThan(uv,vec2(1))))continue;
        vec3 bytes=floor(texture(spriteLiveOwner,uv).rgb*255.0+.5);
        float owner=bytes.r+bytes.g*256.0+bytes.b*65536.0;
        if(owner>0.0&&abs(owner-spriteOwner)>.5&&p.z-spriteShadowBias>texture(spriteLiveDepth,uv).r)blocked+=weight;
    }
    // Physical sunlight visibility; artwork darkness is applied separately.
    return 1.0-blocked/budget;
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
 vec4 mapping=spriteAtlas?atlasMapping:spriteNormalUV;vec2 uv=tc*mapping.xy+mapping.zw;
 vec3 px=dFdx(fogPosition),py=dFdy(fogPosition);vec2 ux=dFdx(uv),uy=dFdy(uv);
 float det=ux.x*uy.y-uy.x*ux.y;
 if(abs(det)<1e-10)return fallback;
 vec3 t=(px*uy.y-py*ux.y)/det,b=(py*ux.x-px*uy.x)/det;
 if(dot(t,t)<1e-12||dot(b,b)<1e-12)return fallback;
 t=normalize(t);b=normalize(b-t*dot(t,b));vec3 face=normalize(cross(t,b));
 if(dot(face,fogEye-fogPosition)<0.0)face=-face;
 vec4 packed=texture(spriteNormalMap,spriteAtlas?clamp(uv,0.0,1.0)*atlasNormal.xy+atlasNormal.zw:uv);
 if((!spritePackedMaterial&&packed.a<.01)||any(lessThan(uv,vec2(0)))||any(greaterThan(uv,vec2(1))))return fallback;
 vec3 n=packed.rgb*2.0-1.0;n.xy=(packed.rg*255.0-128.0)/127.0;
 if(spritePackedMaterial){surfaceMaterialValid=true;surfaceMaterial=vec2(packed.a,packed.b);n.z=sqrt(max(0.0,1.0-dot(n.xy,n.xy)));}
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
vec3 spritePbr(vec3 base,vec3 albedoEncoded,vec3 n,float received){
 if(!spritePbrEnabled||!surfaceMaterialValid||!spritePackedMaterial||!spriteNormalEnabled||spriteOwner<=0.0||dot(n,n)<1e-8)return base;
 vec3 albedo=displayToLinear(albedoEncoded);
 // Artwork contains baked shadows: lift transmission tint without changing hue.
 // Cap amplification at 4x and preserve black rather than inventing light.
 vec3 transmissionTint=albedo/max(.25,max(albedo.r,max(albedo.g,albedo.b)));
 vec3 v=normalize(fogEye-fogPosition),l=normalize(spriteSunDirection);
 float m=surfaceMaterial.x,s=m,thickness=surfaceMaterial.y;
 if(spriteSplitMaterial){
  int a=int(floor(m*255.0+0.5));
  m=float(a>>7);s=float(a&127)/127.0;
 }
 float gloss=spriteSplitMaterial?max(m,s):m;
 float nl=max(0.0,dot(n,l));
 vec3 h=v+l;float spec=0.0;
 if(gloss>0.0&&spritePbrStrength.y>0.0&&received>0.0&&nl>0.0&&dot(h,h)>1e-8)spec=pow(max(0.0,dot(n,normalize(h))),mix(8.0,256.0,s*s))*nl;
 vec3 f0=mix(vec3(.04),albedo,m)*gloss;
 vec3 specularContribution=spriteSunRadiance*f0*spec*spritePbrStrength.y*received;
 vec3 result=base;
 // Thin, nonmetal regions transmit the backlight; received shadows suppress it.
 float back=max(0.0,dot(-n,l));
 // Retain an adjustable off-axis floor; the directional lobe fades at 30 degrees.
 float transmission=back*mix(spriteSssMinimum,1.0,smoothstep(.8660254,1.0,dot(v,-l)));
 vec3 sssContribution=transmissionTint*spriteSunRadiance*transmission*(1.0-thickness)*(1.0-m)*spritePbrStrength.z*received;
 result+=sssContribution;
 if(spriteSssDebug)spriteSssOnly=sssContribution*(1.0-(spriteSkyAvailable?m*spritePbrStrength.x:0.0));
 if(spriteSkyAvailable&&m>0.0&&spritePbrStrength.x>0.0){
  vec3 r=reflect(-v,n);
  vec2 uv=vec2(fract(.5+(atan(r.x,-r.z)-spriteSkyInfo.x)/6.28318530718),.5-asin(clamp(r.y,-1.0,1.0))/3.14159265359);
  vec3 environment=textureLod(spriteSkyMap,uv,(1.0-s)*spriteSkyInfo.y).rgb;
  result=mix(result,environment*albedo,m*spritePbrStrength.x);
 }
 // Direct sunlight is independent of the environment blend.
 result+=specularContribution;
 // Reuse the existing per-sprite light mask, range and distance falloff.
 // Local light is added after the sky blend so metallic surfaces retain highlights.
 // Sun shadow visibility must not occlude unrelated point/segment lights.
 float localSssWeight=(1.0-thickness)*(1.0-m)*spritePbrStrength.z;
 if((spritePbrStrength.y>0.0&&gloss>0.0)||localSssWeight>0.0)for(int block=0;block<4;block++){
  uint mask=spriteLightMask[block];
  for(int bit=0;bit<32;bit++,mask>>=1u){
   if(mask==0u)break;if((mask&1u)==0u)continue;int i=block*32+bit;
   vec3 d=fogPosition-spriteLights[i].xyz;vec4 axis=spriteLightAxes[i];
   d-=axis.xyz*clamp(dot(d,axis.xyz)*axis.w,0.0,1.0);
   float distance2=dot(d,d),range=distance2*spriteLights[i].w;
   if(range>=1.0)continue;
   vec3 localL=-d*inversesqrt(max(distance2,1e-8));
   float reach=1.0-range;
   vec3 radiance=spriteLightColors[i].rgb*(reach*reach*spriteLightColors[i].a);
   float signedNL=dot(n,localL);
   if(localSssWeight>0.0&&signedNL<0.0){
    float forward=mix(spriteSssMinimum,1.0,smoothstep(.8660254,1.0,dot(v,-localL)));
    vec3 localSss=transmissionTint*radiance*(-signedNL*forward*localSssWeight);
    result+=localSss;
    if(spriteSssDebug)spriteSssOnly+=localSss;
   }
   float localNL=max(0.0,signedNL);if(localNL<=0.0||spritePbrStrength.y<=0.0||gloss<=0.0)continue;
   vec3 localH=v+localL;float h2=dot(localH,localH);if(h2<1e-8)continue;
   float highlight=pow(max(0.0,dot(n,localH*inversesqrt(h2))),mix(8.0,256.0,s*s))*localNL;
   vec3 localSpecular=radiance*f0*(highlight*spritePbrStrength.y);
   result+=localSpecular;
   if(spriteSpecularDebug)specularContribution+=localSpecular;
  }
 }
 if(spriteSpecularDebug)spriteSpecularOnly=specularContribution;
 return result;
}
void main() {
    // Derivatives must be evaluated before alpha discard, once for all lights.
    vec3 bodyNormal=vec3(0);
    if(spriteOwner>0.0&&(any(notEqual(spriteLightMask,uvec4(0)))||(spriteShadows&&spriteShadowAmount>0.0)||(spritePbrEnabled&&pbrDetail>0.0&&spriteNormalEnabled&&spritePackedMaterial))){bodyNormal=spriteBodyNormal();if(spriteNormalEnabled)bodyNormal=spriteMappedNormal(bodyNormal);}
    float selfVisibility=1.0;
    if(spriteShadows && spriteOwner>0.0 && spriteShadowAmount>0.0)selfVisibility=spriteSelfVisibility(bodyNormal);
    if(reflectionClip>0.5 && worldHeight<reflectionHeight) discard;
    vec4 sampleColor = texture(tex, spriteAtlas?clamp(tc,0.0,1.0)*atlasColor.xy+atlasColor.zw:tc);
    if (sampleColor.a < alphaCutoff || (softOnly && sampleColor.a>=.995)) discard;
    vec4 encoded = sampleColor * tint;
    encoded.a *= vertexAlpha;
    if(spriteCutout)encoded.a=1.0;
    if(detailEnabled){
        // Explicit gradients keep mip selection defined across distance branches.
        vec2 baseUV=tc*detailDimensions;
        vec2 gradientX=dFdx(baseUV),gradientY=dFdy(baseUV);
        float weight=detailLodRange.y>detailLodRange.x
            ?1.0-smoothstep(detailLodRange.x,detailLodRange.y,distance(fogPosition,fogEye)):1.0;
        if(weight>0.0){
            vec3 factor=vec3(1.0);
            for(int i=0;i<3;i++){
                int id=int(materialIds[i]+0.5);
                if(id>0)factor+=(textureGrad(detailTex,vec3(baseUV*detailScales[id],float(id)),gradientX*detailScales[id],gradientY*detailScales[id]).rgb*2.0-1.0)*materialWeights[i];
            }
            encoded.rgb*=mix(vec3(1.0),factor,weight);
        }
    }
    float receivedVisibility=1.0;
    if(spriteShadows && spriteOwner>0.0)receivedVisibility=spriteVisibility();
    float visibility=1.0;
    if(spriteShadows && spriteOwner>0.0 && spriteShadowAmount>0.0)visibility=min(mix(1.0,receivedVisibility,spriteShadowAmount),selfVisibility);
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
    if(pbrDetail>0.0)linearColor=mix(linearColor,spritePbr(linearColor,(sampleColor*tint).rgb,bodyNormal,receivedVisibility),localFog*pbrDetail);
    if(spriteSpecularDebug&&pbrDetail>0.0&&surfaceMaterialValid&&spritePackedMaterial&&spriteOwner>0.0)linearColor=spriteSpecularOnly*localFog*pbrDetail;
    if(spriteSssDebug&&pbrDetail>0.0&&surfaceMaterialValid&&spritePackedMaterial&&spriteOwner>0.0)linearColor=spriteSssOnly*localFog*pbrDetail;
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
  if(spriteShadows&&spriteShadowAmount>0.0)vertexVisibility=min(mix(1.0,spriteVisibility(),spriteShadowAmount),spriteSelfVisibility(n));
  if(any(notEqual(spriteLightMask,uvec4(0))))vertexLocalLight=spriteLocalLight(n);
 }
)GLSL");
 }else{
  replace("in vec2 tc;flat in float vertexAlpha;","in vec2 tc;flat in float vertexAlpha;\nflat in float vertexShadeWeight;\nin float vertexVisibility;\nin vec4 vertexLocalLight;");
  replace("if(spriteOwner>0.0&&(any", "if((vertexShadeWeight<1.0||(spritePbrEnabled&&pbrDetail>0.0&&spriteNormalEnabled&&spritePackedMaterial))&&spriteOwner>0.0&&(any");
  replace("if(spriteShadows && spriteOwner>0.0 && spriteShadowAmount>0.0)selfVisibility", "if(vertexShadeWeight<1.0&&spriteShadows && spriteOwner>0.0 && spriteShadowAmount>0.0)selfVisibility");
  replace("if(spriteShadows && spriteOwner>0.0)receivedVisibility", "if((vertexShadeWeight<1.0||(spritePbrEnabled&&pbrDetail>0.0&&spritePackedMaterial&&spriteNormalEnabled))&&spriteShadows && spriteOwner>0.0)receivedVisibility");
  replace("if(spriteShadows && spriteOwner>0.0 && spriteShadowAmount>0.0)visibility", "if(vertexShadeWeight<1.0&&spriteShadows && spriteOwner>0.0 && spriteShadowAmount>0.0)visibility");
  replace("if(spriteOwner>0.0&&any(notEqual(spriteLightMask,uvec4(0))))local", "if(vertexShadeWeight<1.0&&spriteOwner>0.0&&any(notEqual(spriteLightMask,uvec4(0))))local");
  replace("visibility=mix(visibility,1.0,local.a);", "visibility=mix(visibility,vertexVisibility,vertexShadeWeight);local=mix(local,vertexLocalLight,vertexShadeWeight);\n    visibility=mix(visibility,1.0,local.a);");
 }
 return s;
}

// Compile the OFF path with literal constants so lighting/shadow branches and
// their attributes/samplers can be eliminated by the driver, not merely skipped.
static std::string spriteShaderSource(const char*source,bool shadows,bool lighting,bool atlas=true,bool pbr=false){
 std::string s=(shadows||lighting)?spriteVertexLodSource(source):std::string(source);
 auto replace=[&](const char*a,const char*b){auto at=s.find(a);if(at!=std::string::npos)s.replace(at,strlen(a),b);};
 if(pbr)s.insert(s.find('\n')+1,"#define PBR_DETAIL_LOD\n");
 if(!pbr){
  replace("uniform bool spritePbrEnabled;","const bool spritePbrEnabled=false;");
  // Remove the PBR function, call and uniforms from the source, not merely a
  // runtime branch. Some mobile drivers retain unused helper uniforms.
  auto first=s.find("vec3 spritePbr(");if(first!=std::string::npos){auto last=s.find("void main()",first);s.erase(first,last-first);}
  replace("    if(pbrDetail>0.0)linearColor=mix(linearColor,spritePbr(linearColor,(sampleColor*tint).rgb,bodyNormal,receivedVisibility),localFog*pbrDetail);","");
  replace("    if(spriteSpecularDebug&&pbrDetail>0.0&&surfaceMaterialValid&&spritePackedMaterial&&spriteOwner>0.0)linearColor=spriteSpecularOnly*localFog*pbrDetail;","");
  replace("    if(spriteSssDebug&&pbrDetail>0.0&&surfaceMaterialValid&&spritePackedMaterial&&spriteOwner>0.0)linearColor=spriteSssOnly*localFog*pbrDetail;","");
  replace("uniform bool spriteSssDebug;","");replace("vec3 spriteSssOnly=vec3(0);","");
  replace("uniform bool spriteSpecularDebug;","");replace("vec3 spriteSpecularOnly=vec3(0);","");
  for(const char*declaration:{"uniform float spriteSssMinimum;","uniform bool spriteSkyAvailable;","uniform sampler2D spriteSkyMap;","uniform vec2 spriteSkyInfo;","uniform vec3 spritePbrStrength, spriteSunRadiance;"})replace(declaration,"");
 }
 if(!atlas)replace("uniform bool spriteAtlas;","const bool spriteAtlas=false;");
 if(!lighting){replace("uniform int spriteLightCount;","const int spriteLightCount=0;");replace("flat in highp uvec4 spriteLightMask;","const uvec4 spriteLightMask=uvec4(0);");}
 if(!shadows){replace("uniform bool spriteShadows;","const bool spriteShadows=false;");replace("uniform float spriteShadowLodDistance;","const float spriteShadowLodDistance=0.0;");}
 if(!shadows&&!lighting&&!pbr)replace("uniform bool spriteNormalEnabled;","const bool spriteNormalEnabled=false;");
 return s;
}
static std::string spriteShaderSource(const char*source,bool enhanced){return spriteShaderSource(source,enhanced,enhanced);}

// The far mesh path uses only the visible face normal and the sun direction.
static std::string modelShadowLod(std::string s,bool far){
 auto at=s.find("vec3 displayToLinear(");
 if(at==std::string::npos)return s;
 s.insert(at,R"(
float meshFaceVisibility(){
 vec3 n=cross(dFdx(fogPosition),dFdy(fogPosition));
 if(dot(n,fogEye-fogPosition)<0.0)n=-n;
 return spriteSelfVisibility(n*inversesqrt(max(dot(n,n),1e-16)));
}
)");
 const std::string marker="encoded.rgb*=visibility;";
 s.replace(s.find(marker),marker.size(),far?
  "visibility=meshFaceVisibility();encoded.rgb*=visibility;":
  "if(spriteShadows)visibility=mix(meshFaceVisibility(),visibility,pbrDetail);encoded.rgb*=visibility;");
 return s;
}
static std::string modelShaderSource(const char*source,bool shadows,bool lighting){
 std::string s=spriteShaderSource(source,false,false,false,true);
 auto replace=[&](const char*a,const char*b){auto at=s.find(a);if(at!=std::string::npos)s.replace(at,strlen(a),b);};
 if(shadows){replace("const bool spriteShadows=false;","uniform bool spriteShadows;");replace("const float spriteShadowLodDistance=0.0;","uniform float spriteShadowLodDistance;");}
 if(lighting){replace("const int spriteLightCount=0;","uniform int spriteLightCount;");replace("const uvec4 spriteLightMask=uvec4(0);","flat in highp uvec4 spriteLightMask;");}
 auto a=s.find("vec3 spriteBodyNormal(){");auto b=s.find("// Authoring Y",a);
 if(a!=std::string::npos&&b!=std::string::npos)s.replace(a,b-a,"vec3 spriteBodyNormal(){vec3 n=cross(dFdx(fogPosition),dFdy(fogPosition));if(dot(n,n)<1e-12)return vec3(0,0,1);n=normalize(n);return dot(n,fogEye-fogPosition)<0.0?-n:n;}\n");
 replace("vec4 packed=texture(spriteNormalMap,spriteAtlas?clamp(uv,0.0,1.0)*atlasNormal.xy+atlasNormal.zw:uv);","vec4 packed=texture(spriteNormalMap,fract(uv));");
 replace("owner>0.0&&abs(owner-spriteOwner)>.5", "owner>0.0");
 replace("||any(lessThan(uv,vec2(0)))||any(greaterThan(uv,vec2(1)))","");
 return shadows?modelShadowLod(s,false):s;
}
