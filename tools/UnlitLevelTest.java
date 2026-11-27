package org.mythvr.standalone;
import java.io.*;import java.nio.file.*;import java.util.*;import org.json.*;
/** Isolated on-device optional asset staging and corruption regression. */
public class UnlitLevelTest {
 public static void main(String[] args)throws Exception{
  ExternalLevelTest.C c=new ExternalLevelTest.C(args[0],args[1]);String[] row=null;
  for(String[] r:LevelAssets.catalog(c))if(r[0].equals("6c312020"))row=r;
  if(row==null)throw new AssertionError("catalog");
  LevelAssets.activate(c,row);LevelAssets.publish(c);
  File staged=new File(LevelAssets.active,"terrain-base.pack");if(!staged.isFile())throw new AssertionError("optional base was not staged");
  try(InputStream in=LevelAssets.open(c,row[0],"terrain-base.pack")){byte[] magic=new byte[8];if(in.read(magic)!=8||!new String(magic,"US-ASCII").equals("MYTHBC01"))throw new AssertionError("base read");}
  File source=new File(LevelAssets.external.get(row[0]),"terrain-base.pack");byte[] original=Files.readAllBytes(source.toPath());
  try{byte[] broken=original.clone();broken[1040]^=1;Files.write(source.toPath(),broken);boolean rejected=false;try{LevelAssets.activate(c,row);}catch(IOException e){rejected=true;}if(!rejected||LevelAssets.active!=null)throw new AssertionError("corrupt base accepted");}finally{Files.write(source.toPath(),original);}
  System.out.println("PASS optional base color: manifest, verified staging, native path, reads, corrupted checksum rejection");
 }
}
