package org.mythvr.standalone;
import java.io.*;import java.nio.*;import java.util.*;import org.json.*;
public class ActivePluginsTest {
 static final long BASE=0x10000;static byte[] bytes=new byte[32768];
 static ByteBuffer b=ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN);
 static void setup(){Arrays.fill(bytes,(byte)0);b.putInt(0,0x10100);b.putShort(0x104,(short)32);b.putShort(0x106,(short)408);b.putShort(0x112,(short)3);b.putInt(0x118,0x11000);record(0,3,"base",1);record(1,1,"first",2);record(2,1,"second",3);}
 static void record(int slot,int kind,String name,int crc){int p=0x1000+slot*408;b.putShort(p,(short)1);b.putShort(p+8,(short)kind);Arrays.fill(bytes,p+12,p+44,(byte)0);byte[] n=name.getBytes();System.arraycopy(n,0,bytes,p+12,n.length);b.putInt(p+112,crc);}
 static byte[] read(long address,int size)throws IOException{long p=address-BASE;if(p<0||p+size>bytes.length)throw new IOException("range");return Arrays.copyOfRange(bytes,(int)p,(int)p+size);}
 interface Check{void run()throws Exception;}
 static void rejects(Check c)throws Exception{try{c.run();}catch(IOException expected){return;}throw new AssertionError("accepted invalid state");}
 public static void main(String[] args)throws Exception{
  setup();ActivePlugins.Snapshot actual=ActivePlugins.read(ActivePluginsTest::read,BASE);
  JSONObject source=new JSONObject();JSONArray files=new JSONArray(),plugins=new JSONArray();
  for(String id:actual.identities)files.put(new JSONObject().put("runtime_identity",id));for(String id:actual.plugins)plugins.put(id);
  source.put("files",files).put("active_plugins",plugins);ActivePlugins.verify(actual,source);
  source.put("active_plugins",new JSONArray().put(actual.plugins.get(1)).put(actual.plugins.get(0)));rejects(()->ActivePlugins.verify(actual,source));
  source.put("active_plugins",new JSONArray().put(actual.plugins.get(0)));rejects(()->ActivePlugins.verify(actual,source));
  source.put("active_plugins",plugins).put("files",new JSONArray().put(files.get(0)).put(files.get(1)));rejects(()->ActivePlugins.verify(actual,source));
  b.putShort(0x112,(short)33);rejects(()->ActivePlugins.read(ActivePluginsTest::read,BASE));
  setup();b.putShort(0x106,(short)400);rejects(()->ActivePlugins.read(ActivePluginsTest::read,BASE));
  setup();final int[] captures={0};rejects(()->ActivePlugins.read((address,size)->{if(address==BASE&&++captures[0]==4)b.putInt(0x1000+112,99);return read(address,size);},BASE));
  setup();record(2,1,"first",2);rejects(()->ActivePlugins.read(ActivePluginsTest::read,BASE));
  if(args.length>0){Set<String> addons=ActivePlugins.resourceAddons(new File(args[0]));if(addons.size()!=3)throw new AssertionError("resource count="+addons.size());
   source.put("files",files).put("active_plugins",plugins);setup();ActivePlugins.Snapshot withAddon=ActivePlugins.read(ActivePluginsTest::read,BASE);
   withAddon.identities.addAll(addons);ActivePlugins.verify(withAddon,source,addons);rejects(()->ActivePlugins.verify(withAddon,source));
   withAddon.identities.add("6:unknown:0");rejects(()->ActivePlugins.verify(withAddon,source,addons));
   System.out.println("PASS real resource addon headers and template hash; unknown addons rejected");}
  System.out.println("PASS loaded archive order, missing/extra plugins, reversed order, unknown archive, bounds/stride, concurrent change, duplicate identity");
 }
}
