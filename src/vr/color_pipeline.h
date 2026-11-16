#pragma once
// Original palette/filter/tint arithmetic is in display-encoded RGB. Decode
// only at the output boundary. The sRGB swapchain encodes on write and OpenXR
// decodes it for composition. Alpha is always linear.
static constexpr const char* mythVertexShader = "#version 300 es\nlayout(location=0) in vec3 pos;layout(location=1) in vec2 uv;layout(location=2) in vec3 detailIds;layout(location=3) in vec3 detailWeights;flat out vec3 materialIds;out vec3 materialWeights;uniform mat4 mvp;out vec2 tc;out float worldHeight;out vec3 fogPosition;void main(){gl_Position=mvp*vec4(pos,1);tc=uv;materialIds=detailIds;materialWeights=detailWeights;worldHeight=pos.y;fogPosition=pos;}";
static constexpr const char* mythFragmentShader = R"GLSL(#version 300 es
precision highp float;
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
out vec4 color;
vec3 displayToLinear(vec3 c) {
    c = clamp(c, 0.0, 1.0);
    return mix(pow((c + 0.055) / 1.055, vec3(2.4)), c / 12.92,
               lessThanEqual(c, vec3(0.04045)));
}
void main() {
    if(reflectionClip>0.5 && worldHeight<reflectionHeight) discard;
    vec4 sampleColor = texture(tex, tc);
    if (sampleColor.a < alphaCutoff || (softOnly && sampleColor.a>=.995)) discard;
    vec4 encoded = sampleColor * tint;
    if(detailEnabled){
        vec3 factor=vec3(1.0);
        for(int i=0;i<3;i++){
            int id=int(materialIds[i]+0.5);
            if(id>0)factor+=(texture(detailTex,vec3(tc*detailDimensions*detailScales[id],float(id))).rgb*2.0-1.0)*materialWeights[i];
        }
        encoded.rgb*=factor;
    }
    if(fogColorDensity.a>0.0){
        float transmission=exp(-fogColorDensity.a*distance(fogPosition,fogEye));
        float fogAmount=1.0-transmission;
        encoded.rgb=mix(encoded.rgb,fogColorDensity.rgb,fogAmount*fogAmount);
    }
    // Offscreen reflection color uses premultiplied alpha, including FX.
    color = vec4(displayToLinear(encoded.rgb) * (reflectionClip>0.5 ? encoded.a : 1.0), encoded.a);
}
)GLSL";
