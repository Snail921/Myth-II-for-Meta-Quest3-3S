package org.mythvr.standalone;
import java.io.*;import java.util.*;import java.nio.file.*;
public class DetailAudit { public static void main(String[] args)throws Exception{
List<DetailCodec.Tag> tags=DetailCodec.archive(new File(args[0]));Map<Integer,DetailCodec.Tag> images=new HashMap<>();int maps=0,decoded=0;long pixels=0;
for(DetailCodec.Tag t:tags)if(t.type==0x64746578){DetailCodec.Image im=DetailCodec.decode(t.read());images.put(t.id,t);decoded++;pixels+=(long)im.width*im.height;}
for(DetailCodec.Tag t:tags)if(t.type==0x646d6170){DetailCodec.MapData map=new DetailCodec.MapData(t);for(byte[] indices:new byte[][]{map.vertices,map.triangles})for(byte v:indices){int i=v&255;if(i!=0&&map.ids[i]!=0&&map.ids[i]!=-1&&!images.containsKey(map.ids[i]))throw new IOException("missing texture");}maps++;}
System.out.println("PASS maps="+maps+" images="+decoded+" pixels="+pixels);
for(String line:Files.readAllLines(Paths.get("build/vr/assets/levels/detail-bindings.tsv"))){String[] s=line.split("\t");int id=(int)Long.parseLong(s[1],16);for(DetailCodec.Tag t:tags)if(t.type==0x646d6170&&t.id==id){DetailCodec.MapData map=new DetailCodec.MapData(t);File out=new File("build/detail-test/"+s[0]+".pack");DetailTextures.build(out,map,images);System.out.println(s[0]+" "+out.length());}}
}}
