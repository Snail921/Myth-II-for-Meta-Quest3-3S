package org.mythvr.standalone;
import java.io.*;
import java.nio.file.*;
import java.util.*;
import org.json.*;
/** Isolated Quest test of successive external campaign level activation. */
public class PluginCampaignTest {
 public static void main(String[] args)throws Exception {
  ExternalLevelTest.C c=new ExternalLevelTest.C(args[0],args[1]){public File getExternalFilesDir(String type){return args.length>3?new File(args[3]):super.getExternalFilesDir(type);}};
  List<String[]> catalog=LevelAssets.catalog(c);
  int tested=0;
  for(String key:args[2].split(",")){
   String[] selected=null;
   for(String[] row:catalog)if(row[0].equals(key))selected=row;
   if(selected==null||!LevelAssets.external.containsKey(key)||LevelAssets.builtin.containsKey(key))throw new AssertionError("new external level missing: "+key);
   JSONObject source=LevelAssets.candidateRequirements.get(selected);
   ActivePlugins.Snapshot loaded=new ActivePlugins.Snapshot();
   JSONArray plugins=source.getJSONArray("active_plugins");
   for(int i=0;i<plugins.length();i++)loaded.plugins.add(plugins.getString(i));
   List<String[]> matches=new ArrayList<>();
   for(String[] row:catalog)if(row[1].equals(selected[1])&&row[2].equals(selected[2])&&row[3].equals(selected[3])&&row[9].equals(selected[9]))matches.add(row);
   if(LevelAssets.select(matches,loaded)!=selected)throw new AssertionError("ambiguous pack selection: "+key);
   LevelAssets.activate(c,selected);LevelAssets.publish(c);
   if(LevelAssets.active==null||!LevelAssets.activeKey.equals(key))throw new AssertionError("wrong active level");
   for(String name:LevelAssets.REQUIRED)try(InputStream in=LevelAssets.open(c,key,name)){
    if(name.equals("collections.bin")){
     ByteArrayOutputStream out=new ByteArrayOutputStream();byte[] b=new byte[4096];int n;
     while((n=in.read(b))!=-1)out.write(b,0,n);
     CollectionManifest manifest=new CollectionManifest(out.toByteArray());
     if(!manifest.tagged)throw new AssertionError("legacy plugin identities");
    }else if(in.read()<0&&!name.equals("detail-bindings.tsv"))throw new AssertionError("empty asset");
   }
   String published=new String(Files.readAllBytes(new File(c.getFilesDir(),"active-level-assets.path").toPath()),"UTF-8");
   if(!published.equals(LevelAssets.active.getAbsolutePath()))throw new AssertionError("native path");
   tested++;System.out.println("PASS "+key+" "+selected[8]+": verified/staged 15 assets and tagged identities");
  }
  System.out.println("PASS campaign level switches: "+tested);
 }
}
