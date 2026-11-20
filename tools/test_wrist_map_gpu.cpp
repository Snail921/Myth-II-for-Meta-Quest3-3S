#include "../src/vr/main.cpp"
#include <cassert>
#include <sys/stat.h>
int main(){
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
 EGLDisplay display=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(display,0,0));EGLint aa[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_DEPTH_SIZE,24,EGL_NONE};EGLConfig cfg;EGLint n;assert(eglChooseConfig(display,aa,&cfg,1,&n)&&n);
 EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE},pa[]={EGL_WIDTH,512,EGL_HEIGHT,512,EGL_NONE};auto context=eglCreateContext(display,cfg,EGL_NO_CONTEXT,ca);auto surface=eglCreatePbufferSurface(display,cfg,pa);assert(eglMakeCurrent(display,surface,surface,context));glViewport(0,0,512,512);
 program=glCreateProgram();glAttachShader(program,shader(GL_VERTEX_SHADER,spriteShaderSource(mythVertexShader,false,false,false).c_str()));glAttachShader(program,shader(GL_FRAGMENT_SHADER,spriteShaderSource(mythFragmentShader,false,false,false).c_str()));glLinkProgram(program);GLint linked;glGetProgramiv(program,GL_LINK_STATUS,&linked);assert(linked);glUseProgram(program);glUniform1i(glGetUniformLocation(program,"tex"),0);glUniform1i(glGetUniformLocation(program,"detailTex"),1);
 glGenVertexArrays(1,&vao);glBindVertexArray(vao);glGenBuffers(1,&vbo);glGenTextures(1,&whiteTexture);glBindTexture(GL_TEXTURE_2D,whiteTexture);unsigned char p[]={255,255,255,255};glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,p);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);wristMapTexture=whiteTexture;
 Mat m{5,0,0,0,0,5,0,0,0,0,1,0,0,0,0,1};glUniformMatrix4fv(glGetUniformLocation(program,"mvp"),1,GL_FALSE,m.data());camera={0,0,0};cameraYaw=0;cameraSpeeds.mapPitch=0;wristMapAspect=1;wristMapShown=wristMapPoseValid=true;wristMapPose.orientation={sqrtf(.5f),0,0,sqrtf(.5f)};wristMapPose.position={0,.1f,-.08f};
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
 mapPen.data.fill(0);unsigned npoints=3;memcpy(mapPen.data.data()+4,&npoints,4);float range=128;
 memcpy(mapPen.data.data()+16,&range,4);memcpy(mapPen.data.data()+20,&range,4);
 mapPen.data[33]=255;mapPen.data[35]=255;unsigned char stroke[]={0,2,20,30,100,90};memcpy(mapPen.data.data()+160,stroke,6);
 mapPen.updated=seconds();mapPenEnabled=true;mapPenForward={0,0,-1};render();
 glReadPixels(0,0,512,512,GL_RGBA,GL_UNSIGNED_BYTE,image.data());unsigned greenLine=0,redBorder=0;
 for(size_t i=0;i<image.size();i+=4){if(image[i]<10&&image[i+1]>240&&image[i+2]<10)greenLine++;if(image[i]>240&&image[i+1]<50&&image[i+2]<50)redBorder++;}
 assert(greenLine>100&&redBorder>100&&glGetError()==GL_NO_ERROR);
 mapPenEnabled=false;mapPen.data.fill(0);
 puts("PASS team-colour annotation geometry and active red border on Quest GPU");

}
