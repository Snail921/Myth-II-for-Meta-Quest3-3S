#include "../src/vr/main.cpp"
#include <cassert>
int main(){

 std::vector<unsigned char> frame(64+272,0);memcpy(frame.data(),"MYTHBR01",8);
 auto put=[&](size_t at,unsigned v){memcpy(frame.data()+at,&v,4);};put(8,20);put(12,1);put(16,1);put(32,216);put(36,80);
 auto append=[&](unsigned v){size_t n=frame.size();frame.resize(n+4);memcpy(frame.data()+n,&v,4);};
 append(0);append(0);append(0);append(0);append(0);append(4);append(0);append(0);append(0);
 frame.resize(frame.size()+1828+40+44);append(0);append(0);append(1);size_t record=frame.size();frame.resize(record+44);put(record,1);put(record+4,101);memcpy(frame.data()+record+12,"Teammate",9);
 assert(snapshot(frame.data(),frame.size()));assert(coopRecipients.size()==1&&coopRecipients[0].identity==101&&!strcmp(coopRecipients[0].name,"Teammate"));
 assert(!snapshot(frame.data(),frame.size()-1));frame.back()=1;assert(!snapshot(frame.data(),frame.size()));frame.back()=0;put(record,32);assert(!snapshot(frame.data(),frame.size()));put(record,1);assert(snapshot(frame.data(),frame.size()));
 puts("PASS schema20 recipient framing, truncation, invalid slot and name termination");
 camera={0,0,0};cameraYaw=0;
 CoopRecipient a{};a.slot=1;a.identity=101;a.color[0]=255;strcpy(a.name,"Player One");CoopRecipient b=a;b.slot=2;b.identity=102;b.color[0]=0;b.color[1]=255;strcpy(b.name,"Player Two");
 coopRecipients={a,b};Object o{};o.selected=true;o.controllable=true;o.unitID=12;objects={o};coopSelectionHash=123;
 XrPosef head{};head.orientation.w=1;openCoopTransfer(head);assert(coopMenuOpen&&coopChoices.size()==2&&coopFrozenSelection==123);
 for(int i=0;i<2;i++){Vec3 target={0,coopTop()-i*coopRowHeight()-.025f,-1.2f};pointCoopTransfer({0,0,0},normalized(target));assert(coopHover==i);}
 pointCoopTransfer({0,0,0},normalized(Vec3{0,coopTop()-2*coopRowHeight()-.025f,-1.2f}));assert(coopHover==-1);
 pointCoopTransfer({0,0,0},normalized(Vec3{.4f,0,-1.2f}));assert(coopHover==-1);
 pointCoopTransfer({0,0,0},{0,0,1});assert(coopHover==-1);
 head.position={2,1,3};head.orientation={0,sinf(.6f),0,cosf(.6f)};openCoopTransfer(head);auto expected=add(Vec3{2,1,3},times(rotated(head.orientation,{0,0,-1}),1.2f));assert(dot(sub(coopCenter,expected),sub(coopCenter,expected))<1e-8f);
 assert(coopCurrent(a));coopRecipients[0].identity++;assert(!coopCurrent(a));coopRecipients={a,b};
 EGLDisplay display=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(display,0,0));EGLint aa[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_DEPTH_SIZE,24,EGL_NONE};EGLConfig cfg;EGLint n;assert(eglChooseConfig(display,aa,&cfg,1,&n)&&n);
 EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE},pa[]={EGL_WIDTH,512,EGL_HEIGHT,512,EGL_NONE};auto context=eglCreateContext(display,cfg,EGL_NO_CONTEXT,ca);auto surface=eglCreatePbufferSurface(display,cfg,pa);assert(eglMakeCurrent(display,surface,surface,context));glViewport(0,0,512,512);
 program=glCreateProgram();glAttachShader(program,shader(GL_VERTEX_SHADER,mythVertexShader));glAttachShader(program,shader(GL_FRAGMENT_SHADER,mythFragmentShader));glLinkProgram(program);GLint linked;glGetProgramiv(program,GL_LINK_STATUS,&linked);assert(linked);glUseProgram(program);glUniform1i(glGetUniformLocation(program,"tex"),0);glUniform1i(glGetUniformLocation(program,"detailTex"),1);
 glGenVertexArrays(1,&vao);glBindVertexArray(vao);glGenBuffers(1,&vbo);glGenTextures(1,&whiteTexture);glBindTexture(GL_TEXTURE_2D,whiteTexture);unsigned char p[]={255,255,255,255};glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,p);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);wristMapTexture=whiteTexture;

 infoFont=whiteTexture;infoAdvance.fill(14);coopNormal={0,0,1};coopRight={1,0,0};coopUp={0,1,0};coopCenter={0,0,0};coopHover=-1;pointerValid=false;
 Mat matrix{2,0,0,0,0,2,0,0,0,0,1,0,0,0,0,1};glUniformMatrix4fv(glGetUniformLocation(program,"mvp"),1,GL_FALSE,matrix.data());
 glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);printf("before draw error=%x\n",glGetError());drawCoopTransfer();unsigned error=glGetError();printf("after draw error=%x\n",error);assert(error==GL_NO_ERROR);
 std::vector<unsigned char> pixels(512*512*4);glReadPixels(0,0,512,512,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());unsigned red=0,green=0,white=0;
 for(size_t i=0;i<pixels.size();i+=4){red+=pixels[i]>100&&pixels[i+1]<10;green+=pixels[i+1]>100&&pixels[i]<10;white+=pixels[i]>240&&pixels[i+1]>240&&pixels[i+2]>240;}
 assert(red>100&&green>100&&white>100);
 coopMenuOpen=false;glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);drawCoopTransfer();glReadPixels(0,0,512,512,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());for(size_t i=0;i<pixels.size();i+=4)assert(pixels[i]==0&&pixels[i+1]==0&&pixels[i+2]==0);
 puts("PASS recipient identity, two row pointer hits/no cancel/outside/behind, colored buttons and white text, hidden menu, GL_NO_ERROR");
}
