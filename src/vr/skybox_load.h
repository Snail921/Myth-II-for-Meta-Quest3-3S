#pragma once
#include <sys/stat.h>
// Android's PNG/JPEG decoder, once per VR entry on the native render thread.
// No custom Java class loader dependency; only boot-classpath graphics classes.
static void loadSkybox(JNIEnv*env,const char*externalRoot,const char*level,float yaw){
 clearSkybox();skyYaw=yaw;
 if(!externalRoot||!level||strchr(level,'/')||strchr(level,'\\')||!strcmp(level,".."))return;
 if(env->PushLocalFrame(24)<0){env->ExceptionClear();return;}
 auto decode=[&](){
  char path[2048];bool found=false;struct stat st{};
  for(const char*ext:{"png","jpg","jpeg"}){snprintf(path,sizeof(path),"%s/Skyboxes/%s.%s",externalRoot,level,ext);if(!stat(path,&st)&&S_ISREG(st.st_mode)){found=true;break;}}
  if(!found||st.st_size<=0||st.st_size>64*1024*1024){LOG("Skybox absent or oversized: %s",level);return;}
  jclass factory=env->FindClass("android/graphics/BitmapFactory"),options=env->FindClass("android/graphics/BitmapFactory$Options"),bitmapClass=env->FindClass("android/graphics/Bitmap");
  if(!factory||!options||!bitmapClass)return;
  auto ctor=env->GetMethodID(options,"<init>","()V");auto decode=env->GetStaticMethodID(factory,"decodeFile","(Ljava/lang/String;Landroid/graphics/BitmapFactory$Options;)Landroid/graphics/Bitmap;");
  auto bounds=env->GetFieldID(options,"inJustDecodeBounds","Z"),width=env->GetFieldID(options,"outWidth","I"),height=env->GetFieldID(options,"outHeight","I");
  if(env->ExceptionCheck())return;
  jobject opt=env->NewObject(options,ctor);jstring name=env->NewStringUTF(path);if(!opt||!name)return;
  env->SetBooleanField(opt,bounds,JNI_TRUE);env->CallStaticObjectMethod(factory,decode,name,opt);if(env->ExceptionCheck())return;
  int w=env->GetIntField(opt,width),h=env->GetIntField(opt,height);GLint limit;glGetIntegerv(GL_MAX_TEXTURE_SIZE,&limit);
  if(w<2||h<1||h>2048||w!=h*2||w>4096||w>limit){LOG("Skybox rejected dimensions %dx%d: %s",w,h,path);return;}
  env->SetBooleanField(opt,bounds,JNI_FALSE);jobject bitmap=env->CallStaticObjectMethod(factory,decode,name,opt);if(!bitmap||env->ExceptionCheck())return;
  auto recycle=env->GetMethodID(bitmapClass,"recycle","()V"),getPixels=env->GetMethodID(bitmapClass,"getPixels","([IIIIIII)V");
  jintArray pixels=env->NewIntArray(w*h);if(!pixels)return;
  env->CallVoidMethod(bitmap,getPixels,pixels,0,w,0,0,w,h);if(env->ExceptionCheck())return;
  std::vector<jint> argb(w*h);env->GetIntArrayRegion(pixels,0,w*h,argb.data());if(env->ExceptionCheck())return;
  env->CallVoidMethod(bitmap,recycle);std::vector<unsigned char> rgba(size_t(w)*h*4);
  for(size_t i=0;i<argb.size();i++){unsigned c=argb[i];rgba[i*4]=c>>16;rgba[i*4+1]=c>>8;rgba[i*4+2]=c;rgba[i*4+3]=255;}
  uploadSkybox(w,h,rgba.data());LOG("Skybox loaded %s %dx%d yaw=%.2f",path,w,h,yaw*180/3.14159265f);
 };
 decode();if(env->ExceptionCheck()){env->ExceptionClear();clearSkybox();LOG("Skybox decode failed; black fallback");}env->PopLocalFrame(nullptr);
}
