package org.mythvr.standalone;
import java.io.*;import java.nio.file.*;import java.util.*;import org.json.*;
public class HDIndexTest {public static void main(String[] a)throws Exception{
 long start=System.nanoTime();Map<String,HDCollections.Ref> refs=HDCollections.index(new File(a[0]).listFiles());int[] hits=new int[4];
 if(a.length>1){JSONArray rows=new JSONArray(new String(Files.readAllBytes(Paths.get(a[1])),"UTF-8"));for(int i=0;i<rows.length();i++){JSONArray r=rows.getJSONArray(i);String key=r.getString(2)+":"+r.getString(3)+":"+r.getInt(4)+":"+r.getInt(5);for(int m=0;m<2;m++){HDCollections.Ref ref=refs.get(key+":"+m);if(ref==null)continue;try(RandomAccessFile f=new RandomAccessFile(ref.file,"r")){f.seek(ref.offset);byte[] b=new byte[52];f.readFully(b);if(DetailCodec.us(b,0)!=DetailCodec.us(ref.meta,80)||DetailCodec.us(b,2)!=DetailCodec.us(ref.meta,82))throw new IOException("dimension mismatch "+key);}hits[r.getInt(0)]++;}}}
 System.out.println("PASS HD index="+refs.size()+" matches sprite/model/animated/FX="+Arrays.toString(hits)+" ms="+(System.nanoTime()-start)/1000000);
}}
