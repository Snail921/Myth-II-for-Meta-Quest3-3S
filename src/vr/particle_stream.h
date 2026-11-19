#pragma once
#include "sprite_light_bounds.h"
// Preserve back-to-front alpha order; only adjacent identical draws may merge.
struct ParticleStream {
 struct Range{GLuint texture;float alpha;GLint first;GLsizei count;unsigned owner;sprite_atlas::Resolved resolved;};
 SpriteLightBoundsStream lightBounds;sprite_atlas::AttributeStream atlasAttributes;
 std::vector<Vertex> packed;std::vector<Range> ranges;GLuint buffers[3]{};unsigned cursor=0;
 void clear(){atlasAttributes.data.clear();lightBounds.bounds.clear();packed.clear();ranges.clear();}
 void append(GLuint texture,float alpha,const Vertex*vertices,size_t count,unsigned owner=0){if(!count)return;if(!enhancedSpritePath)owner=0;lightBounds.append(vertices,count);auto resolved=sprite_atlas::resolve(texture,hd::textures.count(texture)!=0);texture=resolved.texture;atlasAttributes.append(resolved,count);GLint first=(GLint)packed.size();packed.insert(packed.end(),vertices,vertices+count);if(!ranges.empty()&&ranges.back().texture==texture&&ranges.back().alpha==alpha&&ranges.back().owner==owner&&sprite_atlas::key(ranges.back().resolved)==sprite_atlas::key(resolved))ranges.back().count+=(GLsizei)count;else ranges.push_back({texture,alpha,first,(GLsizei)count,owner,resolved});}
 void upload(){if(packed.empty())return;cursor=(cursor+1)%3;if(!buffers[cursor])glGenBuffers(1,&buffers[cursor]);attributes(buffers[cursor]);glBufferData(GL_ARRAY_BUFFER,packed.size()*sizeof(Vertex),packed.data(),GL_STREAM_DRAW);lightBounds.upload(cursor);atlasAttributes.upload(cursor);}
 void draw(GLint tint,bool reflected=false){
  if(diagnosticEnabled&&enhancedSpritePath){auto&c=vertexLodCounts[reflected?3:2];if(!c.sampled){c.sampled=true;for(auto&r:ranges)lightBounds.count(c,r.first,r.count,r.owner>0);}}
  if(ranges.empty())return;attributes(buffers[cursor]);lightBounds.bind(cursor);atlasAttributes.bind(cursor);GLint atlasLocation=sprite_atlas::enabled?glGetUniformLocation(program,"spriteAtlas"):-1;int previousAtlas=-1;sprite_material::StreamBinding materialState;for(auto&r:ranges){if(enhancedSpritePath)glVertexAttrib1f(4,float(r.owner));if(sprite_atlas::enabled&&previousAtlas!=int(r.resolved.atlas)){glUniform1i(atlasLocation,r.resolved.atlas);previousAtlas=r.resolved.atlas;}materialState.bindResolved(program,r.resolved.material);glBindTexture(GL_TEXTURE_2D,r.texture);glUniform4f(tint,1,1,1,r.alpha);glDrawArrays(GL_TRIANGLES,r.first,r.count);}sprite_atlas::AttributeStream::end();SpriteLightBoundsStream::end();sprite_material::unbind(program);glVertexAttrib1f(4,0);}
};
