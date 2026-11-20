package org.mythvr.standalone;
import android.content.Context;
import android.graphics.BitmapFactory;
import android.util.AtomicFile;
import android.util.Log;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.*;
import org.json.JSONObject;

/** A choice is persisted only when preparing a new match, not on VR re-entry. */
final class SkyboxChoice {
 static boolean competitive(int kind){return kind>=0&&kind<=15&&kind!=13;}
 static boolean imageName(String name){String n=name.toLowerCase(Locale.ROOT);return name.indexOf('/')<0&&name.indexOf('\\')<0&&name.indexOf('\n')<0&&name.indexOf('\r')<0&&name.getBytes(StandardCharsets.UTF_8).length<480&&(n.endsWith(".png")||n.endsWith(".jpg")||n.endsWith(".jpeg"));}
 static boolean usable(File dir,File f){try{
  if(!imageName(f.getName())||!f.isFile()||!f.getCanonicalFile().getParentFile().equals(dir.getCanonicalFile())||f.length()<=0||f.length()>64L*1024*1024)return false;
  BitmapFactory.Options opt=new BitmapFactory.Options();opt.inJustDecodeBounds=true;BitmapFactory.decodeFile(f.getPath(),opt);
  return opt.outHeight>0&&opt.outHeight<=2048&&opt.outWidth==2*opt.outHeight&&opt.outWidth<=4096;
 }catch(Exception e){return false;}}
 static void prepare(Context c,int kind,String level,float fixedYaw)throws IOException{
  File root=new File(c.getExternalFilesDir(null),"Skyboxes"),chosen=null;float yaw=fixedYaw;
  boolean random=competitive(kind)&&c.getSharedPreferences("vr_graphics",Context.MODE_PRIVATE).getBoolean("multiplayer_random_skybox",false);
  if(random){List<File> choices=new ArrayList<>();File[] all=root.listFiles();if(all!=null)for(File f:all)if(usable(root,f))choices.add(f);
   choices.sort(Comparator.comparing(File::getName));if(!choices.isEmpty())chosen=choices.get(new java.security.SecureRandom().nextInt(choices.size()));
  }
  if(chosen==null)for(String ext:new String[]{"png","jpg","jpeg"}){File f=new File(root,level+"."+ext);if(usable(root,f)){chosen=f;break;}}
  if(chosen!=null){if(random)yaw=0;String n=chosen.getName();File meta=new File(root,n.substring(0,n.lastIndexOf('.'))+".skybox.json");
   try{if(meta.isFile()&&meta.length()<=65536){double angle=new JSONObject(new String(Files.readAllBytes(meta.toPath()),StandardCharsets.UTF_8)).optDouble("runtime_yaw_degrees",yaw);if(Double.isFinite(angle))yaw=(float)angle;}}catch(Exception ignored){}
  }
  AtomicFile file=new AtomicFile(new File(c.getFilesDir(),"active-skybox.txt"));FileOutputStream out=file.startWrite();
  try{out.write(((chosen==null?"":chosen.getName())+"\n"+Float.toString(yaw)+"\n").getBytes(StandardCharsets.UTF_8));file.finishWrite(out);}catch(IOException e){file.failWrite(out);throw e;}
  Log.i("MythSkybox","Match choice kind="+kind+" random="+random+" level="+level+" image="+(chosen==null?"map fallback":chosen.getName())+" yaw="+yaw);
 }
}
