#pragma once
#include "sprite_light_bounds.h"
// Preserve back-to-front alpha order; only adjacent identical draws may merge.
struct ParticleStream {
 struct Range{GLuint texture;float alpha;GLint first;GLsizei count;unsigned owner;};
 SpriteLightBoundsStream lightBounds;
 std::vector<Vertex> packed;std::vector<Range> ranges;GLuint buffers[3]{};unsigned cursor=0;
 void clear(){lightBounds.bounds.clear();packed.clear();ranges.clear();}
 void append(GLuint texture,float alpha,const Vertex*vertices,size_t count,unsigned owner=0){if(!count)return;if(!enhancedSpritePath)owner=0;lightBounds.append(vertices,count);GLint first=(GLint)packed.size();packed.insert(packed.end(),vertices,vertices+count);if(!ranges.empty()&&ranges.back().texture==texture&&ranges.back().alpha==alpha&&ranges.back().owner==owner)ranges.back().count+=(GLsizei)count;else ranges.push_back({texture,alpha,first,(GLsizei)count,owner});}
 void upload(){if(packed.empty())return;cursor=(cursor+1)%3;if(!buffers[cursor])glGenBuffers(1,&buffers[cursor]);attributes(buffers[cursor]);glBufferData(GL_ARRAY_BUFFER,packed.size()*sizeof(Vertex),packed.data(),GL_STREAM_DRAW);lightBounds.upload(cursor);}
 void draw(GLint tint){if(ranges.empty())return;attributes(buffers[cursor]);lightBounds.bind(cursor);sprite_material::StreamBinding materialState;for(auto&r:ranges){if(enhancedSpritePath)glVertexAttrib1f(4,float(r.owner));materialState.bind(program,r.texture);glBindTexture(GL_TEXTURE_2D,r.texture);glUniform4f(tint,1,1,1,r.alpha);glDrawArrays(GL_TRIANGLES,r.first,r.count);}SpriteLightBoundsStream::end();sprite_material::unbind(program);glVertexAttrib1f(4,0);}
};
