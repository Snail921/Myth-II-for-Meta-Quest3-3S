package org.mythvr.standalone;
import java.nio.*;import java.nio.file.*;import java.io.*;
public class CollectionManifestTest {
 static void rejected(byte[] b)throws Exception{try{new CollectionManifest(b);throw new AssertionError("accepted malformed");}catch(IOException expected){}}
 public static void main(String[] a)throws Exception{
  CollectionManifest old=new CollectionManifest(Files.readAllBytes(Paths.get(a[0])));if(old.tagged||old.count<1)throw new AssertionError("legacy");
  ByteBuffer b=ByteBuffer.allocate(76).order(ByteOrder.LITTLE_ENDIAN);b.put("MVRCL002".getBytes("US-ASCII")).putInt(2);
  int[] tags={0x66697265,0x31316366};
  for(int i=0;i<2;i++){b.putInt(32+i);for(int j=0;j<6;j++)b.putInt(j);b.putInt(tags[i]);}
  CollectionManifest m=new CollectionManifest(b.array());if(!m.tagged||m.tag(0)==m.tag(1)||m.canonical(0)==m.canonical(1)||m.field(0,5)!=m.field(1,5))throw new AssertionError("distinct tag binding");
  rejected(new byte[0]);rejected(java.util.Arrays.copyOf(b.array(),75));b.putInt(72,tags[0]);rejected(b.array());b.putInt(72,tags[1]);b.putInt(44,32);rejected(b.array());b.putInt(44,33);b.putInt(8,256);rejected(b.array());
  System.out.println("PASS legacy and tagged manifests, equal metadata/distinct tags, malformed sizes, duplicate tags/slots and count limits");
 }
}
