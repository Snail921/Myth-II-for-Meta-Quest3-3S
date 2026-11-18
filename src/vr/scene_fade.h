#pragma once
static GLuint sceneFadeProgram=0;
static float sceneFade=0;
static double sceneFadeStart=-1;
static void initSceneFade(){
 if(sceneFadeProgram)return;
 const char*vs="#version 300 es\nvoid main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);gl_Position=vec4(p*2.0-1.0,0,1);}";
 const char*fs="#version 300 es\nprecision mediump float;uniform float opacity;out vec4 color;void main(){color=vec4(0,0,0,opacity);}";
 GLuint v=shader(GL_VERTEX_SHADER,vs),f=shader(GL_FRAGMENT_SHADER,fs);sceneFadeProgram=glCreateProgram();glAttachShader(sceneFadeProgram,v);glAttachShader(sceneFadeProgram,f);glLinkProgram(sceneFadeProgram);glDeleteShader(v);glDeleteShader(f);GLint ok=0;glGetProgramiv(sceneFadeProgram,GL_LINK_STATUS,&ok);if(!ok)throw std::runtime_error("scene fade program");
}
static void restartSceneFade(){LOG("VR transition fade restart mono=%.3f",seconds());sceneFadeStart=seconds();sceneFade=0;}
static void updateSceneFade(){float previous=sceneFade;if(sceneFadeStart<0){sceneFadeStart=seconds();LOG("VR transition first scene/fade mono=%.3f",sceneFadeStart);}sceneFade=std::clamp(float((seconds()-sceneFadeStart)/.5),0.f,1.f);if(previous<1&&sceneFade>=1)LOG("VR transition fade complete mono=%.3f",seconds());}
static void drawSceneFade(){
 if(sceneFade>=1)return;
 glUseProgram(sceneFadeProgram);glUniform1f(glGetUniformLocation(sceneFadeProgram,"opacity"),1-sceneFade);glDisable(GL_DEPTH_TEST);glDepthMask(GL_FALSE);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glDrawArrays(GL_TRIANGLES,0,3);glDisable(GL_BLEND);glDepthMask(GL_TRUE);glEnable(GL_DEPTH_TEST);
}
