#pragma once
// Preserve back-to-front alpha order; only adjacent identical draws may merge.
struct ParticleStream {
 struct Range{GLuint texture;float alpha;GLint first;GLsizei count;};
 std::vector<Vertex> packed;std::vector<Range> ranges;GLuint buffers[3]{};unsigned cursor=0;
 void clear(){packed.clear();ranges.clear();}
 void append(GLuint texture,float alpha,const Vertex*vertices,size_t count){if(!count)return;GLint first=(GLint)packed.size();packed.insert(packed.end(),vertices,vertices+count);if(!ranges.empty()&&ranges.back().texture==texture&&ranges.back().alpha==alpha)ranges.back().count+=(GLsizei)count;else ranges.push_back({texture,alpha,first,(GLsizei)count});}
 void upload(){if(packed.empty())return;cursor=(cursor+1)%3;if(!buffers[cursor])glGenBuffers(1,&buffers[cursor]);attributes(buffers[cursor]);glBufferData(GL_ARRAY_BUFFER,packed.size()*sizeof(Vertex),packed.data(),GL_STREAM_DRAW);}
 void draw(GLint tint){if(ranges.empty())return;attributes(buffers[cursor]);for(auto&r:ranges){glBindTexture(GL_TEXTURE_2D,r.texture);glUniform4f(tint,1,1,1,r.alpha);glDrawArrays(GL_TRIANGLES,r.first,r.count);}}
};
