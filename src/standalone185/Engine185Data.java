package org.mythvr.standalone;
import android.content.Context;
import java.io.*;
/** User-provided engine files; never installs originals from the APK. */
final class Engine185Data {
 static final File TARGET=new File("/storage/emulated/0/Android/data/org.mythvr.standalone/files/Game");
 static synchronized void prepare(Context c)throws Exception {verify();SharedAssets.prepare(c);
  File config=new File(c.getFilesDir(),"rootfs/home/xuser-1/.container");
  if(config.isFile()){
   org.json.JSONObject j=new org.json.JSONObject(new String(java.nio.file.Files.readAllBytes(config.toPath()),"UTF-8"));
   j.put("drives","D:"+TARGET.getPath()+"E:/storage/emulated/0/Download");
   android.util.AtomicFile f=new android.util.AtomicFile(config);FileOutputStream out=f.startWrite();
   try{out.write(j.toString().getBytes("UTF-8"));f.finishWrite(out);}catch(Exception e){f.failWrite(out);throw e;}
  }}
 static void verify()throws Exception {
  File exe=new File(TARGET,"Myth II.exe");
  if(!exe.isFile())throw new IOException("Copy the PC manager's Game folder into the app's files folder first.");
  if(!PluginSources.digest(exe).equals(EngineProfile.SHA256))throw new IOException("Unsupported 1.8.5 executable");
  if(!new File(TARGET,"tags/small install").isFile()||!new File(TARGET,"SMACKW32.DLL").isFile())throw new IOException("Original game data is incomplete");
 }
}
