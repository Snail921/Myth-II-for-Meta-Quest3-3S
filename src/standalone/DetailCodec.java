package org.mythvr.standalone;

import java.io.*;
import java.util.*;

/** Bounded landscape reader for resource ADDONs, including mixed HD packs. */
final class DetailCodec {
    static int be(byte[] b,int p){return (b[p]&255)<<24|(b[p+1]&255)<<16|(b[p+2]&255)<<8|(b[p+3]&255);}
    static int le(byte[] b,int p){return Integer.reverseBytes(be(b,p));}
    static int us(byte[] b,int p){return (b[p]&255)<<8|(b[p+1]&255);}
    static final class Tag {
        final File file; final int type,id,version; final long offset; final int size;
        Tag(File f,byte[] h){file=f;type=be(h,36);id=be(h,40);offset=Integer.toUnsignedLong(be(h,44));size=be(h,48);version=us(h,56);}
        byte[] read()throws IOException{if(size<0||size>32*1024*1024||offset<128||offset+size>file.length())throw new IOException("detail tag bounds");byte[] b=new byte[size];try(RandomAccessFile f=new RandomAccessFile(file,"r")){f.seek(offset);f.readFully(b);}return b;}
    }
    static List<Tag> archive(File file)throws IOException {
        try(RandomAccessFile f=new RandomAccessFile(file,"r")){
            if(f.length()<128)return Collections.emptyList();byte[] h=new byte[128];f.readFully(h);
            if(be(h,124)!=0x646e6732||us(h,0)!=6||(us(h,2)!=101&&us(h,2)!=102))return Collections.emptyList();
            int entries=us(h,100),count=us(h,102);long start=128L+entries*112L;
            if(count>16384||start+count*64L>f.length())throw new IOException("detail archive table");
            List<Tag> tags=new ArrayList<>();f.seek(start);
            for(int i=0;i<count;i++){byte[] row=new byte[64];f.readFully(row);Tag t=new Tag(file,row);
                // HD collection payloads are indexed but never allocated by the terrain reader.
                // Scenario/gameplay addons still require engine activation binding.
                if(t.type!=0x646d6170&&t.type!=0x64746578&&t.type!=0x64323536)return Collections.emptyList();
                if(be(row,60)!=0x6d746832||t.offset<start+count*64L||t.size<0||t.offset+t.size>f.length())throw new IOException("detail archive entry");
                if(t.type==0x646d6170||t.type==0x64746578)tags.add(t);}
            return tags;
        }
    }
    static byte[] rle(byte[] b,int offset,int length)throws IOException {
        if(offset<0)throw new IOException("detail RLE offset");byte[] out=new byte[length];int p=0;
        while(p<length){if(offset+2>b.length)throw new IOException("detail RLE truncated");byte value=b[offset++];int n=(b[offset++]&255)+1;if(n>length-p)throw new IOException("detail RLE overflow");Arrays.fill(out,p,p+n,value);p+=n;}return out;
    }
    static final class MapData {
        int width,height; byte[] vertices,triangles;int[] ids=new int[128],pixels=new int[128];
        MapData(Tag tag)throws IOException{
            if(tag.version!=2)throw new IOException("Unsupported detail map version "+tag.version);byte[] b=tag.read();if(b.length<256)throw new IOException("detail map header");
            width=be(b,0);height=be(b,4);int entries=be(b,24);
            if(width<1||height<1||width>512||height>512||entries<256||entries+8192>b.length)throw new IOException("detail map bounds");
            vertices=rle(b,be(b,8),width*height);triangles=rle(b,be(b,16),width*height*2);
            for(int i=0;i<128;i++){ids[i]=be(b,entries+i*64);pixels[i]=(b[entries+i*64+4]&255)+1;}
            for(byte[] a:new byte[][]{vertices,triangles})for(byte v:a)if((v&255)>127)throw new IOException("detail material index");
        }
    }
    static final class Bits {
        final byte[] data;int word,position;
        Bits(byte[] b)throws IOException{data=b;word=readWord(0);}
        int readWord(int p)throws IOException{if(p<0||p+4>data.length)throw new IOException("detail bitstream truncated");return le(data,p);}
        // Original 0x4b7eb0: little-endian words, MSB-first, low-first at word crossings.
        int take(int n)throws IOException{
            if(n==0)return 0;int shift=position&31,remaining=32-shift;position+=n;
            if(n<remaining){int result=word>>>(32-n);word<<=n;return result;}
            int result=word>>>shift;word=readWord((position>>>5)*4);
            int extra=n-remaining;if(extra!=0){result|=(word>>>(32-extra))<<remaining;word<<=extra;}return result;
        }
    }
    static int unzig(int v){return (v>>>1)^-(v&1);}
    static int predict(int left,int up,int diagonal){
        if(left<up){if(diagonal<up){if(left<diagonal)return up-diagonal+left;return (up*7+left)>>3;}return (left*7+up)>>3;}
        if(diagonal<left){if(up<diagonal)return up-diagonal+left;return (left*7+up)>>3;}return (up*7+left)>>3;
    }
    static final class Image {int width,height;byte[] rgb;Image(int w,int h,byte[] p){width=w;height=h;rgb=p;}}
    static Image decode(byte[] tag)throws IOException{
        if(tag.length<28)throw new IOException("detail image header");int size=le(tag,16);
        if(size<4||size>tag.length-20)throw new IOException("detail image size");Bits bits=new Bits(Arrays.copyOfRange(tag,20,20+size));
        int w=bits.take(12),h=bits.take(12),mode=bits.take(2);
        if(w<1||h<1||w>2048||h>2048||mode>2)throw new IOException("detail image dimensions/mode");
        int bx=(w+7)/8,by=(h+7)/8;byte[] codes=new byte[bx*by*3];for(int i=0;i<codes.length;i++)codes[i]=(byte)bits.take(3);
        byte[] raw=new byte[w*h*3];
        for(int y=0;y<h;y++)for(int x=0;x<w;x++)for(int c=0;c<3;c++){
            int k=codes[((y/8)*bx+x/8)*3+c];int v=0;
            if(k!=7){int q=0;while(bits.take(1)!=0){if(++q>255)throw new IOException("detail Rice overflow");}v=(q<<k)|bits.take(k);if(v>255)throw new IOException("detail Rice value");}
            raw[(y*w+x)*3+c]=(byte)v;
        }
        byte[] out=new byte[raw.length];System.arraycopy(raw,0,out,0,3);
        for(int y=0;y<h;y++)for(int x=0;x<w;x++){
            if(x==0&&y==0)continue;int p=(y*w+x)*3,primary=y==0?0:mode;
            int second=primary==0?2:primary==1?0:1,third=3-primary-second;
            int a,b,c;
            if(x==0||y==0){int prev=p-(x==0?w*3:3);a=out[prev+primary]&255;b=a-(out[prev+second]&255);c=a-(out[prev+third]&255);}
            else{int l=p-3,u=p-w*3,d=u-3;int lp=out[l+primary]&255,up=out[u+primary]&255,dp=out[d+primary]&255;
                a=predict(lp,up,dp);b=predict(lp-(out[l+second]&255),up-(out[u+second]&255),dp-(out[d+second]&255));c=predict(lp-(out[l+third]&255),up-(out[u+third]&255),dp-(out[d+third]&255));}
            int value=(a-unzig(raw[p]&255))&255;out[p+primary]=(byte)value;out[p+second]=(byte)(value-b+unzig(raw[p+1]&255));out[p+third]=(byte)(value-c+unzig(raw[p+2]&255));
        }
        return new Image(w,h,out);
    }
}
