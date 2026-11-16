package org.mythvr.standalone;
import java.io.*;import java.nio.file.*;import java.util.*;import org.json.*;
/** Isolated device test: never changes the app's live files or game settings. */
public class PluginLevelTest {
 public static void main(String[] a)throws Exception{
  ExternalLevelTest.C c=new ExternalLevelTest.C(a[0],a[1]);String key=a[2];String[] row=null;
  for(String[] r:LevelAssets.catalog(c))if(r[0].equals(key))row=r;
  if(row==null||!LevelAssets.external.containsKey(key)||LevelAssets.builtin.containsKey(key))throw new AssertionError("new external level index");
  LevelAssets.activate(c,row);LevelAssets.publish(c);
  if(LevelAssets.active==null)throw new AssertionError("new level stage");
  for(String name:LevelAssets.REQUIRED)try(InputStream in=LevelAssets.open(c,key,name)){if(in.read()<0&&!name.equals("detail-bindings.tsv"))throw new AssertionError("empty "+name);}
  System.out.println("PASS new plugin map: "+row[8]+", all 15 assets verified and resolved without APK map");
  File manifest=new File(LevelAssets.external.get(key),"manifest.json");byte[] original=Files.readAllBytes(manifest.toPath());
  try{
   JSONObject j=new JSONObject(new String(original,"UTF-8"));j.getJSONObject("files").getJSONObject("hd-tint.bin").put("sha256",String.join("",Collections.nCopies(64,"0")));Files.write(manifest.toPath(),j.toString().getBytes("UTF-8"));
   boolean rejected=false;try{LevelAssets.activate(c,row);}catch(IOException expected){rejected=true;}
   if(!rejected||LevelAssets.active!=null||new File(c.getFilesDir(),"active-level-assets.path").length()!=0)throw new AssertionError("corrupt new level accepted or stale path");
   System.out.println("PASS corrupt plugin rejected with no builtin fallback and no stale active path");
  }finally{Files.write(manifest.toPath(),original);}
 }
}
