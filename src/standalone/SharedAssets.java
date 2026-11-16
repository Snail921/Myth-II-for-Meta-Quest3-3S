package org.mythvr.standalone;
import android.content.Context;
import java.io.*;
import java.nio.file.*;
import org.json.*;
/** Small shared UI assets produced from the user's original archives. */
final class SharedAssets {
 static final String[] NAMES={"info-font.rgba","info-decoration.pack","dialog-templates.pack"};
 static synchronized void prepare(Context c)throws Exception {
  File source=new File(c.getExternalFilesDir(null),"SharedAssets");
  File manifest=new File(source,"manifest.json");
  if(!manifest.isFile()||manifest.length()>16384)throw new IOException("SharedAssets/manifest.json is missing or invalid");
  JSONObject root=new JSONObject(new String(Files.readAllBytes(manifest.toPath()),"UTF-8"));
  if(root.getInt("version")!=1)throw new IOException("SharedAssets version");
  JSONObject specs=root.getJSONObject("files");
  File dest=new File(c.getFilesDir(),"shared-assets");dest.mkdirs();
  for(String name:NAMES){
   JSONObject spec=specs.getJSONObject(name);File input=new File(source,name);long size=spec.getLong("size");
   if(size<1||size>16*1024*1024||input.length()!=size||!PluginSources.digest(input).equals(spec.getString("sha256")))throw new IOException("SharedAssets checksum: "+name);
   File output=new File(dest,name);if(output.isFile()&&PluginSources.digest(output).equals(spec.getString("sha256")))continue;
   File temp=new File(dest,name+".tmp");Files.copy(input.toPath(),temp.toPath(),StandardCopyOption.REPLACE_EXISTING);
   if(!PluginSources.digest(temp).equals(spec.getString("sha256")))throw new IOException("SharedAssets changed during copy");
   Files.move(temp.toPath(),output.toPath(),StandardCopyOption.REPLACE_EXISTING);
  }
 }
 static InputStream open(Context c,String name)throws IOException {return new FileInputStream(new File(new File(c.getFilesDir(),"shared-assets"),name));}
}
