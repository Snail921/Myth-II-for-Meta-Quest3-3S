// Called with the lightweight UI shader. Text reuses the original byte glyph atlas.
static void multiplayerText(const char*text,Vec3 origin,Vec3 right,Vec3 up,float pixel,float maxWidth,bool numeric=false,float alpha=1.f,float red=1,float green=1,float blue=1){
 float width=0;for(const unsigned char*p=(const unsigned char*)text;*p;p++)width+=(numeric?20:infoAdvance[*p])*pixel;
 if(!numeric&&width>maxWidth)pixel*=maxWidth/width;
 if(numeric){origin=add(origin,times(right,maxWidth-width));}
 std::vector<Vertex> vertices;float pen=0;
 auto point=[&](float x,float y,float u,float v){auto p=add(origin,add(times(right,x),times(up,y)));return Vertex{p.x,p.y,p.z,u,v};};
 for(const unsigned char*p=(const unsigned char*)text;*p;p++){
  float u=(*p%16)/16.f,v=(*p/16)/16.f;
  float glyphPen=pen+(numeric?(20-infoAdvance[*p])*.5f*pixel:0);
  auto a=point(glyphPen-4*pixel,0,u,v),b=point(glyphPen+28*pixel,0,u+1.f/16,v),c=point(glyphPen-4*pixel,-32*pixel,u,v+1.f/16),d=point(glyphPen+28*pixel,-32*pixel,u+1.f/16,v+1.f/16);
  vertices.insert(vertices.end(),{a,b,c,b,d,c});pen+=(numeric?20:infoAdvance[*p])*pixel;
 }
 glBindTexture(GL_TEXTURE_2D,infoFont);glUniform4f(glGetUniformLocation(program,"tint"),red,green,blue,alpha);glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(Vertex),vertices.data(),GL_STREAM_DRAW);glDrawArrays(GL_TRIANGLES,0,vertices.size());
}
static void multiplayerRect(Vec3 center,Vec3 right,Vec3 up,float w,float h,float r,float g,float b,float alpha){
 auto vertex=[&](float x,float y){auto p=add(center,add(times(right,x),times(up,y)));return Vertex{p.x,p.y,p.z,0,0};};
 auto a=vertex(-w/2,h/2),bb=vertex(w/2,h/2),c=vertex(-w/2,-h/2),d=vertex(w/2,-h/2);Vertex v[]={a,bb,c,bb,d,c};
 glBindTexture(GL_TEXTURE_2D,whiteTexture);glUniform4f(glGetUniformLocation(program,"tint"),r,g,b,alpha);glBufferData(GL_ARRAY_BUFFER,sizeof(v),v,GL_STREAM_DRAW);glDrawArrays(GL_TRIANGLES,0,6);
}
#include "chat_draw.h"
static void drawMultiplayerHud(){
 double now=seconds();bool scores=multiplayerHud.scores(now),disconnected=multiplayerHud.disconnected(now);
 if(!infoPanelShown||!infoFont||originalDialogMode||(!scores&&!disconnected))return;
 auto m=wristMapPlane();float s=cameraSpeeds.mapSize;const auto*p=multiplayerHud.data.data();unsigned n=scores?u32(p+4):0;
 unsigned columns=std::max(1u,(n+15)/16),rows=std::min(n,16u);
 float bottom=infoPanelBottom(m)+(infoPanelHeight+.008f)*s,step=.011f*s,w=.167f*s;
 auto position=[&](float x,float y){return add(m.center,add(times(m.right,x),times(m.up,y)));};
 float top=bottom+(scores?rows+1:0)*step;
 drawChatMessages(position(0,top+(disconnected?3:.5f)*step),m.right,m.up,s);
 if(disconnected){
  float alpha=multiplayerHud.noticeAlpha(now),pixel=.0003f*s,width=0;
  const char*text=(const char*)p+272;
  for(const unsigned char*c=(const unsigned char*)text;*c;c++)width+=infoAdvance[*c]*pixel;
  float shown=std::min(width,w*columns-.008f*s);
  // One empty row separates this notification from the rule/list header.
  glUniform1f(glGetUniformLocation(program,"alphaCutoff"),0.f);
  multiplayerRect(position(0,top+1.5f*step),m.right,m.up,w*columns,step,.30f,.035f,.10f,.78f*alpha);
  multiplayerText(text,position(-shown/2,top+2*step-.0030f*s),m.right,m.up,pixel,w*columns-.008f*s,false,alpha);
  glUniform1f(glGetUniformLocation(program,"alphaCutoff"),.01f);
 }
 if(!scores)return;
 const char*title=(const char*)p+16;const char*timer=(const char*)p+208;
 multiplayerRect(position(0,top-step/2),m.right,m.up,w*columns,step,.07f,.07f,.07f,.72f);
 multiplayerText(title,position(-w*columns/2+.003f*s,top-.0030f*s),m.right,m.up,.00022f*s,w*columns-.053f*s);
 multiplayerText(timer,position(w*columns/2-.048f*s,top-.0030f*s),m.right,m.up,.00022f*s,.045f*s,true);
 for(unsigned i=0;i<n;i++){
  auto row=p+400+i*132;float left=-w*columns/2+(i/16)*w,y=top-(i%16+1)*step;
  multiplayerRect(position(left+w/2,y-step/2),m.right,m.up,w-.002f*s,step-.001f*s,row[0]/255.f,row[1]/255.f,row[2]/255.f,row[3]/255.f);
  multiplayerText((const char*)row+4,position(left+.004f*s,y-.0030f*s),m.right,m.up,.00022f*s,.075f*s);
  // Bridge separates rule score and survival with two spaces. Keep independent
  // fixed-width numeric columns; percentage changes never resize the time.
  std::string score=(const char*)row+68,survival;
  auto split=score.rfind("  ");
  if(split!=std::string::npos){survival=score.substr(split+2);score.resize(split);}
  else if(!score.empty()&&score.back()=='%'){survival=score;score.clear();}
  multiplayerText(score.c_str(),position(left+.081f*s,y-.0030f*s),m.right,m.up,(u32(p+8)==3||u32(p+8)==4?.00016f:.00022f)*s,.059f*s,true);
  multiplayerText(survival.c_str(),position(left+.143f*s,y-.0030f*s),m.right,m.up,.00022f*s,.021f*s,true);
 }
}
static void drawMultiplayerNotice(const XrPosef&head){
 double now=seconds();if(!infoFont||!multiplayerHud.notice(now))return;
 float scale=multiplayerHud.disconnected(now)?2.f:1.f,alpha=multiplayerHud.noticeAlpha(now);
 // Shared cyclopean head pose, so both eyes see one stereo-correct panel.
 auto pose=worldPose(head);Vec3 right=rotated(pose.orientation,{1,0,0}),up=rotated(pose.orientation,{0,1,0});
 Vec3 center=add(Vec3{pose.position.x,pose.position.y,pose.position.z},rotated(pose.orientation,{0,0,-1.2f}));
 glDisable(GL_DEPTH_TEST);glDepthMask(GL_FALSE);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);attributes(vbo);glUniform1f(glGetUniformLocation(program,"alphaCutoff"),0.f);
 multiplayerRect(center,right,up,.62f*scale,.075f*scale,.30f,.035f,.10f,.78f*alpha);
 const char*text=(const char*)multiplayerHud.data.data()+272;float pixel=.0015f*scale,width=0;for(const unsigned char*p=(const unsigned char*)text;*p;p++)width+=infoAdvance[*p]*pixel;
 float shown=std::min(width,.58f*scale);multiplayerText(text,add(center,add(times(right,-shown/2),times(up,.024f*scale))),right,up,pixel,.58f*scale,false,alpha);
 glDepthMask(GL_TRUE);glEnable(GL_DEPTH_TEST);glDisable(GL_BLEND);glUniform1f(glGetUniformLocation(program,"alphaCutoff"),.5f);glUniform4f(glGetUniformLocation(program,"tint"),1,1,1,1);
}
