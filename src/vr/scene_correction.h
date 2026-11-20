#pragma once
#include <string>
#include <cstdio>
#include <stdexcept>
namespace scene_correction {
static bool enabled=false;
static float strength=1;
static void load(const char*root){
 auto read=[&](const char*name,int fallback,int maximum){FILE*f=fopen((std::string(root)+"/"+name).c_str(),"rb");if(!f)return fallback;int v;char extra;bool ok=fscanf(f,"%d %c",&v,&extra)==1&&v>=0&&v<=maximum;fclose(f);return ok?v:fallback;};
 enabled=read("scene-correction.enabled",0,1)!=0;
 strength=read("scene-correction.strength",100,100)/100.f;
}
// OFF returns the original shader byte-for-byte: no uniform or extra GPU work.
static std::string fragment(std::string s,bool on){
 if(!on)return s;
 auto at=s.find("void main()");if(at==std::string::npos)throw std::runtime_error("correction main");
 s.insert(at,"uniform float sceneCorrectionStrength;\n");
 const char*anchor="encoded.rgb*=visibility;";
 at=s.find(anchor);if(at==std::string::npos)throw std::runtime_error("correction lighting");
 s.insert(at+std::char_traits<char>::length(anchor),R"GLSL(
    // Original artwork is display-encoded here. Grade before fog so the sky
    // and distant fog retain their authored colors; alpha is never modified.
    if(sceneCorrectionStrength>0.0)
        encoded.rgb=mix(encoded.rgb,clamp(encoded.rgb*(255.0/199.0),0.0,1.0),sceneCorrectionStrength);
)GLSL");
 return s;
}
static void apply(unsigned program,bool world=true){
 if(enabled)glUniform1f(glGetUniformLocation(program,"sceneCorrectionStrength"),world?strength:0.f);
}
}
