#pragma once
#include "vertex_lod_profile.h"
static vertexlod::Counts vertexLodCounts[4];
static void reportVertexLod(){
 if(!diagnosticEnabled)return;
 for(int i=0;i<4;i++){auto&c=vertexLodCounts[i];if(c.sampled)LOG("Vertex LOD kind=%s source=%s cpu_estimate=1 sampled_eye_pass=1 total=%u detail=%u transition=%u vertex=%u nonreceiver=%u detail_near=%u detail_large=%u detail_forced=%u vertex_pct=%.1f",i%2?"reflection":"normal",i<2?"sprites":"effects",c.total,c.detail,c.transition,c.vertex,c.nonreceiver,c.near,c.large,c.forced,100.f*c.vertex/std::max(1u,c.total-c.nonreceiver));c={};}
}

// Geometry bounds remain valid across camera/light updates; six vertices share
// a sphere enclosing the entire billboard (including HD extents and flips).
struct SpriteLightBoundsStream {
 std::vector<std::array<float,4>> bounds;GLuint buffers[3]{};
 void append(const Vertex*v,size_t count){
  if(!enhancedSpritePath||!count)return;float lo[3]={v[0].x,v[0].y,v[0].z},hi[3]={v[0].x,v[0].y,v[0].z};
  for(size_t i=1;i<count;i++){float p[3]={v[i].x,v[i].y,v[i].z};for(int j=0;j<3;j++){lo[j]=std::min(lo[j],p[j]);hi[j]=std::max(hi[j],p[j]);}}
  float dx=hi[0]-lo[0],dy=hi[1]-lo[1],dz=hi[2]-lo[2];std::array<float,4>b{(lo[0]+hi[0])*.5f,(lo[1]+hi[1])*.5f,(lo[2]+hi[2])*.5f,sqrtf(dx*dx+dy*dy+dz*dz)*.5f+.001f};bounds.insert(bounds.end(),count,b);
 }
 // One eye pass per source/view per reporting interval, no GPU readback.
 void count(vertexlod::Counts&c,size_t first,size_t count,bool receiver){
  for(size_t i=first;i<first+count&&i<bounds.size();i+=6){auto&b=bounds[i];float dx=b[0]-camera.x,dy=b[1]-camera.y,dz=b[2]-camera.z;c.add(sqrtf(dx*dx+dy*dy+dz*dz),b[3],receiver,shadowDebugEnabled);}
 }
 void upload(unsigned cursor){if(!enhancedSpritePath)return;if(bounds.empty())return;if(!buffers[cursor])glGenBuffers(1,&buffers[cursor]);glBindBuffer(GL_ARRAY_BUFFER,buffers[cursor]);glBufferData(GL_ARRAY_BUFFER,bounds.size()*sizeof(bounds[0]),bounds.data(),GL_STREAM_DRAW);}
 void bind(unsigned cursor){if(!enhancedSpritePath)return;glBindBuffer(GL_ARRAY_BUFFER,buffers[cursor]);glEnableVertexAttribArray(5);glVertexAttribPointer(5,4,GL_FLOAT,GL_FALSE,sizeof(std::array<float,4>),nullptr);glUniform1i(glGetUniformLocation(program,"spriteLightCull"),1);}
 static void end(){if(!enhancedSpritePath)return;glDisableVertexAttribArray(5);glUniform1i(glGetUniformLocation(program,"spriteLightCull"),0);}
};
