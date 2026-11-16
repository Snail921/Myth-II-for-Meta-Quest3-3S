#pragma once
// One orphaned upload per eye pass, instead of one allocation/upload per texture.
// Three persistent buffer names rotate; orphaning avoids overwriting queued GPU reads.
struct SpriteStream {
 struct Range{GLuint texture;GLint first;GLsizei count;};
 GLuint buffers[3]{};unsigned cursor=0;size_t capacity[3]{};
 std::vector<Vertex> packed;std::vector<Range> ranges;
 void upload(const std::map<GLuint,std::vector<Vertex>>&batches){
  packed.clear();ranges.clear();for(auto&b:batches)if(!b.second.empty()){ranges.push_back({b.first,(GLint)packed.size(),(GLsizei)b.second.size()});packed.insert(packed.end(),b.second.begin(),b.second.end());}
  cursor=(cursor+1)%3;if(!buffers[cursor])glGenBuffers(1,&buffers[cursor]);
  size_t bytes=packed.size()*sizeof(Vertex);if(!bytes)return;
  if(capacity[cursor]<bytes)capacity[cursor]=(bytes+65535)&~size_t(65535);
  glBindBuffer(GL_ARRAY_BUFFER,buffers[cursor]);glBufferData(GL_ARRAY_BUFFER,capacity[cursor],nullptr,GL_STREAM_DRAW);glBufferSubData(GL_ARRAY_BUFFER,0,bytes,packed.data());
 }
 void draw(){if(ranges.empty())return;attributes(buffers[cursor]);auto cutoff=glGetUniformLocation(program,"alphaCutoff"),soft=glGetUniformLocation(program,"softOnly");
  bool hasHD=false;for(auto&r:ranges){bool high=hd::textures.count(r.texture);hasHD|=high;glUniform1f(cutoff,high?.995f:.5f);glBindTexture(GL_TEXTURE_2D,r.texture);glDrawArrays(GL_TRIANGLES,r.first,r.count);}
  if(hasHD){float reflected=0;glGetUniformfv(program,glGetUniformLocation(program,"reflectionClip"),&reflected);glEnable(GL_BLEND);glBlendFunc(reflected>.5f?GL_ONE:GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glDepthMask(GL_FALSE);glUniform1i(soft,1);glUniform1f(cutoff,.003f);for(auto&r:ranges)if(hd::textures.count(r.texture)){glBindTexture(GL_TEXTURE_2D,r.texture);glDrawArrays(GL_TRIANGLES,r.first,r.count);}glUniform1i(soft,0);glDepthMask(GL_TRUE);glDisable(GL_BLEND);}
  glUniform1f(cutoff,.5f);
 }
};
