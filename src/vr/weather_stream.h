#pragma once
// Weather has few original textures, no materials/owners/atlas attributes.
// Keep local UVs and original texture names together; upload once for both eyes.
struct WeatherStream {
 struct Range{GLuint texture;GLint first;GLsizei count;};
 std::vector<Vertex> packed;std::vector<Range> ranges;
 GLuint buffers[3]{};unsigned cursor=0;size_t capacity[3]{};
 void upload(const std::map<GLuint,std::vector<Vertex>>&batches){
  packed.clear();ranges.clear();size_t total=0;for(const auto&b:batches)total+=b.second.size();packed.reserve(total);
  for(const auto&b:batches)if(!b.second.empty()){ranges.push_back({b.first,(GLint)packed.size(),(GLsizei)b.second.size()});packed.insert(packed.end(),b.second.begin(),b.second.end());}
  if(packed.empty())return;cursor=(cursor+1)%3;if(!buffers[cursor])glGenBuffers(1,&buffers[cursor]);
  size_t bytes=packed.size()*sizeof(Vertex);if(capacity[cursor]<bytes)capacity[cursor]=(bytes+65535)&~size_t(65535);
  glBindBuffer(GL_ARRAY_BUFFER,buffers[cursor]);glBufferData(GL_ARRAY_BUFFER,capacity[cursor],nullptr,GL_STREAM_DRAW);glBufferSubData(GL_ARRAY_BUFFER,0,bytes,packed.data());
 }
 void draw(){if(ranges.empty())return;attributes(buffers[cursor]);for(auto&r:ranges){glBindTexture(GL_TEXTURE_2D,r.texture);glDrawArrays(GL_TRIANGLES,r.first,r.count);}}
};
