package org.mythvr.standalone;
import java.io.*;import java.util.*;import java.security.*;
public class DetailMixedTest {
 public static void main(String[] args)throws Exception{
  List<DetailCodec.Tag> all=DetailCodec.archive(new File(args[0]));
  if(all.size()!=744)throw new AssertionError("mixed terrain tag count "+all.size());
  if(!DetailCodec.archive(new File(args[1])).isEmpty())throw new AssertionError("sprite-only terrain extraction");
  DetailCodec.Tag t=all.get(0);
  if(DetailTextures.resolve(Arrays.asList(t,t))!=t)throw new AssertionError("identical duplicate");
  byte[] data=t.read();File f=new File("build/detail-test/conflict.fixture");
  try(RandomAccessFile out=new RandomAccessFile(f,"rw")){out.setLength(128+data.length);out.seek(128);out.write(data);out.seek(128);out.write(data[0]^1);}
  byte[] h=new byte[64];java.nio.ByteBuffer b=java.nio.ByteBuffer.wrap(h);b.putInt(36,t.type);b.putInt(40,t.id);b.putInt(44,128);b.putInt(48,data.length);b.putShort(56,(short)t.version);
  DetailCodec.Tag changed=new DetailCodec.Tag(f,h);boolean rejected=false;
  try{DetailTextures.resolve(Arrays.asList(t,changed));}catch(IOException expected){rejected=true;}
  if(!rejected)throw new AssertionError("conflict accepted");
  MessageDigest a=MessageDigest.getInstance("SHA-256"),c=MessageDigest.getInstance("SHA-256");DetailTextures.hashTag(a,t);DetailTextures.hashTag(c,changed);
  if(Arrays.equals(a.digest(),c.digest()))throw new AssertionError("stale cache");
  System.out.println("PASS mixed archive, sprite-only exclusion, duplicate handling, conflict rejection, content invalidation");
 }
}
