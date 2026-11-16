static GLuint formationIcon(unsigned index,bool highlighted);
// A small local bitmap alphabet keeps labels legible without a new font dependency.
static const unsigned char* menuGlyph(char c){
 static const unsigned char glyph[][7]={
 {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},{30,1,1,14,1,1,30},{2,6,10,18,31,2,2},
 {31,16,16,30,1,1,30},{14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},{14,17,17,15,1,1,14},
 {14,17,17,31,17,17,17},{14,17,16,16,16,17,14},{30,17,17,17,17,17,30},{31,16,16,30,16,16,31},
 {17,17,17,31,17,17,17},{16,16,16,16,16,16,31},{14,17,17,17,17,17,14},{30,17,17,30,16,16,16},
 {30,17,17,30,20,18,17},{15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{14,17,16,23,17,17,14},{17,25,21,19,17,17,17},{17,17,17,17,17,17,14},{31,16,16,30,16,16,16},{31,4,4,4,4,4,31},{17,27,21,21,17,17,17}};
 const char*alphabet="0123456789ACDEHLOPRSTGNUFIM";const char*p=strchr(alphabet,c);return p?glyph[p-alphabet]:nullptr;
}
static void drawRadial(){
 if(!radial.kind)return;
 glDisable(GL_DEPTH_TEST);glBindTexture(GL_TEXTURE_2D,whiteTexture);attributes(vbo);
 auto point=[&](float x,float y){auto p=add(camera,turn(radial.point(x,y),cameraYaw));return Vertex{p.x,p.y,p.z,0,0};};
 auto mesh=[&](const std::vector<Vertex>&v,float r,float g,float b){glUniform4f(glGetUniformLocation(program,"tint"),r,g,b,1);glBufferData(GL_ARRAY_BUFFER,v.size()*sizeof(Vertex),v.data(),GL_STREAM_DRAW);glDrawArrays(GL_TRIANGLES,0,v.size());};
 auto text=[&](const char*label,float cx,float cy,float px){std::vector<Vertex>v;float x=cx-strlen(label)*6*px*.5f,y=cy+3.5f*px;
  for(const char*c=label;*c;c++,x+=6*px){auto rows=menuGlyph((*c>='a'&&*c<='z')?*c-'a'+'A':*c);if(!rows)continue;for(int row=0;row<7;row++)for(int col=0;col<5;col++)if(rows[row]&(16>>col)){float a=x+col*px,b=y-row*px;auto p=point(a,b),q=point(a+px*.88f,b),r=point(a,b-px*.88f),s=point(a+px*.88f,b-px*.88f);v.insert(v.end(),{p,q,r,q,s,r});}}
  mesh(v,1,.94f,.83f);
 };
 bool group=radial.kind>=3;
 int count=radial.kind==1?commandMenuCount:10;float radius=radial.kind==1?.19f:.25f;
 for(int i=0;i<count;i++){std::vector<Vertex> v;float mid=i*6.2831853f/count,half=3.14159265f/count-.025f;
  for(int j=0;j<12;j++){float a=mid-half+2*half*j/12,b=mid-half+2*half*(j+1)/12;auto p=point(sinf(a)*.045f,cosf(a)*.045f),q=point(sinf(a)*radius,cosf(a)*radius),r=point(sinf(b)*.045f,cosf(b)*.045f),s=point(sinf(b)*radius,cosf(b)*radius);v.insert(v.end(),{p,q,r,q,s,r});}
  bool chosen=i==radial.item;if(group){bool occupied=unitGroups[i]!=0;mesh(v,chosen?.72f:occupied?.38f:.23f,chosen?.34f:occupied?.16f:.085f,chosen?.07f:occupied?.035f:.015f);}else mesh(v,chosen?.7f:.08f,chosen?.5f:.12f,chosen?.05f:.18f);
  static const char*names[]={"GUARD","RETREAT","SCATTER","STOP","TAUNT"};char number[4];snprintf(number,sizeof(number),"%d",group?(i+1)%10:i+1);const char*label=radial.kind==1?names[i]:number;
  if(radial.kind==2&&formationIcon(i,chosen)){
   float cx=sinf(mid)*radius*.70f,cy=cosf(mid)*radius*.70f,w=.064f,h=w*21.f/28.f;
   auto a=point(cx-w*.5f,cy+h*.5f),b=point(cx+w*.5f,cy+h*.5f),c=point(cx-w*.5f,cy-h*.5f),d=point(cx+w*.5f,cy-h*.5f);
   a.u=0;a.v=0;b.u=1;b.v=0;c.u=0;c.v=1;d.u=1;d.v=1;
   glBindTexture(GL_TEXTURE_2D,formationIcon(i,chosen));mesh({a,b,c,b,d,c},1,1,1);glBindTexture(GL_TEXTURE_2D,whiteTexture);
   text(number,cx,cy-h*.5f-.010f,.0016f);continue;
  }
  float px=.0028f,x=sinf(mid)*radius*.70f-strlen(label)*6*px*.5f,y=cosf(mid)*radius*.70f+3.5f*px;v.clear();
  for(const char*c=label;*c;c++,x+=6*px){auto rows=menuGlyph(*c);if(!rows)continue;for(int row=0;row<7;row++)for(int col=0;col<5;col++)if(rows[row]&(16>>col)){float a=x+col*px,b=y-row*px;auto p=point(a,b),q=point(a+px*.88f,b),r=point(a,b-px*.88f),s=point(a+px*.88f,b-px*.88f);v.insert(v.end(),{p,q,r,q,s,r});}}
  mesh(v,1,1,1);
  if(group&&unitGroups[i]){char units[24];snprintf(units,sizeof(units),"%u Units",unitGroups[i]);text(units,sinf(mid)*radius*.70f,cosf(mid)*radius*.70f-.020f,.0016f);}
 }
 if(radial.kind!=1)text(radial.kind==2?"Formation":radial.kind==3?"Add Group":"Group",0,0,.0015f);
 if(radial.item<0){glUniform4f(glGetUniformLocation(program,"tint"),1,1,1,1);glEnable(GL_DEPTH_TEST);return;}
 auto delta=sub(radial.hand,radial.anchor);float x=dot(delta,radial.right)/radial.scale,y=dot(delta,radial.up)/radial.scale,len=sqrtf(x*x+y*y);if(len>radius){x*=radius/len;y*=radius/len;}
 std::vector<Vertex> cursor;for(int i=0;i<16;i++){float a=i*6.2831853f/16,b=(i+1)*6.2831853f/16;cursor.insert(cursor.end(),{point(x,y),point(x+cosf(a)*.006f,y+sinf(a)*.006f),point(x+cosf(b)*.006f,y+sinf(b)*.006f)});}mesh(cursor,1,1,.2f);
 glUniform4f(glGetUniformLocation(program,"tint"),1,1,1,1);glEnable(GL_DEPTH_TEST);
}
