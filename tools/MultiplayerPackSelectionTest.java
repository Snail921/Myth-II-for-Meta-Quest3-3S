package org.mythvr.standalone;
import java.io.*;import java.util.*;import org.json.*;
/** Read-only audit against the real installed catalogs and source requirements. */
public class MultiplayerPackSelectionTest {
 public static void main(String[] args)throws Exception{
  ExternalLevelTest.C c=new ExternalLevelTest.C(args[0],args[1]){public File getExternalFilesDir(String type){return new File(args[2]);}};
  List<String[]> rows=LevelAssets.catalog(c);Map<String,Integer> counts=new TreeMap<>();
  for(String[] row:rows){File folder=LevelAssets.candidates.get(row);if(folder==null)continue;
   String pack=folder.getParentFile().getParentFile().getName();
   if(!pack.startsWith("multiplayer-")&&!pack.equals("soulblighter"))continue;
   JSONObject source=LevelAssets.candidateRequirements.get(row);ActivePlugins.Snapshot loaded=new ActivePlugins.Snapshot();
   if(source!=null){JSONArray plugins=source.optJSONArray("active_plugins");if(plugins!=null)for(int i=0;i<plugins.length();i++)loaded.plugins.add(plugins.getString(i));}
   List<String[]> matches=new ArrayList<>();for(String[] r:rows)if(r[1].equals(row[1])&&r[2].equals(row[2])&&r[3].equals(row[3])&&r[9].equals(row[9]))matches.add(r);
   if(LevelAssets.select(matches,loaded)!=row)throw new AssertionError("selection: "+row[8]);
   if(pack.startsWith("multiplayer-")&&!SkyboxChoice.usable(new File(args[2],"Skyboxes"),new File(args[2]+"/Skyboxes",row[8]+".png")))throw new AssertionError("sky: "+row[8]);
   counts.put(pack,counts.getOrDefault(pack,0)+1);
  }
  String[] expected=args.length>3?Arrays.copyOfRange(args,3,args.length):new String[]{"multiplayer-original:20","multiplayer-tfl:47","multiplayer-chimera:2","soulblighter:27"};
  for(String spec:expected){String[] v=spec.split(":");if(counts.getOrDefault(v[0],0)!=Integer.parseInt(v[1]))throw new AssertionError(spec+" actual="+counts);}
  System.out.println("PASS deployed source-aware unique selection, valid fixed skies, preserved base campaign: "+counts);
 }
}
