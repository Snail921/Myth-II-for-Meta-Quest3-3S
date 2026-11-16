import java.lang.reflect.*;
import java.nio.*;
import java.nio.file.*;
public class TestOriginalDialogs {
 public static void main(String[] args)throws Exception{
  Class<?> cls=Class.forName("org.mythvr.standalone.OriginalDialogs"),tc=Class.forName("org.mythvr.standalone.OriginalDialogs$Template");
  Constructor<?> ctor=tc.getDeclaredConstructor(ByteBuffer.class);ctor.setAccessible(true);
  Method locate=cls.getDeclaredMethod("locate",ByteBuffer.class,tc,boolean.class);locate.setAccessible(true);
  ByteBuffer pack=ByteBuffer.wrap(Files.readAllBytes(Paths.get("build/vr/assets/dialog-templates.pack"))).order(ByteOrder.LITTLE_ENDIAN);
  for(int i=0;i<10;i++){
   int w=pack.getInt(pack.position()),h=pack.getInt(pack.position()+4);byte[] rgba=new byte[w*h*4];int pos=pack.position();Object t=ctor.newInstance(pack);System.arraycopy(pack.array(),pos+8,rgba,0,rgba.length);
   for(int offset=0;offset<2;offset++){
   int sw=640,sh=480,x=((i<2||i>=4)?sw-w-10:(sw-w)/2)+offset,y=((i<2||i>=4)?8:(sh-h)/2)+offset;
   ByteBuffer image=ByteBuffer.allocate(8+sw*sh*4).order(ByteOrder.LITTLE_ENDIAN);image.putInt(sw).putInt(sh);
   for(int yy=0;yy<h;yy++)for(int xx=0;xx<w;xx++){int p=(yy*w+xx)*4,q=8+((yy+y)*sw+xx+x)*4;for(int c=0;c<3;c++)image.put(q+c,rgba[p+2-c]);}
   int[] found=(int[])locate.invoke(null,image,t,i<2||i>=4);
   if(found==null||Math.abs(found[0]-x)>2||Math.abs(found[1]-y)>2)throw new AssertionError("match "+i+" "+java.util.Arrays.toString(found));
   ByteBuffer blank=ByteBuffer.allocate(image.capacity()).order(ByteOrder.LITTLE_ENDIAN);blank.putInt(sw).putInt(sh);
   if(locate.invoke(null,blank,t,i<2||i>=4)!=null)throw new AssertionError("false positive "+i);
   }
  }
  System.out.println("PASS original toolbar/dialog matching and blank-frame rejection");
 }
}
