package org.mythvr.standalone;
import java.io.*;
import java.nio.*;
import java.util.*;
import org.json.*;

/** Read-only Build471b loaded archive pool, after the engine's priority reorder. */
final class ActivePlugins {
 interface Memory {byte[] read(long address,int size)throws IOException;}
 static int u16(byte[] b,int p){return (b[p]&255)|((b[p+1]&255)<<8);}
 static long u32(byte[] b,int p){return Integer.toUnsignedLong(ByteBuffer.wrap(b).order(ByteOrder.LITTLE_ENDIAN).getInt(p));}
 static String identity(int kind,byte[] name,long checksum){return kind+":"+LevelAssets.hex(name)+":"+checksum;}
 static final class Snapshot {
  final List<String> identities=new ArrayList<>(),plugins=new ArrayList<>();
  final JSONArray archives=new JSONArray();
  JSONObject json()throws JSONException{return new JSONObject().put("order","loaded archive pool order").put("archives",archives);}
 }
 static byte[] capture(Memory memory,long global)throws IOException{
  long pool=u32(memory.read(global,4),0);if(pool<65536)throw new IOException("Loaded archive pool unavailable");
  byte[] head=memory.read(pool,28);int capacity=u16(head,4),stride=u16(head,6),count=u16(head,18);
  long data=u32(head,24);
  if(capacity!=32||stride!=408||count>capacity||count<1||data<65536||data+(long)count*stride>0x100000000L)throw new IOException("Loaded archive pool bounds");
  ByteArrayOutputStream result=new ByteArrayOutputStream();result.write(memory.read(global,4));result.write(head);
  for(int i=0;i<count;i++)result.write(memory.read(data+(long)i*stride,136));
  if(!Arrays.equals(head,memory.read(pool,28))||u32(memory.read(global,4),0)!=pool)throw new IOException("Loaded archives changed during read");
  return result.toByteArray();
 }
 static Snapshot read(Memory memory,long global)throws Exception{
  if(global==0)throw new IOException("Active plugin reader unsupported by engine");
  byte[] bytes=capture(memory,global);if(!Arrays.equals(bytes,capture(memory,global)))throw new IOException("Loaded archives are changing");
  Snapshot out=new Snapshot();Set<String> unique=new HashSet<>();
  for(int p=32,index=0;p<bytes.length;p+=136,index++){
   if(u16(bytes,p)==0)continue;
   int kind=u16(bytes,p+8);byte[] name=Arrays.copyOfRange(bytes,p+12,p+44);long checksum=u32(bytes,p+112);
   int end=0;while(end<name.length&&name[end]!=0)end++;
   if(kind>7||end==0)throw new IOException("Invalid loaded archive identity");
   // Padding after the terminator is not part of the archive name.
   Arrays.fill(name,end,name.length,(byte)0);
   String id=identity(kind,name,checksum);if(!unique.add(id))throw new IOException("Duplicate loaded archive identity");
   out.identities.add(id);if(kind==1)out.plugins.add(id);
   out.archives.put(new JSONObject().put("slot",index).put("kind",kind).put("name",new String(name,0,end,"ISO-8859-1")).put("checksum",checksum).put("identity",id));
  }
  if(out.identities.isEmpty())throw new IOException("No loaded archives");
  return out;
 }
 static Set<String> resourceAddons(File directory)throws Exception{
  Set<String> result=new HashSet<>();File[] files=directory.listFiles();if(files==null)return result;
  for(File file:files){if(!file.isFile()||file.getName().startsWith("."))continue;
   try(RandomAccessFile f=new RandomAccessFile(file,"r")){
    if(f.length()<128)continue;byte[] h=new byte[128];f.readFully(h);
    if(DetailCodec.be(h,124)!=0x646e6732||DetailCodec.us(h,0)!=6)continue;
    int mode=DetailCodec.us(h,2);boolean safe=false;
    if(mode==8){String hash=PluginSources.digest(file);safe=hash.equals("bea867e06a555dd4cfad1d64f059810d21e6b4f2d12c1208e7447070e6ce84ad")||hash.equals("b4980ace2f19323f6b981974092905ebd7536eee75fb1119daf61c421c06e5a4");}
    else if(mode==101||mode==102){
     int entries=DetailCodec.us(h,100),count=DetailCodec.us(h,102);long start=128L+entries*112L;
     safe=entries==0&&count>0&&count<=16384&&start+count*64L<=f.length();
     if(safe){f.seek(start);for(int i=0;i<count;i++){byte[] row=new byte[64];f.readFully(row);int type=DetailCodec.be(row,36);long offset=Integer.toUnsignedLong(DetailCodec.be(row,44));int size=DetailCodec.be(row,48);
      if((type!=0x646d6170&&type!=0x64746578&&type!=0x64323536)||DetailCodec.be(row,60)!=0x6d746832||size<0||offset<start+count*64L||offset+size>f.length()){safe=false;break;}
     }}
    }
    if(safe){byte[] name=Arrays.copyOfRange(h,4,36);int end=0;while(end<32&&name[end]!=0)end++;Arrays.fill(name,end,32,(byte)0);
     String id=identity(6,name,Integer.toUnsignedLong(DetailCodec.be(h,104)));if(!result.add(id))throw new IOException("Ambiguous resource addon identity");}
   }
  }return result;
 }
 static void verify(Snapshot actual,JSONObject source)throws Exception{verify(actual,source,Collections.emptySet());}
 static void verify(Snapshot actual,JSONObject source,Set<String> resources)throws Exception{
  if(source==null||!source.has("active_plugins"))return;
  JSONArray archives=source.getJSONArray("files");Set<String> allowed=new HashSet<>();
  for(int i=0;i<archives.length();i++)allowed.add(archives.getJSONObject(i).getString("runtime_identity"));
  for(String id:actual.identities)if(!allowed.contains(id)&&!resources.contains(id))throw new IOException("外部パックに含まれない原作アーカイブが有効です: "+id);
  List<String> expected=new ArrayList<>();JSONArray plugins=source.getJSONArray("active_plugins");
  for(int i=0;i<plugins.length();i++)expected.add(plugins.getString(i));
  if(!actual.plugins.equals(expected))throw new IOException("有効プラグインまたは優先順位が外部パックと一致しません");
 }
}
