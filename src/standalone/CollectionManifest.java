package org.mythvr.standalone;
import java.io.IOException;
import java.nio.*;
import java.util.*;
/** V1 metadata binding remains readable; V2 requires original tag IDs. */
final class CollectionManifest {
 final ByteBuffer data;final int count,header,stride;final boolean tagged;
 CollectionManifest(byte[] bytes)throws IOException{
  data=ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN);
  tagged=bytes.length>=8&&Arrays.equals(Arrays.copyOf(bytes,8),new byte[]{'M','V','R','C','L','0','0','2'});
  header=tagged?12:4;stride=tagged?32:28;
  if(bytes.length<header)throw new IOException("short collection manifest");
  count=data.getInt(header-4);
  if(count<1||count>255||bytes.length!=header+count*stride)throw new IOException("collection manifest size");
  Set<Integer> slots=new HashSet<>(),tags=new HashSet<>();
  for(int i=0;i<count;i++){
   int slot=canonical(i);
   if(slot<0||slot>=255||!slots.add(slot))throw new IOException("duplicate/invalid canonical slot");
   if(tagged&&(tag(i)==-1||!tags.add(tag(i))))throw new IOException("duplicate/invalid collection tag");
  }
 }
 int canonical(int i){return data.getInt(header+i*stride);}
 int tag(int i){return data.getInt(header+i*stride+28);}
 int field(int i,int j){return data.getInt(header+i*stride+4+j*4);}
}
