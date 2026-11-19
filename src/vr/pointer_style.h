#pragma once
namespace pointer_style {
static float scale=1;
static void load(const char*root){scale=1;FILE*f=fopen((std::string(root)+"/pointer-thickness").c_str(),"rb");if(f){int v;char extra;if(fscanf(f,"%d %c",&v,&extra)==1&&v>=10&&v<=40)scale=v/10.f;fclose(f);}}
// Scoped line width: all unrelated UI retains its original width.
struct Lines {GLfloat previous=1;Lines(){glGetFloatv(GL_LINE_WIDTH,&previous);glLineWidth(scale);}~Lines(){glLineWidth(previous);}};
}
