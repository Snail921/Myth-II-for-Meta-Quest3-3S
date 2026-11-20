package org.mythvr.standalone;

import android.app.Activity;
import android.os.SystemClock;
import android.view.MotionEvent;
import android.view.KeyEvent;
import java.io.*;
import java.lang.reflect.*;
import java.util.concurrent.ArrayBlockingQueue;

/** Diagnostic-only observation of Android and X pointer events; never injects input. */
public final class IconInputDiagnostics {
    private static IconInputDiagnostics active;
    private final Activity activity;
    private final Object server,pointer,listener;
    private final Class<?> listenerType;
    private final ArrayBlockingQueue<String> events=new ArrayBlockingQueue<>(512);
    private volatile boolean enabled,stopped;
    private long lastAndroidMove,lastGuestMove;
    private final java.util.concurrent.atomic.AtomicInteger lost=new java.util.concurrent.atomic.AtomicInteger();
    private static Object call(Object o,String name)throws Exception{return o.getClass().getMethod(name).invoke(o);}
    public static void attach(Activity a){a.runOnUiThread(()->{try{
        if(active!=null&&active.activity==a)return;
        if(active!=null)active.stop();active=new IconInputDiagnostics(a);
    }catch(Exception e){android.util.Log.e("MythIconInput","Observer attach failed",e);}});}
    private IconInputDiagnostics(Activity a)throws Exception{
        activity=a;Field f=a.getClass().getDeclaredField("xServer");f.setAccessible(true);server=f.get(a);
        pointer=server.getClass().getField("pointer").get(server);listenerType=Class.forName("com.winlator.xserver.Pointer$OnPointerMotionListener");
        listener=Proxy.newProxyInstance(listenerType.getClassLoader(),new Class<?>[]{listenerType},(proxy,method,args)->{
            String name=method.getName();
            if(name.equals("equals"))return proxy==args[0];if(name.equals("hashCode"))return System.identityHashCode(proxy);if(name.equals("toString"))return "IconInputObserver";
            if(!enabled||stopped)return null;
            long now=SystemClock.elapsedRealtime();
            if(name.equals("onPointerMove")){if(now-lastGuestMove<20)return null;lastGuestMove=now;}
            String value="guest "+name;
            if(name.equals("onPointerMove"))value+=" xy="+args[0]+","+args[1];
            else if(name.startsWith("onPointerButton"))try{value+=" button="+args[0]+" xy="+call(pointer,"getX")+","+call(pointer,"getY");}catch(Exception e){value+=" coordinates-unavailable";}
            record(value);return null;
        });
        Object lock=call(server,"lockAll");try{pointer.getClass().getMethod("addOnPointerMotionListener",listenerType).invoke(pointer,listener);}finally{((AutoCloseable)lock).close();}
        new Thread(this::watch,"MythIconInputProbe").start();
    }
    private void record(String text){if(!events.offer("mono="+SystemClock.elapsedRealtime()+" "+text))lost.incrementAndGet();}
    public static void motion(Activity a,String route,MotionEvent e){
        IconInputDiagnostics p=active;if(p==null||p.activity!=a||!p.enabled||p.stopped)return;
        // Keyboard coordinates could reveal typed characters; never record these events.
        if(RenameKeyboard.ownsMotion(a,e))return;
        int action=e.getActionMasked();long now=SystemClock.elapsedRealtime();
        if(action==MotionEvent.ACTION_MOVE||action==MotionEvent.ACTION_HOVER_MOVE){if(now-p.lastAndroidMove<20)return;p.lastAndroidMove=now;}
        p.record("android "+route+" action="+action+" source="+e.getSource()+" device="+e.getDeviceId()+" buttons="+e.getButtonState()+" xy="+e.getX()+","+e.getY()+" eventUptime="+e.getEventTime());
    }
    public static void key(Activity a,KeyEvent e){
        IconInputDiagnostics p=active;if(p==null||p.activity!=a||!p.enabled||p.stopped)return;
        // Do not log text-producing keys or Unicode values.
        if(e.getKeyCode()==KeyEvent.KEYCODE_BUTTON_R2||e.getKeyCode()==KeyEvent.KEYCODE_BUTTON_L2)
            p.record("android trigger key="+e.getKeyCode()+" action="+e.getAction()+" eventUptime="+e.getEventTime());
    }
    private void watch(){
        int pid=-1;long scan=0;String last="";boolean was=false;
        while(!stopped&&!activity.isDestroyed()){
            enabled=activity.getSharedPreferences("vr_graphics",0).getBoolean("diagnostic_logging",false)&&activity.hasWindowFocus();
            if(enabled){
                if(!was){record("probe=v1 icon_interval_ms=20 move_throttle_ms=20 metadata-only");last="";scan=0;}
                long now=SystemClock.elapsedRealtime();if(now>=scan){pid=StandaloneBridge.findGame();scan=now+2000;}
                String state=TextInputDiagnostics.iconState(pid);if(!state.equals(last)){record("original "+state);last=state;}
            }else if(was)record("probe-inactive");
            flush();was=enabled;
            try{Thread.sleep(enabled?20:500);}catch(InterruptedException e){break;}
        }
        activity.runOnUiThread(this::stop);flush();
    }
    private void flush(){
        if(events.isEmpty()&&lost.get()==0)return;
        File dir=activity.getExternalFilesDir(null);if(dir==null){events.clear();return;}
        File file=new File(dir,"icon-input-diagnostic.log"),old=new File(dir,"icon-input-diagnostic.previous.log");
        try{
            if(file.length()>4*1024*1024){if(old.exists()&&!old.delete())return;if(!file.renameTo(old))return;}
            try(FileWriter out=new FileWriter(file,true)){
                int count=lost.getAndSet(0);if(count>0)out.write("mono="+SystemClock.elapsedRealtime()+" dropped="+count+"\n");
                for(int n=0;n<512;n++){String line=events.poll();if(line==null)break;out.write(line+"\n");}
            }
        }catch(IOException e){android.util.Log.w("MythIconInput","Log write failed");}
    }
    private void stop(){if(stopped)return;stopped=true;enabled=false;
        try{Object lock=call(server,"lockAll");try{((java.util.List<?>)PanelKeyboard.pointerListeners(pointer).get(pointer)).remove(listener);}finally{((AutoCloseable)lock).close();}}catch(Exception e){android.util.Log.w("MythIconInput","Observer cleanup failed");}
    }
}
