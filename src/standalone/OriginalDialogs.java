package org.mythvr.standalone;

import android.app.Activity;
import android.util.AtomicFile;
import java.io.*;
import java.lang.reflect.*;
import java.nio.*;
import java.nio.file.Files;

/** Same-UID original UI mirror. Only active while a VR dialog is open. */
public final class OriginalDialogs {
    private static Activity engine;
    private static volatile boolean running;
    private static Object server,window;
    private static int mode,sequence,lastCommand,rx,ry,rw,rh,rootX,rootY;
    private static long openedAt,clickedAt;
    private static int imageHash;
    private static Template[] templates;
    private static File command,frame;
    private static volatile String stage="idle";
    private static volatile long stageAt;
    private static Thread worker;
    private static synchronized void trace(String text){
        android.util.Log.i("MythDialogs",text);
        if(engine!=null)try(FileWriter w=new FileWriter(new File(engine.getExternalFilesDir(null),"dialog-input-trace.log"),true)){w.write(System.currentTimeMillis()+" "+text+"\n");}catch(Exception ignored){}
    }
    private static void stage(String value){stage=value;stageAt=android.os.SystemClock.uptimeMillis();}

    private static Object call(Object o,String method)throws Exception{return o.getClass().getMethod(method).invoke(o);}
    private static int number(Object o,String method)throws Exception{return ((Number)call(o,method)).intValue();}
    private static final class Template {
        int w,h;byte[] rgba;
        Template(ByteBuffer b){w=b.getInt();h=b.getInt();if(w<1||h<1||w>1024||h>512)throw new IllegalArgumentException();rgba=new byte[w*h*4];b.get(rgba);}
    }
    public static synchronized void attach(Activity a){
        engine=a;if(running)return;running=true;
        command=new File(a.getFilesDir(),"dialog.command");frame=new File(a.getFilesDir(),"dialog.rgba");
        // Never replay a command from an earlier native activity/process.
        command.delete();frame.delete();
        worker=new Thread(()->{try{
            Field field=a.getClass().getDeclaredField("xServer");field.setAccessible(true);server=field.get(a);
            ByteArrayOutputStream bytes=new ByteArrayOutputStream();try(InputStream in=SharedAssets.open(a,"dialog-templates.pack")){byte[] block=new byte[8192];for(int n;(n=in.read(block))>0;)bytes.write(block,0,n);}
            ByteBuffer b=ByteBuffer.wrap(bytes.toByteArray()).order(ByteOrder.LITTLE_ENDIAN);templates=new Template[10];for(int i=0;i<templates.length;i++)templates[i]=new Template(b);
            android.util.Log.i("MythDialogs","Ready command="+command);
            while(!a.isDestroyed()){
                try{if(command.isFile()){
                    byte[] data=Files.readAllBytes(command.toPath());if(data.length==20){ByteBuffer q=ByteBuffer.wrap(data).order(ByteOrder.LITTLE_ENDIAN);if(q.getInt()==0x31474c44){int id=q.getInt(),op=q.getInt(),x=q.getInt(),y=q.getInt();if(id!=lastCommand){lastCommand=id;trace("Received seq="+id+" op="+op+" xy="+x+","+y+" mode="+mode);stage("request "+op);request(op,x,y);trace("Request complete seq="+id);}}}
                }
                if(mode!=0){stage("capture/update");update();}stage("idle");
                }catch(Exception e){android.util.Log.e("MythDialogs","Dialog operation failed",e);if(mode!=0)handoff("mirror unavailable");else publish(null,0);}
                Thread.sleep(125);
            }
        }catch(Exception e){android.util.Log.e("MythDialogs","Initialization failed",e);}finally{running=false;}},"MythDialogs");worker.start();
        new Thread(()->{long reported=0;while(running&&!a.isDestroyed()){
            try{Thread.sleep(1000);}catch(InterruptedException e){return;}
            long at=stageAt;
            if(at!=0&&at!=reported&&!stage.equals("idle")&&android.os.SystemClock.uptimeMillis()-at>2500){
                reported=at;StringBuilder report=new StringBuilder("STALL ").append(stage);
                for(StackTraceElement el:worker.getStackTrace())report.append("\n  ").append(el);
                trace(report.toString());
            }
        }},"MythDialogWatch").start();
    }
    private static ByteBuffer capture()throws Exception{
        stage("capture lockAll");Object lock=call(server,"lockAll");try{stage("capture pixels");
            Object manager=server.getClass().getField("windowManager").get(server);
            Object focused=call(manager,"getFocusedWindow");
            if(focused==null)throw new IOException("No original window");
            Object chosen=null;
            // Wine sometimes focuses a child; select its game-sized ancestor.
            for(Object p=focused;p!=null;p=call(p,"getParent")){
                int w=number(p,"getWidth"),h=number(p,"getHeight");
                String name=String.valueOf(call(p,"getName"));
                if(w>=400&&h>=300&&w<=2048&&h<=1536&&name.toLowerCase(java.util.Locale.ROOT).contains("myth")){chosen=p;break;}
            }
            if(chosen==null){
                Object root=focused;for(Object parent;(parent=call(root,"getParent"))!=null;)root=parent;
                java.util.ArrayList<Object> nodes=new java.util.ArrayList<>();nodes.add(root);
                for(int i=0;i<nodes.size()&&i<1024;i++){Object candidate=nodes.get(i);int cw=number(candidate,"getWidth"),ch=number(candidate,"getHeight");
                    if(cw>=400&&ch>=300&&String.valueOf(call(candidate,"getName")).toLowerCase(java.util.Locale.ROOT).contains("myth")){chosen=candidate;break;}
                    nodes.addAll((java.util.List<?>)call(candidate,"getChildren"));
                }
            }
            if(chosen==null)throw new IOException("No Myth game drawable (focus is not the game)");
            window=chosen;int w=number(window,"getWidth"),h=number(window,"getHeight");
            if(w<400||h<300||w>2048||h>1536)throw new IOException("Invalid game surface "+w+"x"+h);
            rootX=number(window,"getRootX");rootY=number(window,"getRootY");
            Object drawable=call(window,"getContent");
            if(call(drawable,"getData")==null)throw new IOException("GPU-only original surface");
            ByteBuffer pixels=(ByteBuffer)drawable.getClass().getMethod("getImage",short.class,short.class,short.class,short.class).invoke(drawable,(short)0,(short)0,(short)w,(short)h);
            ByteBuffer result=ByteBuffer.allocate(8+w*h*4).order(ByteOrder.LITTLE_ENDIAN);result.putInt(w).putInt(h);pixels.rewind();result.put(pixels);result.rewind();return result;
        }finally{((AutoCloseable)lock).close();}
    }
    // Sparse template match on original opaque artwork. No OCR or translated labels.
    private static double error(ByteBuffer image,int w,int h,Template t,int x,int y,float scale){
        int tw=Math.round(t.w*scale),th=Math.round(t.h*scale);if(x<0||y<0||x+tw>w||y+th>h)return 1e9;
        double sum=0;int count=0;
        for(int k=0;k<48;k++){
            int tx=4+(k*37)%(t.w-8),ty;
            // Dialog outer border is stable while text/buttons inside change.
            if(t.w>100)ty=k%2==0?3+(k%4):t.h-4-(k%4);else ty=4+(k*23)%(t.h-8);
            int p=(ty*t.w+tx)*4;if((t.rgba[p+3]&255)<240)continue;
            int q=8+((y+Math.round(ty*scale))*w+x+Math.round(tx*scale))*4;
            for(int c=0;c<3;c++){sum+=Math.abs((t.rgba[p+c]&255)-(image.get(q+2-c)&255));count++;}
        }
        return count==0?1e9:sum/count;
    }
    private static int[] locate(ByteBuffer image,Template t,boolean button){
        int w=image.getInt(0),h=image.getInt(4);double best=1e9;int[] result=null;
        for(float scale:new float[]{1f,.5f,2f}){
            int tw=Math.round(t.w*scale),th=Math.round(t.h*scale);if(tw>w||th>h)continue;
            int x0=button?Math.max(0,w-220):Math.max(0,(w-tw)/2-80),x1=button?w-tw:Math.min(w-tw,(w-tw)/2+80);
            int y0=button?0:Math.max(0,(h-th)/2-80),y1=button?Math.min(85,h-th):Math.min(h-th,(h-th)/2+80);
            for(int y=y0;y<=y1;y++)for(int x=x0;x<=x1;x++){double e=error(image,w,h,t,x,y,scale);if(e<best){best=e;result=new int[]{x,y,tw,th};if(e<.5)return result;}}
        }
        if(result==null||best>18)return null;
        return result;
    }
    private static void click(int x,int y)throws Exception{
        stage("pointer move");trace("Click guest="+x+","+y+" root="+rootX+","+rootY);server.getClass().getMethod("injectPointerMove",int.class,int.class).invoke(server,rootX+x,rootY+y);
        Class<?> cls=Class.forName("com.winlator.xserver.Pointer$Button");Object button=cls.getField("BUTTON_LEFT").get(null);
        stage("pointer press");server.getClass().getMethod("injectPointerButtonPress",cls).invoke(server,button);
        try{Thread.sleep(60);}finally{stage("pointer release");server.getClass().getMethod("injectPointerButtonRelease",cls).invoke(server,button);}
    }
    private static void escape()throws Exception{
        Class<?> cls=Class.forName("com.winlator.xserver.XKeycode");Object key=cls.getField("KEY_ESC").get(null);
        stage("escape press");server.getClass().getMethod("injectKeyPress",cls).invoke(server,key);try{Thread.sleep(60);}finally{stage("escape release");server.getClass().getMethod("injectKeyRelease",cls).invoke(server,key);}
    }
    private static void request(int op,int x,int y)throws Exception{
        // Results share game state 3 and some dialog artwork with Pause Game.
        // Never inject toolbar/cropped-dialog clicks into the result screen.
        if(StandaloneBridge.originalResultScreen()){handoff("original result screen; toolbar disabled");return;}
        if(op==1||op==2){
            if(mode!=0)return;
            ByteBuffer image=capture();int[] button=locate(image,templates[op-1],true);
            if(button==null)for(int i=op==1?4:7;i<(op==1?7:10)&&button==null;i++)button=locate(image,templates[i],true);
            if(button==null){
                File out=new File(engine.getExternalFilesDir(null),"dialog-unmatched.bgra");Files.write(out.toPath(),image.array());
                throw new IOException("Original toolbar button not found; captured "+image.getInt(0)+"x"+image.getInt(4)+" to "+out);
            }
            mode=op;imageHash=0;rw=rh=0;openedAt=System.currentTimeMillis();clickedAt=0;
            click(button[0]+button[2]/2,button[1]+button[3]/2);
            android.util.Log.i("MythDialogs","Open original dialog "+mode);
        }else if(op==3&&mode!=0&&rw>0&&x>=0&&y>=0&&x<=10000&&y<=10000){
            click(rx+Math.min(rw-1,x*rw/10000),ry+Math.min(rh-1,y*rh/10000));clickedAt=System.currentTimeMillis();
        }else if(op==4&&mode!=0){escape();clickedAt=System.currentTimeMillis();}
    }
    private static void publish(ByteBuffer image,int m)throws Exception{
        int w=m==0?0:rw,h=m==0?0:rh;ByteBuffer b=ByteBuffer.allocate(20+w*h*4).order(ByteOrder.LITTLE_ENDIAN);
        b.putInt(0x31474c44).putInt(++sequence).putInt(m).putInt(w).putInt(h);
        if(m!=0){int stride=image.getInt(0);for(int y=0;y<h;y++)for(int x=0;x<w;x++){int p=8+((ry+y)*stride+rx+x)*4;b.put(image.get(p+2)).put(image.get(p+1)).put(image.get(p)).put((byte)255);}}
        int hash=java.util.Arrays.hashCode(java.util.Arrays.copyOfRange(b.array(),8,b.capacity()));if(m!=0&&hash==imageHash)return;imageHash=hash;
        AtomicFile file=new AtomicFile(frame);FileOutputStream stream=file.startWrite();try{stream.write(b.array());file.finishWrite(stream);}catch(Exception e){file.failWrite(stream);throw e;}
    }
    private static void update()throws Exception{
        if(StandaloneBridge.originalResultScreen()){handoff("original result screen; stop dialog mirror");return;}
        ByteBuffer image=capture();stage("locate dialog");Template template=templates[mode+1];
        int[] bounds=rw>0&&error(image,image.getInt(0),image.getInt(4),template,rx,ry,(float)rw/template.w)<18?new int[]{rx,ry,rw,rh}:locate(image,template,false);long now=System.currentTimeMillis();
        if(bounds!=null){stage("publish dialog");rx=bounds[0];ry=bounds[1];rw=bounds[2];rh=bounds[3];publish(image,mode);return;}
        if(now-openedAt<1500||clickedAt!=0&&now-clickedAt<350)return;
        if(StandaloneBridge.dialogGameRunning()){mode=0;publish(null,0);android.util.Log.i("MythDialogs","Original dialog closed; continue VR");}
        else handoff("original dialog action");
    }
    private static void handoff(String reason)throws Exception{trace("Handoff: "+reason);stage("handoff");mode=0;publish(null,0);StandaloneBridge.dialogToPanel(engine,reason);}
}
