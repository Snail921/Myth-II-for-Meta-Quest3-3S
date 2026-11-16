package org.mythvr.standalone;
import java.io.*;import java.util.*;
public class DetailQuestTest {public static void main(String[] args)throws Exception{
long start=System.nanoTime();List<DetailCodec.Tag> tags=DetailCodec.archive(new File(args[0]));Map<Integer,DetailCodec.Tag> images=new HashMap<>();DetailCodec.Tag map=null;int id=(int)Long.parseLong(args[1],16);
for(DetailCodec.Tag t:tags){if(t.type==0x64746578)images.put(t.id,t);else if(t.id==id)map=t;}
if(map==null)throw new IOException("No map");DetailCodec.MapData m=new DetailCodec.MapData(map);File file=new File(args[2]);DetailTextures.build(file,m,images);if(!DetailTextures.validCache(file,m.width,m.height))throw new IOException("cache invalid");System.out.println("PASS Quest decode/cache "+file.length()+" bytes "+((System.nanoTime()-start)/1000000)+"ms");
}}
