package org.mythvr.standalone;

import android.content.Context;
import android.util.Log;
import java.io.*;
import java.nio.*;
import java.nio.file.*;
import java.security.MessageDigest;
import java.util.*;

/** Prepare only the active landscape; original plugin stays external to the APK. */
final class DetailTextures {
    static final int CACHE_VERSION=2;
    static File plugins(){return new File("/storage/emulated/0/Download/MythII-Standalone/plugins");}
    static String hex(byte[] bytes){StringBuilder s=new StringBuilder();for(byte b:bytes)s.append(String.format(Locale.ROOT,"%02x",b&255));return s.toString();}
    static void putInt(OutputStream out,int n)throws IOException{out.write(n);out.write(n>>>8);out.write(n>>>16);out.write(n>>>24);}
    static void prepare(Context context,String[] level) {
        File active=new File(context.getFilesDir(),"active-detail.pack");
        // Never reuse a previous level's or removed plugin's active binding.
        if(active.exists()&&!active.delete())throw new IllegalStateException("Cannot clear active detail binding");
        if(!context.getSharedPreferences("vr_graphics",0).getBoolean("detail_textures",true))return;
        try {
            int id=0;boolean found=false;
            try(BufferedReader r=new BufferedReader(new InputStreamReader(LevelAssets.open(context,level[0],"detail-bindings.tsv"),"UTF-8"))){String line;while((line=r.readLine())!=null){String[] v=line.split("\t");if(v.length==2&&v[0].equals(level[0])){id=(int)Long.parseLong(v[1],16);found=true;break;}}}
            if(!found)return;
            File[] files=plugins().listFiles();if(files==null)return;Arrays.sort(files,Comparator.comparing(File::getName));
            Map<Integer,List<DetailCodec.Tag>> maps=new HashMap<>(),candidates=new HashMap<>();int archives=0;
            for(File file:files){if(!file.isFile()||file.getName().startsWith("."))continue;List<DetailCodec.Tag> tags=DetailCodec.archive(file);if(tags.isEmpty())continue;
                archives++;for(DetailCodec.Tag t:tags){Map<Integer,List<DetailCodec.Tag>> table=t.type==0x646d6170?maps:candidates;
                    table.computeIfAbsent(t.id,k->new ArrayList<>()).add(t);}
            }
            DetailCodec.Tag tag=resolve(maps.get(id));if(tag==null)return;
            DetailCodec.MapData map=new DetailCodec.MapData(tag);
            if(map.width!=Integer.parseInt(level[2])||map.height!=Integer.parseInt(level[3]))throw new IOException("detail map dimensions mismatch");
            Map<Integer,DetailCodec.Tag> images=new TreeMap<>();boolean[] used=new boolean[128];
            for(byte[] indices:new byte[][]{map.vertices,map.triangles})for(byte v:indices)used[v&255]=true;
            for(int i=1;i<128;i++)if(used[i]&&map.ids[i]!=0&&map.ids[i]!=-1){DetailCodec.Tag image=resolve(candidates.get(map.ids[i]));
                if(image==null)throw new IOException("Missing detail texture "+Integer.toHexString(map.ids[i]));images.put(image.id,image);}
            // Content-address only this landscape and its dependencies. HD sprites can be gigabytes.
            MessageDigest digest=MessageDigest.getInstance("SHA-256");hashTag(digest,tag);
            for(DetailCodec.Tag image:images.values())hashTag(digest,image);
            File dir=new File(context.getCacheDir(),"landscape-details");if(!dir.isDirectory()&&!dir.mkdirs())throw new IOException("detail cache directory");
            String key=CACHE_VERSION+"-"+level[0]+"-"+hex(digest.digest());File cached=new File(dir,key+".pack");
            boolean hit=validCache(cached,Integer.parseInt(level[2]),Integer.parseInt(level[3]));if(!hit){
                if(map.width!=Integer.parseInt(level[2])||map.height!=Integer.parseInt(level[3]))throw new IOException("detail map dimensions mismatch");
                File temp=new File(dir,key+".tmp");try{build(temp,map,images);Files.move(temp.toPath(),cached.toPath(),StandardCopyOption.REPLACE_EXISTING);}finally{temp.delete();}}
            File temp=new File(context.getFilesDir(),"active-detail.tmp");Files.copy(cached.toPath(),temp.toPath(),StandardCopyOption.REPLACE_EXISTING);Files.move(temp.toPath(),active.toPath(),StandardCopyOption.REPLACE_EXISTING);
            // Bound disk growth; active copy is independent of evicted caches.
            File[] old=dir.listFiles();if(old!=null){Arrays.sort(old,Comparator.comparingLong(File::lastModified));long total=0;for(File f:old)total+=f.length();for(File f:old)if(total>256L*1024*1024&&!f.equals(cached)){long size=f.length();if(f.delete())total-=size;}}
            Log.i("MythStandalone","Detail textures prepared level="+level[8]+" addons="+archives+" cache="+hit+" bytes="+active.length());
        }catch(Exception e){active.delete();Log.e("MythStandalone","Detail textures unavailable; original terrain retained",e);}
    }
    static DetailCodec.Tag resolve(List<DetailCodec.Tag> candidates)throws IOException{
        if(candidates==null||candidates.isEmpty())return null;DetailCodec.Tag chosen=candidates.get(0);
        for(int i=1;i<candidates.size();i++){DetailCodec.Tag other=candidates.get(i);
            if(chosen.version!=other.version||!Arrays.equals(chosen.read(),other.read()))
                throw new IOException("Conflicting detail addons: "+chosen.file.getName()+" / "+other.file.getName());}
        return chosen;
    }
    static void hashTag(MessageDigest digest,DetailCodec.Tag tag)throws IOException{
        ByteArrayOutputStream header=new ByteArrayOutputStream();putInt(header,tag.type);putInt(header,tag.id);putInt(header,tag.version);putInt(header,tag.size);
        digest.update(header.toByteArray());digest.update(tag.read());
    }
    static boolean validCache(File file,int w,int h){
        try(RandomAccessFile f=new RandomAccessFile(file,"r")){byte[] b=new byte[24];f.readFully(b);int count=DetailCodec.le(b,16),edge=DetailCodec.le(b,20);
            return new String(b,0,8,"US-ASCII").equals("MYTHDT01")&&DetailCodec.le(b,8)==w&&DetailCodec.le(b,12)==h&&count>0&&count<=128&&edge>0&&edge<=2048&&(edge&(edge-1))==0&&f.length()==24L+w*h*3L+count*8L+(long)edge*edge*4*count;
        }catch(IOException e){return false;}
    }
    static void build(File target,DetailCodec.MapData map,Map<Integer,DetailCodec.Tag> tags)throws IOException {
        boolean[] used=new boolean[128];for(byte[] a:new byte[][]{map.vertices,map.triangles})for(byte b:a)used[b&255]=true;
        int[] remap=new int[128];Arrays.fill(remap,0);List<DetailCodec.Image> images=new ArrayList<>();List<Integer> pixels=new ArrayList<>();int size=1;
        images.add(new DetailCodec.Image(1,1,new byte[]{(byte)128,(byte)128,(byte)128}));pixels.add(1);
        for(int i=1;i<128;i++)if(used[i]&&map.ids[i]!=-1&&map.ids[i]!=0){DetailCodec.Tag t=tags.get(map.ids[i]);if(t==null)throw new IOException("Missing detail texture "+Integer.toHexString(map.ids[i]));
            if(t.version!=1)throw new IOException("Unsupported detail texture version");DetailCodec.Image im=DetailCodec.decode(t.read());remap[i]=images.size();images.add(im);pixels.add(map.pixels[i]);size=Math.max(size,Math.max(im.width,im.height));}
        // Mipmapped array, at most 64 MiB including mipmaps. Size reduction only under pressure.
        int edge=1;while(edge<size)edge*=2;while((long)edge*edge*4*images.size()*4/3>64L*1024*1024)edge/=2;
        try(OutputStream out=new BufferedOutputStream(new FileOutputStream(target),262144)){
            out.write("MYTHDT01".getBytes("US-ASCII"));for(int value:new int[]{map.width,map.height,images.size(),edge})putInt(out,value);
            for(byte[] a:new byte[][]{map.vertices,map.triangles})for(byte b:a)out.write(remap[b&255]);
            for(int i=0;i<images.size();i++){DetailCodec.Image im=images.get(i);putInt(out,Float.floatToIntBits(pixels.get(i)/(float)im.width));putInt(out,Float.floatToIntBits(pixels.get(i)/(float)im.height));}
            byte[] row=new byte[edge*4];
            for(DetailCodec.Image im:images)for(int y=0;y<edge;y++){
                float sy=(y+.5f)*im.height/edge-.5f;int y0=(int)Math.floor(sy);float fy=sy-y0;
                for(int x=0;x<edge;x++){float sx=(x+.5f)*im.width/edge-.5f;int x0=(int)Math.floor(sx);float fx=sx-x0;
                    for(int c=0;c<3;c++){float v=0;for(int dy=0;dy<2;dy++)for(int dx=0;dx<2;dx++){int xx=Math.floorMod(x0+dx,im.width),yy=Math.floorMod(y0+dy,im.height);v+=(im.rgb[(yy*im.width+xx)*3+c]&255)*(dx==0?1-fx:fx)*(dy==0?1-fy:fy);}row[x*4+c]=(byte)Math.round(v);}row[x*4+3]=(byte)255;}
                out.write(row);
            }
        }
    }
}
