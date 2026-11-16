package org.mythvr.standalone;
import android.content.Context;import android.util.Log;import org.json.*;
import java.io.*;import java.nio.file.*;import java.util.*;
/** Index external HD images without reading their multi-gigabyte pixel payloads. */
final class HDCollections {
 static final class Ref {File file;long offset;int size;byte[] meta,hues;int priority;}
 static String name(byte[] b,int off,int n)throws Exception{int end=off;while(end<off+n&&b[end]!=0)end++;return new String(b,off,end-off,"MacRoman").toLowerCase(Locale.ROOT);}
 static Map<String,Ref> index(File[] files)throws Exception{
  Map<String,Ref> result=new HashMap<>();Arrays.sort(files,Comparator.comparing(File::getName));
  for(File file:files){if(!file.isFile()||file.getName().startsWith("."))continue;
   try(RandomAccessFile f=new RandomAccessFile(file,"r")){
    if(f.length()<128)continue;byte[] h=new byte[128];f.readFully(h);int version=DetailCodec.us(h,2);
    if(DetailCodec.be(h,124)!=0x646e6732||DetailCodec.us(h,0)!=6||(version!=101&&version!=102))continue;
    int count=DetailCodec.us(h,102);long table=128L+DetailCodec.us(h,100)*112L;
    if(table+count*64L>f.length())throw new IOException("HD archive table");
    for(int i=0;i<count;i++){f.seek(table+i*64L);byte[] t=new byte[64];f.readFully(t);if(DetailCodec.be(t,36)!=0x64323536)continue;
     long off=Integer.toUnsignedLong(DetailCodec.be(t,44)),size=Integer.toUnsignedLong(DetailCodec.be(t,48));
     if(off<table+count*64L||size<64||off+size>f.length())throw new IOException("HD collection bounds");
     f.seek(off);byte[] head=new byte[64];f.readFully(head);int n=DetailCodec.be(head,8),rp=DetailCodec.be(head,12),nh=DetailCodec.be(head,52),hp=DetailCodec.be(head,56);
     if(n<0||n>65536||rp<0||64L+rp+n*128L>size||nh<0||nh>8||hp<0||(nh>0&&64L+hp+nh*128L>size))throw new IOException("HD reference bounds");
     byte[] hues=new byte[nh*128];f.seek(off+64+hp);f.readFully(hues);
     String tag=String.format(Locale.ROOT,"%08x",DetailCodec.be(head,4));
     for(int r=0;r<n;r++){f.seek(off+64+rp+r*128L);byte[] meta=new byte[128];f.readFully(meta);int len=DetailCodec.be(meta,68);long pos=Integer.toUnsignedLong(DetailCodec.be(meta,64));
      if(len<52||len>32*1024*1024||64+pos+len>size)throw new IOException("HD bitmap bounds");
      Ref ref=new Ref();ref.file=file;ref.offset=off+64+pos;ref.size=len;ref.meta=meta;ref.hues=hues;ref.priority=version;
      String key=tag+":"+Long.toString(Integer.toUnsignedLong(DetailCodec.be(meta,84)))+":"+DetailCodec.us(meta,100)+":"+DetailCodec.us(meta,102)+":"+(DetailCodec.be(meta,104)&1);
      Ref old=result.get(key);if(old==null||old.priority<version)result.put(key,ref);
      // Original first matching reference wins within the same priority.
     }
    }
   }
  }return result;
 }
 static void prepare(Context c,String[] level){
  File target=new File(c.getFilesDir(),"active-hd.pack"),temp=new File(c.getFilesDir(),"active-hd.tmp");target.delete();
  try(OutputStream out=new FileOutputStream(new File(c.getFilesDir(),"detail-shadow-setting.bin"))){DetailTextures.putInt(out,c.getSharedPreferences("vr_graphics",0).getBoolean("detail_shadows",true)?1:0);}catch(IOException e){Log.e("MythStandalone","Shadow setting",e);}
  if(!c.getSharedPreferences("vr_graphics",0).getBoolean("hires_sprites",true))return;
  try{
   File[] files=DetailTextures.plugins().listFiles();if(files==null)return;Map<String,Ref> refs=index(files);
   ByteArrayOutputStream input=new ByteArrayOutputStream();try(InputStream in=LevelAssets.open(c,level[0],"hd-bindings.json")){byte[] b=new byte[16384];int n;while((n=in.read(b))>0)input.write(b,0,n);}
   JSONArray rows=new JSONArray(new String(input.toByteArray(),"UTF-8"));ByteArrayOutputStream records=new ByteArrayOutputStream();int matched=0;
   for(int i=0;i<rows.length();i++){JSONArray row=rows.getJSONArray(i);String key=row.getString(2)+":"+row.getString(3).toLowerCase(Locale.ROOT)+":"+row.getInt(4)+":"+row.getInt(5);for(int mirror=0;mirror<2;mirror++){Ref r=refs.get(key+":"+mirror);if(r==null)continue;
    DetailTextures.putInt(records,row.getInt(0));DetailTextures.putInt(records,row.getInt(1)|(mirror<<30));byte[] path=r.file.getAbsolutePath().getBytes("UTF-8");DetailTextures.putInt(records,path.length);records.write(path);
    DetailTextures.putInt(records,(int)r.offset);DetailTextures.putInt(records,(int)(r.offset>>>32));DetailTextures.putInt(records,r.size);records.write(r.meta);DetailTextures.putInt(records,r.hues.length);records.write(r.hues);JSONArray names=row.getJSONArray(6);for(int hi=0;hi<8;hi++){int source=-1;if(hi<r.hues.length/128){String hue=name(r.hues,hi*128,64);for(int j=0;j<names.length();j++)if(hue.equals(names.getString(j)))source=j;}DetailTextures.putInt(records,source);}matched++;}
   }
   try(OutputStream out=new FileOutputStream(temp)){out.write("MYTHHD02".getBytes("US-ASCII"));DetailTextures.putInt(out,matched);try(InputStream tint=LevelAssets.open(c,level[0],"hd-tint.bin")){byte[] b=new byte[16];new DataInputStream(tint).readFully(b);out.write(b);}records.writeTo(out);}
   Files.move(temp.toPath(),target.toPath(),StandardCopyOption.REPLACE_EXISTING);Log.i("MythStandalone","HD bindings level="+level[8]+" matched="+matched+" indexed="+refs.size());
  }catch(Exception e){target.delete();temp.delete();Log.e("MythStandalone","HD unavailable; retaining original images",e);}
 }
}
