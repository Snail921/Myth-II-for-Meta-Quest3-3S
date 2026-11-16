package org.mythvr.standalone;
import java.io.*;import java.util.*;import org.json.*;
public class RealPackSelectionTest {
 public static void main(String[] args)throws Exception{
  ExternalLevelTest.C c=new ExternalLevelTest.C(args[0],args[1]){public File getExternalFilesDir(String type){return new File(args[2]);}};
  List<String[]> rows=LevelAssets.catalog(c);int checked=0;
  for(String[] row:rows){File f=LevelAssets.candidates.get(row);if(f==null||!f.toString().contains("chimera-campaign-v2"))continue;
   JSONObject source=LevelAssets.candidateRequirements.get(row);ActivePlugins.Snapshot loaded=new ActivePlugins.Snapshot();JSONArray plugins=source.getJSONArray("active_plugins");for(int i=0;i<plugins.length();i++)loaded.plugins.add(plugins.getString(i));
   List<String[]> matches=new ArrayList<>();for(String[] r:rows)if(r[1].equals(row[1])&&r[2].equals(row[2])&&r[3].equals(row[3])&&r[9].equals(row[9]))matches.add(r);
   if(LevelAssets.select(matches,loaded)!=row)throw new AssertionError(row[8]);checked++;System.out.println("PASS deployed pack selection: "+row[8]);
  }
  if(checked!=8)throw new AssertionError("Chimera count="+checked);
  List<String[]> willow=new ArrayList<>();for(String[] r:rows)if(r[0].equals("6c312020"))willow.add(r);
  String[] w=LevelAssets.select(willow,new ActivePlugins.Snapshot());if(w==null||!LevelAssets.candidates.containsKey(w))throw new AssertionError("Willow external regression");
  System.out.println("PASS 8 deployed Chimera packs and existing external Willow Creek (read-only)");
 }
}
