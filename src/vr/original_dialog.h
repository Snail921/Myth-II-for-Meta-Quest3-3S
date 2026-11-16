// Original dialogs are rendered by the guest; private same-UID files carry pixels/input.
static std::string dialogRoot,dialogTracePath;
static GLuint dialogTexture=0;
static unsigned dialogSequence=0,dialogCommandSequence=0;
static int dialogWidth=0,dialogHeight=0;
static double dialogPollTime=0,dialogRequestedAt=0;
static void initOriginalDialogs(android_app*a){dialogTracePath=std::string(a->activity->externalDataPath)+"/dialog-vr-trace.log";dialogRoot=a->activity->internalDataPath;dialogCommandSequence=(unsigned)(seconds()*1000);originalDialogMode=0;dialogTexture=0;dialogSequence=0;dialogWidth=dialogHeight=0;dialogPollTime=0;}
static void originalDialogRequest(int op,int x,int y){
 __android_log_print(ANDROID_LOG_INFO,"MythDialogs","VR request op=%d root=%s",op,dialogRoot.c_str());if(dialogRoot.empty())return;
 if(op<=2){unlink((dialogRoot+"/dialog.rgba").c_str());originalDialogMode=op;dialogWidth=dialogHeight=0;dialogSequence=0;dialogRequestedAt=seconds();wristMapShown=infoPanelShown=true;}
 FILE*trace=fopen(dialogTracePath.c_str(),"a");if(trace){fprintf(trace,"%.3f request op=%d xy=%d,%d mode=%d size=%dx%d\n",seconds(),op,x,y,originalDialogMode,dialogWidth,dialogHeight);fclose(trace);}
 unsigned data[5]={0x31474c44,++dialogCommandSequence,(unsigned)op,(unsigned)x,(unsigned)y};
 std::string tmp=dialogRoot+"/dialog.command.tmp",path=dialogRoot+"/dialog.command";FILE*f=fopen(tmp.c_str(),"wb");if(!f){__android_log_print(ANDROID_LOG_ERROR,"MythDialogs","Command file open failed errno=%d",errno);return;}
 bool ok=fwrite(data,1,sizeof(data),f)==sizeof(data);fclose(f);if(ok)rename(tmp.c_str(),path.c_str());
}
static void pollOriginalDialog(){
 if(!originalDialogMode||seconds()-dialogPollTime<.12)return;dialogPollTime=seconds();
 FILE*f=fopen((dialogRoot+"/dialog.rgba").c_str(),"rb");if(!f){if(seconds()-dialogRequestedAt>5&&dialogWidth==0){LOG("Original dialog response timeout");originalDialogMode=0;}return;}
 unsigned h[5];if(fread(h,1,sizeof(h),f)!=sizeof(h)||h[0]!=0x31474c44||h[2]>2||h[3]>1024||h[4]>768){fclose(f);return;}
 if(h[1]==dialogSequence){fclose(f);return;}
 if(!h[2]){originalDialogMode=0;dialogSequence=h[1];fclose(f);input.swallowRight=true;return;}
 if(!h[3]||!h[4]){fclose(f);return;}std::vector<unsigned char> pixels(size_t(h[3])*h[4]*4);if(fread(pixels.data(),1,pixels.size(),f)!=pixels.size()){fclose(f);return;}fclose(f);
 dialogWidth=h[3];dialogHeight=h[4];dialogSequence=h[1];originalDialogMode=h[2];
 if(!dialogTexture)glGenTextures(1,&dialogTexture);glBindTexture(GL_TEXTURE_2D,dialogTexture);
 glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,dialogWidth,dialogHeight,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
}
static bool hitOriginalDialog(Vec3 origin,Vec3 direction,Vec3&hit,float&u,float&v){
 if(!originalDialogMode||!dialogWidth||!wristMapPoseValid)return false;auto m=wristMapPlane();
 Vec3 n{m.right.y*m.up.z-m.right.z*m.up.y,m.right.z*m.up.x-m.right.x*m.up.z,m.right.x*m.up.y-m.right.y*m.up.x};
 float den=dot(direction,n);if(fabsf(den)<.00001f)return false;float t=dot(sub(m.center,origin),n)/den;if(t<=0||!std::isfinite(t))return false;
 auto p=add(origin,times(direction,t)),delta=sub(p,m.center);float s=cameraSpeeds.mapSize,w=.30f*s,h=w*dialogHeight/dialogWidth,bottom=infoPanelBottom(m)+(.040f+.012f)*s;
 hit=p;u=.5f+dot(delta,m.right)/w;v=1-(dot(delta,m.up)-bottom)/h;return std::isfinite(u)&&std::isfinite(v)&&u>=0&&u<=1&&v>=0&&v<=1;
}
static void drawOriginalDialog(){
 if(!originalDialogMode||!dialogTexture||!dialogWidth)return;auto m=wristMapPlane();float s=cameraSpeeds.mapSize,w=.30f*s,h=w*dialogHeight/dialogWidth,bottom=infoPanelBottom(m)+(.040f+.012f)*s;
 auto point=[&](float x,float y,float u,float v){auto p=add(m.center,add(times(m.right,x),times(m.up,y)));return Vertex{p.x,p.y,p.z,u,v};};
 std::vector<Vertex> vertices{point(-w/2,bottom+h,0,0),point(w/2,bottom+h,1,0),point(-w/2,bottom,0,1),point(w/2,bottom+h,1,0),point(w/2,bottom,1,1),point(-w/2,bottom,0,1)};
 glBindTexture(GL_TEXTURE_2D,dialogTexture);glUniform4f(glGetUniformLocation(program,"tint"),1,1,1,1);glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(Vertex),vertices.data(),GL_STREAM_DRAW);glDrawArrays(GL_TRIANGLES,0,vertices.size());
 if(pointerValid){std::vector<Vertex> line{{pointerStart.x,pointerStart.y,pointerStart.z,0,0},{pointerEnd.x,pointerEnd.y,pointerEnd.z,0,0}};
  if(originalDialogHovered)for(int i=0;i<24;i++){float a=i*6.2831853f/24,b=(i+1)*6.2831853f/24;for(float angle:{a,b}){auto p=add(pointerEnd,add(times(m.right,cosf(angle)*.002f*s),times(m.up,sinf(angle)*.002f*s)));line.push_back({p.x,p.y,p.z,0,0});}}
  glBindTexture(GL_TEXTURE_2D,whiteTexture);glUniform4f(glGetUniformLocation(program,"tint"),1,originalDialogHovered?1.f:.8f,0,1);glBufferData(GL_ARRAY_BUFFER,line.size()*sizeof(Vertex),line.data(),GL_STREAM_DRAW);glDrawArrays(GL_LINES,0,line.size());}
}
