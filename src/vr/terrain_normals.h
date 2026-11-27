#pragma once
// Continuous lighting normals from the shared height grid. This changes shading,
// not geometry, collision or the depth maps used to cast terrain shadows.
static GLuint terrainNormalTexture=0;
static void releaseTerrainNormals(){if(terrainNormalTexture)glDeleteTextures(1,&terrainNormalTexture);terrainNormalTexture=0;}
static std::vector<unsigned char> terrainNormalPixels(unsigned w,unsigned h,const std::vector<float>&height){
 if(w<2||h<2||height.size()!=size_t(w)*h)return {};
 std::vector<unsigned char> pixels(size_t(w)*h*4);
 for(unsigned y=0;y<h;y++)for(unsigned x=0;x<w;x++){
  unsigned l=x?x-1:x,r=std::min(x+1,w-1),t=y?y-1:y,b=std::min(y+1,h-1);
  float nx=(height[y*w+r]-height[y*w+l])/float(r-l);
  float nz=-(height[b*w+x]-height[t*w+x])/float(b-t);
  float inv=1.f/sqrtf(nx*nx+1.f+nz*nz);float n[]={nx*inv,inv,nz*inv};
  for(int c=0;c<3;c++)pixels[(size_t(y)*w+x)*4+c]=(unsigned char)std::clamp(int(lroundf((n[c]*.5f+.5f)*255)),0,255);
  pixels[(size_t(y)*w+x)*4+3]=255;
 }
 return pixels;
}
static void buildTerrainNormals(unsigned w,unsigned h,const std::vector<float>&height){
 releaseTerrainNormals();if(!unifiedGroundShadows())return;
 auto pixels=terrainNormalPixels(w,h,height);if(pixels.empty())return;
 glGenTextures(1,&terrainNormalTexture);glBindTexture(GL_TEXTURE_2D,terrainNormalTexture);
 glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
 LOG("Unified terrain smooth normals grid=%ux%u bytes=%zu",w,h,pixels.size());
}
