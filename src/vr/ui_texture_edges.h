#pragma once
// Original color-key pixels retain blue RGB at alpha=0. Linear filtering must
// sample neighboring artwork instead. Alpha and all visible pixels stay exact.
static std::vector<unsigned char> uiTexturePixels(const unsigned char*source,unsigned w,unsigned h){
 std::vector<unsigned char> pixels(source,source+size_t(w)*h*4);
 std::vector<unsigned> queue;queue.reserve(size_t(w)*h);
 std::vector<unsigned char> visited(size_t(w)*h,0);
 for(unsigned i=0;i<w*h;i++)if(pixels[i*4+3]){visited[i]=1;queue.push_back(i);}
 for(size_t head=0;head<queue.size();head++){
  unsigned i=queue[head],x=i%w,y=i/w;
  auto spread=[&](unsigned j){if(visited[j])return;visited[j]=1;memcpy(pixels.data()+j*4,pixels.data()+i*4,3);queue.push_back(j);};
  if(x)spread(i-1);if(x+1<w)spread(i+1);if(y)spread(i-w);if(y+1<h)spread(i+w);
 }
 return pixels;
}
