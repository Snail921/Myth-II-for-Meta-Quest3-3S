package org.mythvr.standalone;
import java.io.*;
import java.nio.file.*;
import java.security.MessageDigest;
import java.util.*;
import org.json.*;

/** Verifies required files, not the engine's active plugin selection/load order. */
final class PluginSources {
 static String digest(File file)throws Exception{
  MessageDigest md=MessageDigest.getInstance("SHA-256");
  try(InputStream in=new FileInputStream(file)){byte[] b=new byte[262144];int n;while((n=in.read(b))!=-1)md.update(b,0,n);}
  return LevelAssets.hex(md.digest());
 }
 static JSONObject requirements(File level,JSONObject manifest)throws Exception{
  if(!manifest.has("source_files_sha256"))return null; // Legacy packs remain readable.
  String hash=manifest.getString("source_files_sha256");
  if(!hash.matches("[0-9a-f]{64}"))throw new IOException("source manifest hash");
  File pack=level.getParentFile().getParentFile();
  File source=LevelAssets.child(pack,"source-files.json");
  if(!source.isFile()||source.length()>65536)throw new IOException("Missing/oversized source-files.json");
  byte[] bytes=Files.readAllBytes(source.toPath());
  if(!LevelAssets.hex(MessageDigest.getInstance("SHA-256").digest(bytes)).equals(hash))throw new IOException("Source requirements changed");
  JSONObject result=new JSONObject(new String(bytes,"UTF-8"));
  if(result.getInt("version")!=1)throw new IOException("Unsupported source requirements");
  return result;
 }
 static int verify(File level,JSONObject manifest,File game,String engineHash)throws Exception{
  JSONObject source=requirements(level,manifest);if(source==null)return 0;
  if(!source.getString("engine_sha256").equals(engineHash))throw new IOException("Source pack requires another engine version");
  if(!digest(LevelAssets.child(game,"Myth II.exe")).equals(engineHash))throw new IOException("Original executable changed");
  JSONArray files=source.getJSONArray("files");
  if(files.length()<1||files.length()>256)throw new IOException("Source archive count");
  Set<String> seen=new HashSet<>();
  for(int i=0;i<files.length();i++){
   JSONObject spec=files.getJSONObject(i);String path=spec.getString("path");
   if(!path.matches("(?:tags|plugins)/[^/\\\\]+")||path.indexOf('\0')>=0||!seen.add(path.toLowerCase(Locale.ROOT)))throw new IOException("Invalid source archive path");
   String hash=spec.getString("sha256");long size=spec.getLong("size");
   if(size<0||!hash.matches("[0-9a-f]{64}"))throw new IOException("Invalid source archive metadata");
   File file=LevelAssets.child(game,path);
   if(!file.isFile())throw new IOException("必要な原作ファイルがありません: "+path);
   if(file.length()!=size||!digest(file).equals(hash))throw new IOException("原作ファイルの版が外部パックと一致しません: "+path);
  }
  return files.length();
 }
}
