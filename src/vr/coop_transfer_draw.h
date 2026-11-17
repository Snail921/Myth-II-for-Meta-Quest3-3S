static void drawCoopTransfer(){
 if(!coopMenuOpen||!infoFont)return;
 glDisable(GL_DEPTH_TEST);glDepthMask(GL_FALSE);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);attributes(vbo);glUniform1f(glGetUniformLocation(program,"alphaCutoff"),.01f);
 auto point=[&](float x,float y,float u=0,float v=0){auto p=add(camera,turn(add(coopCenter,add(times(coopRight,x),times(coopUp,y))),cameraYaw));return Vertex{p.x,p.y,p.z,u,v};};
 auto mesh=[&](const std::vector<Vertex>&v,GLuint tex,float r,float g,float b){glBindTexture(GL_TEXTURE_2D,tex);glUniform4f(glGetUniformLocation(program,"tint"),r,g,b,1);glBufferData(GL_ARRAY_BUFFER,v.size()*sizeof(Vertex),v.data(),GL_STREAM_DRAW);glDrawArrays(GL_TRIANGLES,0,v.size());};
 for(unsigned i=0;i<coopChoices.size();i++){
  float top=coopTop()-i*coopRowHeight(),bottom=top-.059f;auto a=point(-.29f,top),b=point(.29f,top),c=point(-.29f,bottom),d=point(.29f,bottom);
  float r=.12f,g=.12f,blue=.12f;if(i<coopChoices.size()){auto&p=coopChoices[i];r=p.color[0]/255.f;g=p.color[1]/255.f;blue=p.color[2]/255.f;}
  float factor=coopHover==(int)i?1.f:.70f;mesh({a,b,c,b,d,c},whiteTexture,r*factor,g*factor,blue*factor);
  const char*name=coopChoices[i].name;float pixel=.0015f,width=0;for(const unsigned char*p=(const unsigned char*)name;*p;p++)width+=infoAdvance[*p]*pixel;
  if(width>.53f)pixel*=.53f/width;width=0;for(const unsigned char*p=(const unsigned char*)name;*p;p++)width+=infoAdvance[*p]*pixel;
  float pen=-width*.5f,y=top-.007f;std::vector<Vertex> glyphs;
  for(const unsigned char*p=(const unsigned char*)name;*p;p++){float u=(*p%16)/16.f,v=(*p/16)/16.f;auto a=point(pen-4*pixel,y,u,v),b=point(pen+28*pixel,y,u+1.f/16,v),c=point(pen-4*pixel,y-32*pixel,u,v+1.f/16),d=point(pen+28*pixel,y-32*pixel,u+1.f/16,v+1.f/16);glyphs.insert(glyphs.end(),{a,b,c,b,d,c});pen+=infoAdvance[*p]*pixel;}
  mesh(glyphs,infoFont,1,1,1);
 }
 if(pointerValid){std::vector<Vertex> line{{pointerStart.x,pointerStart.y,pointerStart.z,0,0},{pointerEnd.x,pointerEnd.y,pointerEnd.z,0,0}};glBindTexture(GL_TEXTURE_2D,whiteTexture);glUniform4f(glGetUniformLocation(program,"tint"),1,1,0,1);glBufferData(GL_ARRAY_BUFFER,line.size()*sizeof(Vertex),line.data(),GL_STREAM_DRAW);glDrawArrays(GL_LINES,0,line.size());}
 if(pointerValid&&coopHover>=0){
  // UI cursor uses the panel plane, not the terrain's horizontal plane.
  std::vector<Vertex> ring;auto right=turn(coopRight,cameraYaw),up=turn(coopUp,cameraYaw);
  for(int i=0;i<32;i++){float angle=i*6.2831853f/32;auto p=add(pointerEnd,add(times(right,.004f*cosf(angle)),times(up,.004f*sinf(angle))));ring.push_back({p.x,p.y,p.z,0,0});}
  glBindTexture(GL_TEXTURE_2D,whiteTexture);glUniform4f(glGetUniformLocation(program,"tint"),1,1,1,1);glBufferData(GL_ARRAY_BUFFER,ring.size()*sizeof(Vertex),ring.data(),GL_STREAM_DRAW);glDrawArrays(GL_LINE_LOOP,0,ring.size());
 }
 glDepthMask(GL_TRUE);glEnable(GL_DEPTH_TEST);glDisable(GL_BLEND);glUniform1f(glGetUniformLocation(program,"alphaCutoff"),.5f);glUniform4f(glGetUniformLocation(program,"tint"),1,1,1,1);
}
