package org.mythvr.standalone;
import android.content.Context;
import android.util.Log;
import java.io.*;
import java.nio.file.*;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.util.*;
import org.json.*;

/** Whole-level bundles: never mix external geometry with APK sprite indices. */
final class LevelAssets {
 static final String[] REQUIRED={"collections.bin","scene.pack","sprite-indices.pack","models.pack","animated-models.pack","connectors.pack","particles.pack","reflection-water.pack","surface-effects.pack","minimap.rgba","hd-bindings.json","hd-tint.bin","detail-bindings.tsv","fog.pack","water-light.pack"};
 static final Map<String,File> external=new HashMap<>();
 static final Map<String,String[]> builtin=new HashMap<>();
 static final Map<String[],File> candidates=new IdentityHashMap<>();
 static final Map<String[],JSONObject> candidateRequirements=new IdentityHashMap<>();
 static File active;static String activeKey="";
 static File root(Context c){return new File(c.getExternalFilesDir(null),"LevelPacks");}
 static String[] row(String line)throws IOException{
  String[] r=line.split("\t",-1);if(r.length!=10||!r[0].matches("[0-9a-f]{8}")||!r[9].matches("[0-9a-f]{64}"))throw new IOException("level catalog identity");
  try{long tag=Long.parseLong(r[1]);if(tag<0||tag>0xffffffffL)throw new IOException("landscape ID");for(int i=2;i<4;i++){int n=Integer.parseInt(r[i]);if(n<32||n>512||n%32!=0)throw new IOException("map dimensions");}for(int i=4;i<8;i++)if(!Float.isFinite(Float.parseFloat(r[i])))throw new IOException("map coordinates");}catch(NumberFormatException e){throw new IOException("catalog number",e);}
  if(r[8].isEmpty()||r[8].getBytes(StandardCharsets.UTF_8).length>=64||r[8].indexOf('\0')>=0)throw new IOException("level name");return r;
 }
 static File child(File base,String name)throws IOException{File f=new File(base,name).getCanonicalFile();if(!f.toPath().startsWith(base.getCanonicalFile().toPath())||f.equals(base.getCanonicalFile()))throw new IOException("bundle path escape");return f;}
 static JSONObject manifest(File dir,String[] r)throws Exception{
  File f=child(dir,"manifest.json");if(f.length()>65536)throw new IOException("manifest size");JSONObject j=new JSONObject(new String(Files.readAllBytes(f.toPath()),StandardCharsets.UTF_8));
  if(j.getInt("version")!=1||!j.getString("catalog").equals(String.join("\t",r)))throw new IOException("manifest/catalog mismatch");
  JSONObject files=j.getJSONObject("files");if(files.length()!=REQUIRED.length)throw new IOException("bundle file set");long total=0;
  for(String name:REQUIRED){JSONObject spec=files.getJSONObject(name);long size=spec.getLong("size");if(size<0||size>512L*1024*1024||!spec.getString("sha256").matches("[0-9a-f]{64}"))throw new IOException("bundle file metadata");File data=child(dir,name);if(!data.isFile()||data.length()!=size)throw new IOException("bundle missing/size: "+name);total+=size;}
  if(total>1024L*1024*1024)throw new IOException("bundle total size");return j;
 }
 static List<String[]> catalog(Context c)throws Exception{
  external.clear();builtin.clear();candidates.clear();candidateRequirements.clear();LinkedHashMap<String,String[]> rows=new LinkedHashMap<>();
  try(BufferedReader in=new BufferedReader(new InputStreamReader(c.getAssets().open("levels/catalog.tsv"),StandardCharsets.UTF_8))){String line;while((line=in.readLine())!=null){String[] r=row(line);rows.put(r[0],r);builtin.put(r[0],r);}}catch(FileNotFoundException missing){/* External-only APK. */}catch(IOException missing){if(!rows.isEmpty())throw missing;}
  File base=root(c);base.mkdirs();File[] packs=base.listFiles();if(packs==null)return new ArrayList<>(rows.values());Arrays.sort(packs,Comparator.comparing(File::getName));List<String[]> alternatives=new ArrayList<>();Set<String> conflicts=new HashSet<>();
  for(File pack:packs){if(!pack.isDirectory()||pack.getName().startsWith("."))continue;try{
   if(!pack.getCanonicalFile().toPath().startsWith(base.getCanonicalFile().toPath()))throw new IOException("package path");File catalog=child(pack,"catalog.tsv");if(!catalog.isFile())continue;if(catalog.length()>1024*1024)throw new IOException("catalog size");
   for(String line:Files.readAllLines(catalog.toPath(),StandardCharsets.UTF_8)){if(line.trim().isEmpty())continue;try{String[] r=row(line);File dir=child(pack,"levels/"+r[0]);JSONObject metadata=manifest(dir,r);JSONObject requirements=PluginSources.requirements(dir,metadata);candidates.put(r,dir);candidateRequirements.put(r,requirements);alternatives.add(r);
    if(external.containsKey(r[0])||conflicts.contains(r[0])){external.remove(r[0]);conflicts.add(r[0]);}else external.put(r[0],dir);
    Log.i("MythStandalone","External level indexed: "+r[8]+" / "+pack.getName());
   }catch(Exception e){Log.e("MythStandalone","External level rejected: "+pack.getName(),e);}}
  }catch(Exception e){Log.e("MythStandalone","External package rejected: "+pack.getName(),e);}}
  List<String[]> result=new ArrayList<>(rows.values());result.addAll(alternatives);return result;
 }
 static String[] select(List<String[]> matches,ActivePlugins.Snapshot loaded)throws Exception{
  List<String[]> verified=new ArrayList<>(),legacy=new ArrayList<>(),internal=new ArrayList<>();
  for(String[] r:matches){File dir=candidates.get(r);if(dir==null){internal.add(r);continue;}
   JSONObject source=candidateRequirements.get(r);
   if(source==null||!source.has("active_plugins")){legacy.add(r);continue;}
   if(loaded==null)continue;
   List<String> expected=new ArrayList<>();JSONArray plugins=source.getJSONArray("active_plugins");
   for(int i=0;i<plugins.length();i++)expected.add(plugins.getString(i));
   if(expected.equals(loaded.plugins))verified.add(r);
  }
  // Source hashes and all archive identities are still checked before staging/VR entry.
  // Never select by directory sort order, or silently fall back to the base campaign
  // while a gameplay plugin is active.
  List<String[]> eligible=!verified.isEmpty()?verified:(!legacy.isEmpty()&&(loaded==null||loaded.plugins.isEmpty())?legacy:internal);
  if(verified.isEmpty()&&legacy.isEmpty()&&loaded!=null&&!loaded.plugins.isEmpty()&&!matches.isEmpty())throw new IOException("有効プラグインに一致する外部レベルパックがありません");
  if(verified.isEmpty()&&!legacy.isEmpty()&&loaded!=null&&!loaded.plugins.isEmpty())throw new IOException("外部パックに有効プラグインの照合情報がありません");
  if(eligible.size()>1)throw new IOException("同じ原作構成に一致するレベルパックが複数あります。重複パックを退避してください");
  return eligible.isEmpty()?null:eligible.get(0);
 }
 static String hex(byte[] b){StringBuilder s=new StringBuilder();for(byte v:b)s.append(String.format(Locale.ROOT,"%02x",v&255));return s.toString();}
 static void clear(File dir)throws IOException{if(!dir.exists())return;File[] files=dir.listFiles();if(files==null)throw new IOException("cache directory");for(File f:files){if(!f.isFile()||!f.delete())throw new IOException("cache cleanup");}if(!dir.delete())throw new IOException("cache cleanup directory");}
 static void purgeMatchingCache(Context c,String expected)throws Exception{
  if(!expected.matches("[0-9a-f]{64}"))throw new IOException("invalid cache digest");
  for(String name:new String[]{"external-level","external-level.tmp"}){
   File dir=new File(c.getFilesDir(),name),scene=new File(dir,"scene.pack");if(!scene.isFile())continue;
   MessageDigest sha=MessageDigest.getInstance("SHA-256");byte[] buf=new byte[65536];try(InputStream in=new FileInputStream(scene)){int n;while((n=in.read(buf))!=-1)sha.update(buf,0,n);}
   if(!hex(sha.digest()).equals(expected))continue;
   clear(dir);File signal=new File(c.getFilesDir(),"active-level-assets.path");Files.write(signal.toPath(),new byte[0]);
   Log.i("MythStandalone","Removed matching external level cache: "+name);
   Files.write(new File(c.getExternalFilesDir(null),"level-cache-removal.txt").toPath(),(expected+" removed "+name).getBytes(StandardCharsets.UTF_8));
  }
 }
 static String[] activate(Context c,String[] requested)throws Exception{
  active=null;activeKey="";File signal=new File(c.getFilesDir(),"active-level-assets.path");Files.write(signal.toPath(),new byte[0]);File source=candidates.get(requested);if(source==null)return requested;external.put(requested[0],source);
  File staging=new File(c.getFilesDir(),"external-level.tmp"),target=new File(c.getFilesDir(),"external-level");
  try{JSONObject metadata=manifest(source,requested);int verifiedSources=PluginSources.verify(source,metadata,DetailTextures.plugins().getParentFile(),EngineProfile.SHA256);if(verifiedSources>0)Log.i("MythStandalone","External source files verified: "+verifiedSources+" (active plugin order not checked)");JSONObject files=metadata.getJSONObject("files");clear(staging);if(!staging.mkdirs())throw new IOException("create level staging");
   byte[] buf=new byte[65536];for(String name:REQUIRED){JSONObject spec=files.getJSONObject(name);MessageDigest sha=MessageDigest.getInstance("SHA-256");long size=0;try(InputStream in=new FileInputStream(child(source,name));OutputStream out=new FileOutputStream(new File(staging,name))){int n;while((n=in.read(buf))!=-1){size+=n;if(size>spec.getLong("size"))throw new IOException("bundle changed: "+name);sha.update(buf,0,n);out.write(buf,0,n);}}
    if(size!=spec.getLong("size")||!hex(sha.digest()).equals(spec.getString("sha256")))throw new IOException("bundle checksum: "+name);
   }
   clear(target);Files.move(staging.toPath(),target.toPath());active=target;activeKey=requested[0];Log.i("MythStandalone","External level verified and staged: "+requested[8]);return requested;
  }catch(Exception e){clear(staging);String[] original=builtin.get(requested[0]);if(original==null||!original[9].equals(requested[9])||!original[1].equals(requested[1])||!original[2].equals(requested[2])||!original[3].equals(requested[3]))throw e;Log.e("MythStandalone","External level invalid; using complete APK level",e);return original;}
 }
 static InputStream open(Context c,String key,String name)throws IOException{
  if(active!=null&&activeKey.equals(key))return new FileInputStream(child(active,name));
  return c.getAssets().open(name.equals("detail-bindings.tsv")?"levels/"+name:"levels/"+key+"/"+name);
 }
 static void publish(Context c)throws IOException{File f=new File(c.getFilesDir(),"active-level-assets.path");Files.write(f.toPath(),(active==null?"":active.getAbsolutePath()).getBytes(StandardCharsets.UTF_8));}
}
