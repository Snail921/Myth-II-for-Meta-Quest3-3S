package org.mythvr.standalone;
import java.io.*;import java.nio.file.*;import java.util.*;import org.json.*;
public class MultiPackTest {
 static void check(boolean b){if(!b)throw new AssertionError();}
 interface Check{void run()throws Exception;}
 static void rejects(Check c)throws Exception{try{c.run();}catch(IOException expected){return;}throw new AssertionError("ambiguous/unmatched accepted");}
 static String hash="0000000000000000000000000000000000000000000000000000000000000000";
 static void pack(File root,String name,String row,String... plugins)throws Exception{
  File dir=new File(root,name),level=new File(dir,"levels/"+row.split("\t")[0]);level.mkdirs();JSONObject files=new JSONObject();
  for(String f:LevelAssets.REQUIRED){Files.write(new File(level,f).toPath(),new byte[]{1});files.put(f,new JSONObject().put("size",1).put("sha256",hash));}
  JSONObject req=new JSONObject().put("version",1).put("active_plugins",new JSONArray(Arrays.asList(plugins)));
  File source=new File(dir,"source-files.json");Files.write(source.toPath(),req.toString().getBytes("UTF-8"));
  JSONObject m=new JSONObject().put("version",1).put("catalog",row).put("files",files).put("source_files_sha256",PluginSources.digest(source));
  Files.write(new File(level,"manifest.json").toPath(),m.toString().getBytes("UTF-8"));Files.write(new File(dir,"catalog.tsv").toPath(),(row+"\n").getBytes("UTF-8"));
 }
 static List<String[]> matches(List<String[]> rows,String key){List<String[]> out=new ArrayList<>();for(String[] r:rows)if(r[0].equals(key))out.add(r);return out;}
 public static void main(String[] args)throws Exception{
  ExternalLevelTest.C c=new ExternalLevelTest.C(args[0],args[1]);List<String[]> base=LevelAssets.catalog(c);String row=String.join("\t",base.get(0)),key=base.get(0)[0];
  File root=LevelAssets.root(c);pack(root,"a",row,"plugin-A");pack(root,"b",row,"plugin-B");
  List<String[]> list=matches(LevelAssets.catalog(c),key);check(list.size()==3);ActivePlugins.Snapshot s=new ActivePlugins.Snapshot();s.plugins.add("plugin-A");String[] chosen=LevelAssets.select(list,s);check(LevelAssets.candidates.get(chosen).getParentFile().getParentFile().getName().equals("a"));
  s.plugins.set(0,"plugin-B");chosen=LevelAssets.select(list,s);check(LevelAssets.candidates.get(chosen).getParentFile().getParentFile().getName().equals("b"));
  s.plugins.set(0,"unknown");rejects(()->LevelAssets.select(list,s));s.plugins.clear();check(!LevelAssets.candidates.containsKey(LevelAssets.select(list,s)));
  pack(root,"ordered",row,"plugin-A","plugin-B");List<String[]> ordered=matches(LevelAssets.catalog(c),key);s.plugins.add("plugin-A");s.plugins.add("plugin-B");check(LevelAssets.select(ordered,s)!=null);Collections.reverse(s.plugins);rejects(()->LevelAssets.select(ordered,s));
  pack(root,"duplicate",row,"plugin-A");List<String[]> duplicate=matches(LevelAssets.catalog(c),key);s.plugins.clear();s.plugins.add("plugin-A");rejects(()->LevelAssets.select(duplicate,s));
  // Hidden backups cannot create an ambiguity.
  check(new File(root,"duplicate").renameTo(new File(root,".duplicate")));check(LevelAssets.select(matches(LevelAssets.catalog(c),key),s)!=null);
  // Changed requirements are rejected while indexing, never silently retarget a pack.
  Files.write(new File(root,"a/source-files.json").toPath(),"{}".getBytes("UTF-8"));List<String[]> changed=matches(LevelAssets.catalog(c),key);rejects(()->LevelAssets.select(changed,s));
  System.out.println("PASS same-ID/same-fingerprint packs A/B, base fallback, unknown plugin, ordered combination, reversed order, duplicate rejection, hidden backup, altered source manifest");
 }
}
