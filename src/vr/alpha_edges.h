#pragma once
#include <vector>
#include <cstdint>
#include <algorithm>
// Keep alpha intact. Linear sampling must not mix a transparent colour key
// (the original connector uses blue) into visible edge pixels.
inline void extendAlphaEdges(std::vector<uint8_t>&rgba,unsigned w,unsigned h,bool repeat){
 if(!w||!h||rgba.size()!=size_t(w)*h*4)return;
 auto source=rgba;
 for(unsigned y=0;y<h;y++)for(unsigned x=0;x<w;x++){
  size_t dst=(size_t(y)*w+x)*4;if(source[dst+3])continue;
  int best=100;size_t nearest=0;
  for(int dy=-2;dy<=2;dy++)for(int dx=-2;dx<=2;dx++){
   int distance=dx*dx+dy*dy;if(!distance||distance>=best)continue;
   int sx=int(x)+dx,sy=int(y)+dy;
   if(repeat){sx=(sx%int(w)+w)%w;sy=(sy%int(h)+h)%h;}
   else if(sx<0||sy<0||sx>=int(w)||sy>=int(h))continue;
   size_t at=(size_t(sy)*w+sx)*4;if(!source[at+3])continue;
   best=distance;nearest=at;
  }
  if(best<100)for(int c=0;c<3;c++)rgba[dst+c]=source[nearest+c];
 }
}
