#include "../src/vr/main.cpp"
#include <cassert>
#include <sys/stat.h>
int main(){
 // Schema 12: bounded fields, truncation, missing terminators, old frame support.
 FILE*input=fopen("/data/local/tmp/npc-schema11.bin","rb");assert(input);fseek(input,0,SEEK_END);size_t length=ftell(input);rewind(input);std::vector<unsigned char> packet(length);assert(fread(packet.data(),1,length,input)==length);fclose(input);
 assert(snapshot(packet.data(),packet.size()));packet.resize(length+1812);packet[8]=12;unsigned count=1;memcpy(packet.data()+length,&count,4);strcpy((char*)packet.data()+length+4,"Warrior");assert(snapshot(packet.data(),packet.size()));assert(!snapshot(packet.data(),packet.size()-1));packet[length+259]=1;assert(!snapshot(packet.data(),packet.size()));packet[length+259]=0;

 packet.resize(length+1828);packet[8]=13;packet[length+1812]=5;packet[length+1816]=7;packet[length+1820]=255;packet[length+1824]=14;assert(snapshot(packet.data(),packet.size()));assert(selectionInfo[1812]==5);packet[length+1812]=128;assert(!snapshot(packet.data(),packet.size()));packet[length+1812]=5;assert(!snapshot(packet.data(),packet.size()-1));
 packet.resize(length+1868);packet[8]=14;packet[length+1828]=8;packet[length+1828+9*4]=126;assert(snapshot(packet.data(),packet.size()));assert(unitGroups[0]==8&&unitGroups[9]==126);assert(!snapshot(packet.data(),packet.size()-1));packet[length+1828]=127;assert(!snapshot(packet.data(),packet.size()));packet[length+1828]=8;
 assert(formationMenuKind(false,false,true)==2);assert(formationMenuKind(false,false,false)==4);assert(formationMenuKind(false,true,false)==3);assert(formationMenuKind(false,true,true)==3);assert(formationMenuKind(true,true,true)==4);assert(formationMenuKind(true,false,false)==4);
 RadialMenu menu;menu.open(4,{},{1,0,0},{0,1,0},{0,0,1});menu.update({0,.1f,0});assert(menu.release(4)==0&&!menu.kind);menu.open(3,{},{1,0,0},{0,1,0},{0,0,1});assert(menu.release(3)==-1&&!menu.kind);
 for(Vec3 forward:std::vector<Vec3>{{0,0,-1},{1,0,0},{0,0,1},{-1,0,0}}){
  auto ahead=wristMapHeadingPoint(-forward.x,forward.z,forward);assert(fabsf(ahead.x)<.001f&&fabsf(ahead.y-1)<.001f);
  Vec3 right{-forward.z,0,forward.x};auto side=wristMapHeadingPoint(-right.x,right.z,forward);assert(fabsf(side.x-1)<.001f&&fabsf(side.y)<.001f);
 }

 const char*root="/data/local/tmp/myth-wrist-test";mkdir(root,0700);
 auto settings=[&](const char*s){FILE*f=fopen("/data/local/tmp/myth-wrist-test/camera.speeds","wb");assert(f);fputs(s,f);fclose(f);loadCameraSpeeds(root);};
 settings("100 200 300 50");assert(cameraSpeeds.move==1&&cameraSpeeds.stick==2&&cameraSpeeds.drag==3&&cameraSpeeds.mapSize==.5f);
 settings("999 -50 125 250");assert(cameraSpeeds.move==3&&cameraSpeeds.stick==1&&cameraSpeeds.drag==1.25f&&cameraSpeeds.mapSize==2);
 settings("bad");assert(cameraSpeeds.move==1&&cameraSpeeds.stick==1&&cameraSpeeds.drag==1&&cameraSpeeds.mapSize==1);
 unlink("/data/local/tmp/myth-wrist-test/camera.speeds");rmdir(root);
 cameraSpeeds.mapPitch=0; // Pixel assertions below use an explicitly flat test map.
 EGLDisplay display=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(display,0,0));EGLint aa[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_DEPTH_SIZE,24,EGL_NONE};EGLConfig cfg;EGLint n;assert(eglChooseConfig(display,aa,&cfg,1,&n)&&n);
 EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE},pa[]={EGL_WIDTH,512,EGL_HEIGHT,512,EGL_NONE};auto context=eglCreateContext(display,cfg,EGL_NO_CONTEXT,ca);auto surface=eglCreatePbufferSurface(display,cfg,pa);assert(eglMakeCurrent(display,surface,surface,context));glViewport(0,0,512,512);
 program=glCreateProgram();glAttachShader(program,shader(GL_VERTEX_SHADER,mythVertexShader));glAttachShader(program,shader(GL_FRAGMENT_SHADER,mythFragmentShader));glLinkProgram(program);GLint linked;glGetProgramiv(program,GL_LINK_STATUS,&linked);assert(linked);glUseProgram(program);
 glGenVertexArrays(1,&vao);glBindVertexArray(vao);glGenBuffers(1,&vbo);glGenTextures(1,&whiteTexture);glBindTexture(GL_TEXTURE_2D,whiteTexture);unsigned char p[]={255,255,255,255};glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,p);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);wristMapTexture=whiteTexture;
 Mat m{5,0,0,0,0,5,0,0,0,0,1,0,0,0,0,1};glUniformMatrix4fv(glGetUniformLocation(program,"mvp"),1,GL_FALSE,m.data());camera={0,0,0};cameraYaw=0;wristMapAspect=1;wristMapShown=wristMapPoseValid=true;wristMapPose.orientation={sqrtf(.5f),0,0,sqrtf(.5f)};wristMapPose.position={0,.1f,-.08f};
 // Draw the group menu on the actual Quest GLES driver.
 Mat menuMatrix{3,0,0,0,0,3,0,0,0,0,1,0,0,0,0,1};glUniformMatrix4fv(glGetUniformLocation(program,"mvp"),1,GL_FALSE,menuMatrix.data());
 unitGroups={8,0,3,0,0,0,0,0,0,126};radial.open(3,{},{1,0,0},{0,1,0},{0,0,1});radial.update({.1f,.1f,0});glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);drawRadial();assert(glGetError()==GL_NO_ERROR);
 std::vector<unsigned char> menuPixels(512*512*4);glReadPixels(0,0,512,512,GL_RGBA,GL_UNSIGNED_BYTE,menuPixels.data());FILE*menuFile=fopen("/data/local/tmp/group-menu.rgba","wb");assert(menuFile);fwrite(menuPixels.data(),1,menuPixels.size(),menuFile);fclose(menuFile);radial.close();glUniformMatrix4fv(glGetUniformLocation(program,"mvp"),1,GL_FALSE,m.data());
 XrPosef upright{};upright.orientation.w=1;
 assert(wristMapReadableRight(upright,{0,0,0},{0,1,0}).x==1);
 assert(wristMapReadableRight(upright,{0,0,0},{0,-1,0}).x==-1);
 wristMapViewer={0,0,1};
 XrPosef eye{};eye.orientation.w=1;eye.position={-1000,0,1000};
 auto render=[&](){glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);drawWristMap(eye);assert(glGetError()==GL_NO_ERROR);std::vector<unsigned char>b(512*512*4);glReadPixels(0,0,512,512,GL_RGBA,GL_UNSIGNED_BYTE,b.data());unsigned count=0;for(size_t i=0;i<b.size();i+=4)if(b[i]>100)count++;return count;};
 cameraSpeeds.mapSize=.5f;auto small=render();cameraSpeeds.mapSize=1;auto normal=render();cameraSpeeds.mapSize=2;auto large=render();assert(small>100&&normal>small*3&&large>normal*3);
 wristMapShown=false;assert(render()==0);wristMapShown=true;wristMapPoseValid=false;assert(render()==0);wristMapPoseValid=true;radial.kind=2;assert(render()==0);radial.kind=0;
 Object o{};o.unitID=1;o.health=1;o.friendly=true;o.controllable=true;o.team=0;o.x=levelWidth*.25f;o.y=levelHeight*.25f;objects={o};
 auto dotPixel=[&](){render();std::array<unsigned char,4> c;glReadPixels(352,352,1,1,GL_RGBA,GL_UNSIGNED_BYTE,c.data());return c;};
 auto green=dotPixel();assert(green[1]>240&&green[0]<10);
 objects[0].controllable=false;auto npc=dotPixel();assert(npc[2]>240&&npc[0]<10);
 objects[0].friendly=false;auto red=dotPixel();assert(red[0]>240&&red[1]<10);
 objects[0].team=-1;auto blue=dotPixel();assert(blue[2]>240&&blue[0]<10);objects[0].team=1;
 objects[0].selected=true;auto yellow=dotPixel();assert(yellow[0]>240&&yellow[1]>240&&yellow[2]<10);
 objects[0].rawFlags=0x200;auto hidden=dotPixel();assert(hidden[0]>240&&hidden[1]>240&&hidden[2]>240);
 objects[0].rawFlags=0;objects[0].health=0;assert(dotPixel()==hidden);
 objects.clear();wristMapViewer={(originX-levelWidth*.5f)*scale,0,(levelHeight*.5f-originY)*scale};render();
 std::array<unsigned char,4> centerPixel;glReadPixels(256,256,1,1,GL_RGBA,GL_UNSIGNED_BYTE,centerPixel.data());assert(centerPixel[2]>240);
 std::vector<unsigned char> image(512*512*4);glReadPixels(0,0,512,512,GL_RGBA,GL_UNSIGNED_BYTE,image.data());unsigned yellowCount=0;
 for(size_t i=0;i<image.size();i+=4)if(image[i]>240&&image[i+1]>240&&image[i+2]<10)yellowCount++;assert(yellowCount>0);
 // Asymmetric original-style art: left red, right blue. Mesh x=.25 must sample
 // the right half of the source image, while marker coordinates stay unchanged.
 objects.clear();GLuint art;glGenTextures(1,&art);glBindTexture(GL_TEXTURE_2D,art);unsigned char artPixels[]={255,0,0,255,0,0,255,255};
 glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,2,1,0,GL_RGBA,GL_UNSIGNED_BYTE,artPixels);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
 wristMapTexture=art;wristMapViewer={0,0,1};render();std::array<unsigned char,4> left,rightPixel;
 glReadPixels(160,352,1,1,GL_RGBA,GL_UNSIGNED_BYTE,left.data());glReadPixels(352,352,1,1,GL_RGBA,GL_UNSIGNED_BYTE,rightPixel.data());assert(left[0]>240&&left[2]<10&&rightPixel[2]>240&&rightPixel[0]<10);
 wristMapTexture=whiteTexture;glDeleteTextures(1,&art);
 puts("PASS asymmetric overhead artwork U flip aligned with original map positions");
 puts("PASS two-sided readable map basis and hollow yellow camera marker");
 puts("PASS camera settings defaults/bounds; wrist map 0.5/1/2 scale, hidden/tracking/menu gates, live markers, GL_NO_ERROR");

 // Information panel is above the complete rotated map; no warp overlap.
 infoPanelShown=true;wristMapShown=wristMapPoseValid=true;radial.kind=0;
 unsigned cases=0;
 for(float scaleValue:{.5f,1.f,2.f})for(float pitch:{-.785398f,0.f,.785398f})for(int yaw=0;yaw<12;yaw++){
  cameraSpeeds.mapSize=scaleValue;cameraSpeeds.mapPitch=pitch;wristMapForward={sinf(yaw*.523598f),0,cosf(yaw*.523598f)};
  auto plane=wristMapPlane(true);auto target=add(plane.center,times(plane.up,infoPanelBottom(plane)+infoPanelHeight*.5f*scaleValue));
  Vec3 normal{plane.right.y*plane.up.z-plane.right.z*plane.up.y,plane.right.z*plane.up.x-plane.right.x*plane.up.z,plane.right.x*plane.up.y-plane.right.y*plane.up.x};
  Vec3 start=add(target,times(normal,.3f)),direction=times(normal,-1),hit;float u,v;
  assert(hitSelectionPanel(start,direction,hit));assert(!hitWristMap(start,direction,hit,u,v,true));cases++;
 }
 printf("PASS schema12 and %u information panel poses (no minimap overlap)\n",cases);

 cameraSpeeds.mapSize=1;cameraSpeeds.mapPitch=0;camera={0,0,0};cameraYaw=0;wristMapForward={0,0,-1};wristMapPose.orientation={sqrtf(.5f),0,0,sqrtf(.5f)};wristMapPose.position={0,0,-.08f};wristMapViewer={0,0,1};
 FILE*font=fopen("/data/local/tmp/info-font.rgba","rb");assert(font);std::vector<unsigned char> rgba(8+512*512*4+512);assert(fread(rgba.data(),1,rgba.size(),font)==rgba.size());fclose(font);for(unsigned i=0;i<256;i++)infoAdvance[i]=u16(rgba.data()+8+512*512*4+i*2);glGenTextures(1,&infoFont);glBindTexture(GL_TEXTURE_2D,infoFont);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,512,512,0,GL_RGBA,GL_UNSIGNED_BYTE,rgba.data()+8);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
 selectionInfo.fill(0);selectionInfo[0]=1;strcpy((char*)selectionInfo.data()+4,"Warrior");strcpy((char*)selectionInfo.data()+260,"Ghatali the Unworthy");strcpy((char*)selectionInfo.data()+772,"The warriors of the Legion have returned to Willow Creek. This panel follows the wrist while the map turns below it.");

 FILE*deco=fopen("/data/local/tmp/info-decoration.pack","rb");assert(deco);fseek(deco,0,SEEK_END);std::vector<unsigned char> pack(ftell(deco));rewind(deco);assert(fread(pack.data(),1,pack.size(),deco)==pack.size());fclose(deco);size_t at=4;
 for(unsigned i=0;i<u32(pack.data());i++){unsigned id=u32(pack.data()+at),w=u32(pack.data()+at+4),h=u32(pack.data()+at+8);at+=12;GLuint t;glGenTextures(1,&t);glBindTexture(GL_TEXTURE_2D,t);auto clean=uiTexturePixels(pack.data()+at,w,h);
 for(size_t k=0;k<clean.size();k+=4){assert(clean[k+3]==pack[at+k+3]);if(clean[k+3])assert(!memcmp(clean.data()+k,pack.data()+at+k,4));else if(id==39||id==41)assert(clean[k+2]<=clean[k]+32);}
 glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,clean.data());at+=w*h*4;glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,(id>=35&&id<=37)?GL_REPEAT:GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);infoDecoration[id]=t;}

 assert(infoDecoration.size()==28);for(unsigned i=100;i<120;i++)assert(infoDecoration.count(i));
 glUniformMatrix4fv(glGetUniformLocation(program,"mvp"),1,GL_FALSE,menuMatrix.data());camera={};cameraYaw=0;radial.open(2,{},{1,0,0},{0,1,0},{0,0,1});radial.update({0,.1f,0});glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);drawRadial();assert(glGetError()==GL_NO_ERROR);
 glReadPixels(0,0,512,512,GL_RGBA,GL_UNSIGNED_BYTE,menuPixels.data());menuFile=fopen("/data/local/tmp/formation-menu.rgba","wb");assert(menuFile);fwrite(menuPixels.data(),1,menuPixels.size(),menuFile);fclose(menuFile);radial.close();glUniformMatrix4fv(glGetUniformLocation(program,"mvp"),1,GL_FALSE,m.data());
 // All 299 real original name/flavor pairings must survive the actual GPU-layout path.
 FILE*fixtures=fopen("/data/local/tmp/info-fit-fixtures.bin","rb");assert(fixtures);unsigned total;assert(fread(&total,4,1,fixtures)==1);
 for(unsigned test=0;test<total;test++){
  int stock=fgetc(fixtures);assert(stock>=0);selectionInfo.fill(0);selectionInfo[0]=1;selectionInfo[1812]=stock;std::string expected;
  for(unsigned offset:{4u,260u,772u}){size_t n=0;int c;while((c=fgetc(fixtures))>0){assert(n<(offset==4?255:offset==260?511:1023));selectionInfo[offset+n++]=c;if(c!=' '&&c!='\r'&&c!='\n')expected+=(char)c;}assert(c==0);}
  drawSelectionPanel();std::string actual;
  for(size_t i=0;i<infoGlyphs.size();i+=6){auto&v=infoGlyphs[i];unsigned code=(unsigned)lroundf(v.u*16)+(unsigned)lroundf(v.v*16)*16;if(code!=' ')actual+=(char)code;assert(v.x>=-.09f&&v.x<.14f);assert(v.y<=.0361f&&v.y>=.009f);}
  if(actual!=expected){fprintf(stderr,"Text fit failure case %u: %zu/%zu chars\n",test,actual.size(),expected.size());abort();}
 }
 fclose(fixtures);printf("PASS all %u original name/flavor pairs unabridged at 78 percent / five rows\n",total);
 selectionInfo.fill(0);selectionInfo[0]=1;strcpy((char*)selectionInfo.data()+4,"Warrior");strcpy((char*)selectionInfo.data()+260,"Ghatali the Unworthy");strcpy((char*)selectionInfo.data()+772,"The warriors of the Legion have returned to Willow Creek. This panel follows the wrist while the map turns below it.");
 infoPanelShown=true;render();assert(glGetError()==GL_NO_ERROR);image.resize(512*512*4);glReadPixels(0,0,512,512,GL_RGBA,GL_UNSIGNED_BYTE,image.data());FILE*out=fopen("/data/local/tmp/selection-panel.rgba","wb");assert(out);fwrite(image.data(),1,image.size(),out);fclose(out);
 puts("PASS decoration alpha/visible pixels unchanged; transparent blue removed; panel GL_NO_ERROR");
}
