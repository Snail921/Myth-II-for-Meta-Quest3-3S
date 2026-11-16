package org.mythvr.standalone;
import java.io.*;
import java.nio.file.*;
import org.json.*;
public class PluginSourcesTest {
 interface Check {void run()throws Exception;}
 static void rejects(Check c)throws Exception{try{c.run();}catch(IOException expected){return;}throw new AssertionError("Mismatch accepted");}
 static JSONObject bind(File pack,JSONObject requirements)throws Exception{
  File file=new File(pack,"source-files.json");Files.write(file.toPath(),requirements.toString().getBytes("UTF-8"));
  return new JSONObject().put("source_files_sha256",PluginSources.digest(file));
 }
 public static void main(String[] args)throws Exception{
  File real=new File(args[0]);JSONObject manifest=new JSONObject(new String(Files.readAllBytes(new File(real,"manifest.json").toPath()),"UTF-8"));
  int n=PluginSources.verify(real,manifest,new File(args[1]),EngineProfile.SHA256);
  if(n!=11)throw new AssertionError("Expected 11 Chimera source archives");
  System.out.println("PASS actual Quest executable and 11 source archives");
  File base=new File(args[2]);if(!base.mkdirs())throw new IOException("Use fresh test directory");
  File pack=new File(base,"pack"),level=new File(pack,"levels/30316869"),game=new File(base,"game");level.mkdirs();new File(game,"plugins").mkdirs();
  File exe=new File(game,"Myth II.exe"),archive=new File(game,"plugins/example");Files.write(exe.toPath(),new byte[]{1,2,3});Files.write(archive.toPath(),new byte[]{4,5,6});
  String engine=PluginSources.digest(exe);
  JSONObject spec=new JSONObject().put("path","plugins/example").put("size",3).put("sha256",PluginSources.digest(archive));
  JSONObject req=new JSONObject().put("version",1).put("engine_sha256",engine).put("files",new JSONArray().put(spec));
  JSONObject m=bind(pack,req);
  if(PluginSources.verify(level,m,game,engine)!=1)throw new AssertionError("valid fixture");
  Files.write(archive.toPath(),new byte[]{6,5,4});rejects(()->PluginSources.verify(level,m,game,engine));
  Files.delete(archive.toPath());rejects(()->PluginSources.verify(level,m,game,engine));
  Files.write(archive.toPath(),new byte[]{4,5,6});rejects(()->PluginSources.verify(level,m,game,"wrong engine"));
  Files.write(new File(pack,"source-files.json").toPath(),new byte[]{1});rejects(()->PluginSources.verify(level,m,game,engine));
  spec.put("path","plugins/../../escape");JSONObject traversal=bind(pack,req);rejects(()->PluginSources.verify(level,traversal,game,engine));
  if(PluginSources.verify(level,new JSONObject(),game,engine)!=0)throw new AssertionError("legacy compatibility");
  System.out.println("PASS changed/missing archive, engine mismatch, changed requirements, path escape, legacy pack");
 }
}
