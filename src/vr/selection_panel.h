#include "ui_texture_edges.h"
// Text is supplied by the original selection/definition/string tables (schema 13).
// Cache local glyph geometry across both eyes and frames; no window readbacks.
static constexpr float infoPanelHeight=.040f; // Original status-bar parts are 64 pixels high.
static constexpr float infoTextScale=.78f;
static constexpr float infoPanelCenter=.017f; // (-.150 + .184) / 2; center the complete panel, including buttons.
static GLuint infoFont=0;
static std::array<unsigned short,256> infoAdvance{};
static std::map<unsigned,GLuint> infoDecoration;
static GLuint formationIcon(unsigned index,bool highlighted){auto i=infoDecoration.find(100+index*2+(highlighted?1:0));return i==infoDecoration.end()?0:i->second;}
static std::string infoCachedText;
static std::vector<Vertex> infoGlyphs;
static void loadInfoFont(android_app*a){
 infoFont=0;infoButtonFlash[0]=infoButtonFlash[1]=-1;infoAdvance.fill(14);infoDecoration.clear();infoCachedText.clear();infoGlyphs.clear();
 auto data=sharedAsset(a,"info-font.rgba");
 size_t size=data.size();const auto*b=data.data();
 if(b&&size==8+512*512*4+512&&u32(b)==512&&u32(b+4)==512){
  for(unsigned i=0;i<256;i++)infoAdvance[i]=std::clamp<unsigned>(u16(b+8+512*512*4+i*2),1,32);
  glGenTextures(1,&infoFont);glBindTexture(GL_TEXTURE_2D,infoFont);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,512,512,0,GL_RGBA,GL_UNSIGNED_BYTE,b+8);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
 }
 data=sharedAsset(a,"info-decoration.pack");
 b=data.data();size=data.size();size_t pos=4;
 if(b&&size>=4&&(u32(b)==8||u32(b)==28||u32(b)==30||u32(b)==32))for(unsigned i=0;i<u32(b);i++){
  if(pos+12>size)break;unsigned id=u32(b+pos),w=u32(b+pos+4),h=u32(b+pos+8);pos+=12;
  if(!w||!h||w>2048||h>128||pos+size_t(w)*h*4>size)break;
  auto pixels=uiTexturePixels(b+pos,w,h);GLuint tex;glGenTextures(1,&tex);glBindTexture(GL_TEXTURE_2D,tex);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());pos+=size_t(w)*h*4;
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,(id>=35&&id<=37)?GL_REPEAT:GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);infoDecoration[id]=tex;
 }
}
static float infoPanelBottom(const WristMapPlane&m){return .5f*(fabsf(m.forward.x)*m.width+fabsf(m.forward.z)*m.height)+.012f*cameraSpeeds.mapSize;}
static bool hitSelectionPanel(Vec3 origin,Vec3 direction,Vec3&hit){
 if(!infoPanelShown||!wristMapShown||!wristMapPoseValid||!wristMapTexture||radial.kind)return false;
 auto m=wristMapPlane(true);Vec3 n{m.right.y*m.up.z-m.right.z*m.up.y,m.right.z*m.up.x-m.right.x*m.up.z,m.right.x*m.up.y-m.right.y*m.up.x};
 float den=dot(direction,n);if(fabsf(den)<.00001f)return false;float t=dot(sub(m.center,origin),n)/den;if(t<=0||!std::isfinite(t))return false;
 auto p=add(origin,times(direction,t)),delta=sub(p,m.center);float x=dot(delta,m.right)+infoPanelCenter*cameraSpeeds.mapSize,y=dot(delta,m.up),bottom=infoPanelBottom(m),s=cameraSpeeds.mapSize;
 infoPanelButton=0;
 if(x>=.100f*s&&x<=.164f*s&&y>=bottom&&y<=bottom+infoPanelHeight*s){infoPanelButton=x<.132f*s?1:2;hit=add(camera,turn(p,cameraYaw));return true;}
 if(x<-.150f*s||x>.184f*s||y<bottom||y>bottom+infoPanelHeight*s)return false;
 hit=add(camera,turn(p,cameraYaw));return true;
}
static void drawSelectionPanel(){
 if(!infoPanelShown||!infoFont)return;
 auto m=wristMapPlane();float s=cameraSpeeds.mapSize,bottom=infoPanelBottom(m);
 std::string title=(const char*)selectionInfo.data()+4,names=(const char*)selectionInfo.data()+260,flavor=(const char*)selectionInfo.data()+772;
 if(!u32(selectionInfo.data())&&title.empty())title=levelName;
 std::string text=title+"\n"+names+"\n"+flavor;
 unsigned stock=u32(selectionInfo.data()+1812);
 std::string cacheKey=text+(stock?"\1":"\2");
 if(cacheKey!=infoCachedText){
  infoCachedText=cacheKey;infoGlyphs.clear();int row=0;float pen=0;constexpr float pixel=.00034f*infoTextScale;const int rows=5;
  auto lineWidth=[&](int line){return stock&&(.036f-line*.0085f*infoTextScale)>.024f?.140f:.174f;};
  for(size_t i=0;i<text.size()&&row<rows;i++){
   unsigned char c=text[i];if(c=='\n'||c=='\r'){if(c=='\n'||i+1==text.size()||text[i+1]!='\n'){row++;pen=0;}continue;}
   if(c==' '&&pen==0)continue;float maxWidth=lineWidth(row);
   if(c!=' '&&(i==0||text[i-1]==' '||text[i-1]=='\n')){float word=0;for(size_t j=i;j<text.size()&&text[j]!=' '&&text[j]!='\n'&&text[j]!='\r';j++)word+=infoAdvance[(unsigned char)text[j]]*pixel;if(pen&&word<maxWidth&&pen+word>maxWidth){row++;pen=0;}}
   if(pen+infoAdvance[c]*pixel>maxWidth){row++;pen=0;}if(row>=rows)break;
   float x=-.087f+pen-4*pixel,y=.036f-row*.0085f*infoTextScale,u=(c%16)/16.f,v=(c/16)/16.f;
   Vertex a{x,y,0,u,v},b{x+32*pixel,y,0,u+1.f/16,v},cc{x,y-32*pixel,0,u,v+1.f/16},d{x+32*pixel,y-32*pixel,0,u+1.f/16,v+1.f/16};
   infoGlyphs.insert(infoGlyphs.end(),{a,b,cc,b,d,cc});pen+=infoAdvance[c]*pixel;
  }
 }

 auto point=[&](float x,float y,float u=0,float v=0){auto p=add(m.center,add(times(m.right,(x-infoPanelCenter)*s),times(m.up,bottom+y*s)));return Vertex{p.x,p.y,p.z,u,v};};
 auto draw=[&](const std::vector<Vertex>&vertices,GLuint texture,float r,float g,float b,float alpha){glBindTexture(GL_TEXTURE_2D,texture);glUniform4f(glGetUniformLocation(program,"tint"),r,g,b,alpha);glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(Vertex),vertices.data(),GL_STREAM_DRAW);glDrawArrays(GL_TRIANGLES,0,vertices.size());};
 auto a=point(-.150f,0),b=point(.184f,0),c=point(-.150f,infoPanelHeight),d=point(.184f,infoPanelHeight);draw({a,b,c,b,d,c},whiteTexture,.035f,.045f,.055f,.96f);
 auto rect=[&](GLuint tex,float l,float bot,float r,float top,float umax=1,float vmax=1){auto a=point(l,top,0,0),b=point(r,top,umax,0),c=point(l,bot,0,vmax),d=point(r,bot,umax,vmax);draw({a,b,c,b,d,c},tex,1,1,1,1);};
 if(infoDecoration.count(35)){
  // Pixel scale is fixed by the 64px-high end caps. Only the center spans width.
  constexpr float px=infoPanelHeight/64.f;
  rect(infoDecoration[35],-.150f,0,.184f,infoPanelHeight,.334f/(640*px),1.f);
  rect(infoDecoration[36],-.150f,0,.184f,5*px,.334f/(1024*px));rect(infoDecoration[37],-.150f,59*px,.184f,infoPanelHeight,.334f/(1024*px));
  rect(infoDecoration[38],-.150f,0,-.150f+32*px,32*px);rect(infoDecoration[39],.184f-32*px,0,.184f,32*px);
  rect(infoDecoration[40],-.150f,32*px,-.150f+32*px,infoPanelHeight);rect(infoDecoration[41],.184f-32*px,32*px,.184f,infoPanelHeight);
 }
 unsigned uid=u32(selectionInfo.data()+1796),col=mappedCollection(u32(selectionInfo.data()+1800)),seq=u32(selectionInfo.data()+1804);
 auto portrait=quads.find(key(col,seq,0,0));
 if(uid&&portrait!=quads.end()){
  auto&q=portrait->second;GLuint tx=textures[q.texture];for(const auto&o:objects)if(o.unitID==uid){Object icon=o;icon.col=col;tx=coloredTexture(icon,q.texture);break;}
  // Circular portrait mask; its UV remains the original sprite image.
  float cx=-.1173f,cy=.020f,radius=.0155f;std::vector<Vertex> circle;
  auto uv=[&](float x,float y){float u=.5f+x*.5f,v=.5f-y*.5f;if(q.flags&1)u=1-u;if(q.flags&2)v=1-v;return point(cx+x*radius,cy+y*radius,u,v);};
  for(int i=0;i<64;i++){float a=i*6.2831853f/64,b=(i+1)*6.2831853f/64;circle.insert(circle.end(),{uv(0,0),uv(cosf(a),sinf(a)),uv(cosf(b),sinf(b))});}draw(circle,tx,1,1,1,1);
 }
 if(infoDecoration.count(34))rect(infoDecoration[34],-.142f,0,-.0926f,infoPanelHeight);
 if(u32(selectionInfo.data())==1&&stock){
  unsigned itemCol=mappedCollection(u32(selectionInfo.data()+1816)),palette=u32(selectionInfo.data()+1820),sequence=u32(selectionInfo.data()+1824);
  auto found=quads.find(key(itemCol,sequence,0,0));if(found!=quads.end()){
   auto&q=found->second;Object icon{};icon.col=itemCol;icon.palette=palette;GLuint texture=coloredTexture(icon,q.texture);
   float w=.011f,h=w*q.h/std::max(1.f,q.w);if(h>.011f){w*=.011f/h;h=.011f;}
   float l=.058f,t=.035f,u0=q.flags&1?1:0,v0=q.flags&2?1:0;
   auto a=point(l,t,u0,v0),b=point(l+w,t,1-u0,v0),c=point(l,t-h,u0,1-v0),d=point(l+w,t-h,1-u0,1-v0);draw({a,b,c,b,d,c},texture,1,1,1,1);
   char number[8];snprintf(number,sizeof(number),"%u",stock);float x=.072f-4*.00034f*infoTextScale;std::vector<Vertex> digits;
   for(const char*p=number;*p;p++){unsigned char c=*p;float u=(c%16)/16.f,v=(c/16)/16.f,pixel=.00034f*infoTextScale;auto a=point(x,.034f,u,v),b=point(x+32*pixel,.034f,u+1.f/16,v),cc=point(x,.034f-32*pixel,u,v+1.f/16),d=point(x+32*pixel,.034f-32*pixel,u+1.f/16,v+1.f/16);digits.insert(digits.end(),{a,b,cc,b,d,cc});x+=infoAdvance[c]*pixel;}draw(digits,infoFont,1,1,1,1);
  }
 }

 std::vector<Vertex> vertices;vertices.reserve(infoGlyphs.size());for(auto&v:infoGlyphs)vertices.push_back(point(v.x,v.y,v.u,v.v));draw(vertices,infoFont,.95f,.92f,.78f,1);
 for(int button=1;button<=2;button++){
  unsigned id=button==1?13:1,lit=button==1?15:3;float x=.100f+(button-1)*.032f;
  if(infoDecoration.count(id))rect(infoDecoration[id],x,.004f,x+.032f,.036f);
  // Original !sba sequence 4/3, frame 0, view 1 supplies the lit artwork.
  // VR feedback timing is presentation-only, independent of simulation/pause.
  double age=seconds()-infoButtonFlash[button-1];
  if(infoButtonFlash[button-1]>=0&&age>=0&&age<.266667&&infoDecoration.count(lit)){
   float alpha=1.f-float(age/.266667);alpha=alpha*alpha*(3-2*alpha);
   auto a=point(x,.036f,0,0),b=point(x+.032f,.036f,1,0),c=point(x,.004f,0,1),d=point(x+.032f,.004f,1,1);
   draw({a,b,c,b,d,c},infoDecoration[lit],1,1,1,alpha);
  }
 }
 if(infoPanelHovered&&pointerValid){std::vector<Vertex> line{{pointerStart.x,pointerStart.y,pointerStart.z,0,0},{pointerEnd.x,pointerEnd.y,pointerEnd.z,0,0}};glBindTexture(GL_TEXTURE_2D,whiteTexture);glUniform4f(glGetUniformLocation(program,"tint"),.95f,.92f,.78f,1);for(int i=0;i<24;i++){float a=i*6.2831853f/24,b=(i+1)*6.2831853f/24;for(float angle:{a,b}){auto p=add(pointerEnd,add(times(m.right,cosf(angle)*.002f*s),times(m.up,sinf(angle)*.002f*s)));line.push_back({p.x,p.y,p.z,0,0});}}glBufferData(GL_ARRAY_BUFFER,line.size()*sizeof(Vertex),line.data(),GL_STREAM_DRAW);glDrawArrays(GL_LINES,0,line.size());}

}
