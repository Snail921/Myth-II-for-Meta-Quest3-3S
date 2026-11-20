package org.mythvr.standalone;
import android.content.*;import android.graphics.Bitmap;import java.io.*;import java.nio.file.*;import java.lang.reflect.Proxy;import java.util.*;
public class SkyboxChoiceTest {
 static void check(boolean b){if(!b)throw new AssertionError();}
 static class C extends ContextWrapper{
  final File dir;boolean enabled;
  C(String root){super(null);dir=new File(root);}
  public File getFilesDir(){File f=new File(dir,"internal");f.mkdirs();return f;}
  public File getExternalFilesDir(String t){File f=new File(dir,"external");f.mkdirs();return f;}
  public SharedPreferences getSharedPreferences(String n,int mode){return (SharedPreferences)Proxy.newProxyInstance(SharedPreferences.class.getClassLoader(),new Class[]{SharedPreferences.class},(o,m,a)->{if(m.getName().equals("getBoolean"))return enabled;throw new UnsupportedOperationException();});}
 }
 static void image(File f,int w,int h)throws Exception{Bitmap b=Bitmap.createBitmap(w,h,Bitmap.Config.ARGB_8888);try(FileOutputStream out=new FileOutputStream(f)){b.compress(Bitmap.CompressFormat.PNG,100,out);}b.recycle();}
 static String choice(C c)throws Exception{return new String(Files.readAllBytes(new File(c.getFilesDir(),"active-skybox.txt").toPath()),"UTF-8");}
 public static void main(String[] args)throws Exception{
  C c=new C(args[0]);File dir=new File(c.getExternalFilesDir(null),"Skyboxes");dir.mkdirs();
  image(new File(dir,"Match.png"),8,4);image(new File(dir,"Campaign.png"),8,4);image(new File(dir,"Invalid.png"),8,8);
  Files.write(new File(dir,"Match.skybox.json").toPath(),"{\"runtime_yaw_degrees\":42.5}".getBytes("UTF-8"));
  check(SkyboxChoice.usable(dir,new File(dir,"Match.png")));check(!SkyboxChoice.usable(dir,new File(dir,"Invalid.png")));check(!SkyboxChoice.imageName("../escape.png"));check(!SkyboxChoice.imageName("bad\n.png"));
  for(int k=-1;k<=16;k++)check(SkyboxChoice.competitive(k)==(k>=0&&k<=15&&k!=13));
  c.enabled=false;SkyboxChoice.prepare(c,9,"Match",12);check(choice(c).equals("Match.png\n42.5\n"));
  c.enabled=true;SkyboxChoice.prepare(c,13,"Match",12);check(choice(c).equals("Match.png\n42.5\n"));
  for(int i=0;i<20;i++){SkyboxChoice.prepare(c,9,"Match",12);String s=choice(c);check(s.equals("Match.png\n42.5\n")||s.equals("Campaign.png\n0.0\n"));}
  String selected=choice(c);check(selected.equals(choice(c))); // no choice while merely returning to VR
  c.enabled=false;SkyboxChoice.prepare(c,9,"Missing",12);check(choice(c).equals("\n12.0\n"));
  System.out.println("PASS competitive-only/default-OFF selection, scenario candidate, invalid image exclusion, yaw metadata, fallback and persisted choice");
 }
}
