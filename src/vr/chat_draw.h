static void chatOutlined(const char*text,Vec3 origin,Vec3 right,Vec3 up,float pixel,float width,float alpha=1){
 for(int y=-1;y<=1;y++)for(int x=-1;x<=1;x++)if(x||y)
  multiplayerText(text,add(origin,add(times(right,x*pixel*.8f),times(up,y*pixel*.8f))),right,up,pixel,width,false,alpha,.15f,.15f,.15f);
 multiplayerText(text,origin,right,up,pixel,width,false,alpha);
}
static std::vector<std::string> chatLines(const std::string&text,float pixel,float width){
 std::vector<std::string> lines;std::string line;float used=0;
 for(unsigned char c:text){if(c<32)c=' ';float w=infoAdvance[c]*pixel;if(used+w>width&&!line.empty()){lines.push_back(line);line.clear();used=0;}line+=char(c);used+=w;}
 if(!line.empty())lines.push_back(line);return lines;
}
static void drawChatMessages(Vec3 bottom,Vec3 right,Vec3 up,float s){
 double now=seconds();while(!chat.messages.empty()&&now-chat.messages.front().began>=11)chat.messages.pop_front();
 float y=0,pixel=.00024f*s,step=.011f*s,width=.28f*s;
 glUniform1f(glGetUniformLocation(program,"alphaCutoff"),0.f);
 for(auto i=chat.messages.rbegin();i!=chat.messages.rend();++i){
  float alpha=std::clamp(float(11-(now-i->began)),0.f,1.f);
  auto lines=chatLines(i->text,pixel,width-.063f*s);float height=std::max(size_t(1),lines.size())*step;
  Vec3 origin=add(bottom,add(times(right,-width/2),times(up,y+height)));
  auto vertex=[&](float x,float yy){auto p=add(origin,add(times(right,x),times(up,yy)));return Vertex{p.x,p.y,p.z,0,0};};
  float w=.055f*s,h=step;
  auto a=vertex(0,0),b=vertex(w,0),c=vertex(w+.005f*s,-h/2),d=vertex(w,-h),e=vertex(0,-h);
  Vertex v[]={a,b,c,a,c,e,c,d,e};
  glBindTexture(GL_TEXTURE_2D,whiteTexture);glUniform4f(glGetUniformLocation(program,"tint"),i->color[0]/255.f,i->color[1]/255.f,i->color[2]/255.f,.85f*alpha);
  glBufferData(GL_ARRAY_BUFFER,sizeof(v),v,GL_STREAM_DRAW);glDrawArrays(GL_TRIANGLES,0,9);
  chatOutlined(i->name.c_str(),add(origin,add(times(right,.002f*s),times(up,-.002f*s))),right,up,pixel,w-.003f*s,alpha);
  for(unsigned row=0;row<lines.size();row++)chatOutlined(lines[row].c_str(),add(origin,add(times(right,.063f*s),times(up,-.002f*s-row*step))),right,up,pixel,width-.063f*s,alpha);
  y+=height+.002f*s;
 }
 glUniform1f(glGetUniformLocation(program,"alphaCutoff"),.01f);
}
static void drawChatKeyboard(){
 if(!chat.open||!chat.canSend(seconds())||originalDialogMode||!infoFont)return;
 auto p=chatPlane();
 glDisable(GL_DEPTH_TEST);glDepthMask(GL_FALSE);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);attributes(vbo);
 glUniform1f(glGetUniformLocation(program,"alphaCutoff"),0.f);
 auto drawRays=[&](bool hits){

 glBindTexture(GL_TEXTURE_2D,whiteTexture);glUniform4f(glGetUniformLocation(program,"tint"),1,1,1,1);
 std::vector<Vertex> rays;
 auto line=[&](Vec3 a,Vec3 b){rays.push_back({a.x,a.y,a.z,0,0});rays.push_back({b.x,b.y,b.z,0,0});};
 for(int hand=0;hand<2;hand++){
  bool hit=hand?chatLeftHit:chatRightHit;
  if(hit!=hits)continue;
  int pointerHand=cameraSpeeds.swapHands?1:0;
  if(hand!=pointerHand&&!hit)continue;if(hand==pointerHand&&!pointerValid)continue;
  Vec3 start=hit?chatRayStart[hand]:pointerStart,end=hit?chatRayEnd[hand]:pointerEnd;
  if(hit||infoPanelHovered||mapHovered||originalDialogHovered||radial.kind)glUniform4f(glGetUniformLocation(program,"tint"),1,1,1,1);
  else if(hoverID&&!hoverFriendly)glUniform4f(glGetUniformLocation(program,"tint"),1,.05f,.05f,1);
  else if(hoverItem&&pickupGreen())glUniform4f(glGetUniformLocation(program,"tint"),0,1,.1f,1);
  else uiAccent(1);
  rays.clear();line(start,end);
  if(hit)for(int i=0;i<24;i++){
   float a=i*6.2831853f/24,b=(i+1)*6.2831853f/24;
   line(add(end,add(times(p.right,cosf(a)*.004f),times(p.up,sinf(a)*.004f))),
        add(end,add(times(p.right,cosf(b)*.004f),times(p.up,sinf(b)*.004f))));
  }
  {pointer_style::Lines width;glBufferData(GL_ARRAY_BUFFER,rays.size()*sizeof(Vertex),rays.data(),GL_STREAM_DRAW);glDrawArrays(GL_LINES,0,rays.size());}
 }
 };
 drawRays(false);
 auto pos=[&](float x,float y){return add(p.center,add(times(p.right,x),times(p.up,y)));};
 multiplayerRect(p.center,p.right,p.up,.85f,.31f,.04f,.045f,.05f,1.f);
 auto keys=chatKeys(chat.shift);
 for(unsigned i=0;i<keys.size();i++){
  auto&k=keys[i];float w=k.w*.06f-.003f,l=-.42f+k.x*.06f,top=.15f-k.y*.06f;
  bool highlight=int(i)==chatHover||int(i)==chatLeftHover||(k.code==256&&chat.shift);
  multiplayerRect(pos(l+k.w*.03f,top-.03f),p.right,p.up,w,.057f,highlight?.27f:.12f,highlight?.32f:.13f,highlight?.38f:.15f,1);
  float pixel=.00115f,textWidth=0;for(unsigned char c:k.label)textWidth+=infoAdvance[c]*pixel;
  float shown=std::min(w-.006f,textWidth);
  multiplayerText(k.label.c_str(),pos(l+(k.w*.06f-shown)/2,top-.012f),p.right,p.up,pixel,w-.006f);
 }
 std::string draft=chat.draft;
 auto lines=chatLines(draft,.00125f,.82f);if(lines.empty())lines.push_back("");
 for(unsigned i=0;i<lines.size();i++)chatOutlined(lines[i].c_str(),pos(-.41f,.18f+(lines.size()-i)*.035f),p.right,p.up,.00125f,.82f);
 if(fmod(seconds(),1.)<.5){
  float x=0;unsigned row=0;for(size_t i=0;i<chat.cursor;i++){float advance=infoAdvance[(unsigned char)draft[i]]*.00125f;if(x+advance>.82f&&x>0){row++;x=0;}x+=advance;}
  if(chat.cursor<draft.size()&&x+infoAdvance[(unsigned char)draft[chat.cursor]]*.00125f>.82f){row++;x=0;}
  unsigned count=std::max(1u,(unsigned)lines.size());
  chatOutlined("|",pos(-.41f+x,.18f+(count-row)*.035f),p.right,p.up,.00125f,.02f);
 }
 chatOutlined(chat.mode?"Whisper (team)":"Yell (everyone)",pos(-.41f,.22f+lines.size()*.035f),p.right,p.up,.001f,.82f);

 drawRays(true);
 glDepthMask(GL_TRUE);glEnable(GL_DEPTH_TEST);glDisable(GL_BLEND);glUniform1f(glGetUniformLocation(program,"alphaCutoff"),.5f);glUniform4f(glGetUniformLocation(program,"tint"),1,1,1,1);
}
