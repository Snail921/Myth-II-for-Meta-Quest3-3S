#pragma once
static bool shadowDebugEnabled=false;
static const char* shadowDebugVS=R"(#version 300 es
layout(location=0) in vec3 pos;uniform mat4 mvp;out vec3 world;
void main(){world=pos;gl_Position=mvp*vec4(pos,1);})";
static const char* shadowDebugFS=R"(#version 300 es
precision highp float;precision highp int;
in vec3 world;uniform mat4 sunMatrix;
uniform sampler2D staticDepth,liveDepth,liveOwner;uniform float bias;uniform int samples;
out vec4 color;
void main(){
 vec3 p=(sunMatrix*vec4(world,1)).xyz*.5+.5;
 if(any(lessThanEqual(p,vec3(0)))||any(greaterThanEqual(p,vec3(1))))discard;
 const vec2 offsets[9]=vec2[9](vec2(0),vec2(1,1),vec2(-1,-1),vec2(1,-1),vec2(-1,1),vec2(1,0),vec2(-1,0),vec2(0,1),vec2(0,-1));
 vec2 texel=1.0/vec2(textureSize(staticDepth,0));vec3 sum=vec3(0);float hits=0.;
 for(int i=0;i<9;i++){if(i>=samples)break;vec2 uv=p.xy+offsets[i]*texel;
 if(any(lessThan(uv,vec2(0)))||any(greaterThan(uv,vec2(1))))continue;
 bool fixedShadow=p.z-bias>texture(staticDepth,uv).r;
 uv=p.xy+offsets[i]/vec2(textureSize(liveDepth,0));
 vec3 b=floor(texture(liveOwner,uv).rgb*255.+.5);float owner=b.r+b.g*256.+b.b*65536.;
 bool liveShadow=owner>0.&&p.z-bias>texture(liveDepth,uv).r;
 if(!fixedShadow&&!liveShadow)continue;
 // Cyan: fixed mesh/terrain. Magenta: moving model. Yellow: sprite.
 vec3 tint=liveShadow?(owner>16777214.5?vec3(1,0,1):vec3(1,1,0)):vec3(0,1,1);
 if(fixedShadow&&liveShadow)tint=vec3(1);
 sum+=tint;hits+=1.;
 }
 if(hits==0.)discard;
 color=vec4(sum/hits,.6*hits/float(max(samples,1)));
})";
