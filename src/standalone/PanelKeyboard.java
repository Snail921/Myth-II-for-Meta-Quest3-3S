package org.mythvr.standalone;

import android.app.Activity;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import android.view.KeyEvent;
import android.widget.*;
import java.lang.reflect.Method;
import java.lang.reflect.Field;
import java.util.ArrayList;

/** Explicit text entry for the guest's currently focused single-line field. */
public final class PanelKeyboard {
    private static final String TAG="MythPanelKeyboard";
    private static final Handler MAIN=new Handler(Looper.getMainLooper());
    static final class BackPress {
        boolean held,fired;long started;
        boolean down(long now){if(held)return false;held=true;fired=false;started=now;return true;}
        boolean fire(long now){if(!held||fired||now-started<1000)return false;fired=true;return true;}
        // Open at the threshold; consume the matching release without reopening.
        // 0: canceled/unmatched; 1: short; 2: long.
        int up(long now,boolean canceled){int result=!held||canceled?0:fired?0:now-started>=1000?2:1;held=false;fired=false;return result;}
    }
    private static final BackPress backPress=new BackPress();
    private static Runnable backTimer;
    private static void toggle(Activity a){
        RenameKeyboard.open(a);
    }
    private static boolean inputFocused(Activity a){return !a.isFinishing()&&!a.isDestroyed()&&a.hasWindowFocus();}
    private static boolean backKey(Activity a,KeyEvent e){
        if(e.getAction()==KeyEvent.ACTION_DOWN&&e.getRepeatCount()==0&&backPress.down(e.getEventTime())){
            backTimer=()->{if(inputFocused(a)&&backPress.fire(SystemClock.uptimeMillis())){android.util.Log.i(TAG,"Y/B 1s hold: keyboard show");toggle(a);}};
            MAIN.postAtTime(backTimer,backPress.started+1000);
        }else if(e.getAction()==KeyEvent.ACTION_UP){
            if(backTimer!=null)MAIN.removeCallbacks(backTimer);backTimer=null;
            int action=backPress.up(e.getEventTime(),e.isCanceled()||!inputFocused(a));
            if(action==1){
                android.util.Log.i(TAG,"Y/B short press: original drawer");a.onBackPressed();
            }
        }
        return true;
    }
    private static long diagnosticUntil,lastMotionLog;
    static void traceKey(String route,KeyEvent e){
        if(SystemClock.uptimeMillis()>diagnosticUntil)return;
        android.util.Log.i("MythPanelInput",route+" key="+KeyEvent.keyCodeToString(e.getKeyCode())+" scan="+e.getScanCode()+" action="+e.getAction()+" repeat="+e.getRepeatCount()+" source="+e.getSource()+" device="+e.getDeviceId()+" time="+e.getEventTime());
    }
    static void traceMotion(android.view.MotionEvent e){
        long now=SystemClock.uptimeMillis();if(now>diagnosticUntil)return;
        int action=e.getActionMasked();
        boolean moving=action==android.view.MotionEvent.ACTION_MOVE||action==android.view.MotionEvent.ACTION_HOVER_MOVE;
        if(moving&&now-lastMotionLog<500)return;lastMotionLog=now;
        android.util.Log.i("MythPanelInput","motion action="+action+" source="+e.getSource()+" device="+e.getDeviceId()+" buttons="+e.getButtonState()+" x="+e.getAxisValue(0)+" y="+e.getAxisValue(1)+" z="+e.getAxisValue(11)+" rz="+e.getAxisValue(14)+" lt="+e.getAxisValue(17)+" rt="+e.getAxisValue(18));
    }
    // Listen to guest pointer releases, not Android touch-up: TouchpadView delays release.
    private static ShiftHold shiftHold;
    static final class OneClickShift {
        boolean down;
        boolean event(String method,String button){
            if(!"BUTTON_LEFT".equals(button))return false;
            if("onPointerButtonPress".equals(method))down=true;
            if("onPointerButtonRelease".equals(method)&&down){down=false;return true;}
            return false;
        }
    }
    static Field pointerListeners(Object pointer)throws Exception{
        Field field=pointer.getClass().getDeclaredField("onPointerMotionListeners");field.setAccessible(true);return field;
    }
    private static final class ShiftHold {
        final Activity activity;final Object target,pointer,listener;final Method deliver;final Field listeners;
        final OneClickShift click=new OneClickShift();boolean released;
        ShiftHold(Activity a)throws Exception{
            activity=a;target=keyboard(a);deliver=target.getClass().getMethod("onKeyEvent",KeyEvent.class);
            Field field=a.getClass().getDeclaredField("xServer");field.setAccessible(true);Object server=field.get(a);
            pointer=server.getClass().getField("pointer").get(server);
            Class<?> type=Class.forName("com.winlator.xserver.Pointer$OnPointerMotionListener");
            listeners=pointerListeners(pointer);
            listener=java.lang.reflect.Proxy.newProxyInstance(type.getClassLoader(),new Class<?>[]{type},(proxy,method,args)->{
                if(method.getName().equals("equals"))return proxy==args[0];
                if(method.getName().equals("hashCode"))return System.identityHashCode(proxy);
                if(method.getName().equals("toString"))return "OneClickShift";
                if(args!=null&&args.length==1){
                    String event=method.getName(),button=String.valueOf(args[0]);
                    if(event.startsWith("onPointerButton"))shiftLog(a,event+" "+button);
                    if(click.event(event,button))MAIN.postDelayed(()->releaseShift(this),250);
                }
                return null;
            });
            pointer.getClass().getMethod("addOnPointerMotionListener",type).invoke(pointer,listener);
        }
    }
    private static void releaseShift(ShiftHold hold){
        if(hold==null||hold.released)return;
        try{long now=SystemClock.uptimeMillis();hold.deliver.invoke(hold.target,new KeyEvent(now,now,KeyEvent.ACTION_UP,KeyEvent.KEYCODE_SHIFT_LEFT,0));}
        catch(Exception e){android.util.Log.e(TAG,"Shift release failed",e);}
        try{((java.util.List<?>)hold.listeners.get(hold.pointer)).remove(hold.listener);}catch(Exception e){android.util.Log.e(TAG,"Shift listener cleanup",e);}
        hold.released=true;if(shiftHold==hold)shiftHold=null;shiftLog(hold.activity,"One-click Shift released");
    }
    private static int shiftRequest;
    private static void shiftLog(Activity a,String message){
        android.util.Log.i(TAG,message);
        try{java.io.File dir=a.getExternalFilesDir(null);if(dir!=null)try(java.io.FileWriter out=new java.io.FileWriter(new java.io.File(dir,"shift-hold.log"),true)){out.write(System.currentTimeMillis()+" "+message+"\n");}}catch(Exception ignored){}
    }
    private static void armShift(Activity a){int request=++shiftRequest;shiftLog(a,"Shift requested");awaitShiftFocus(a,request,0);}
    static void customShift(Activity a,boolean hold){if(hold)armShift(a);else{++shiftRequest;releaseShift(shiftHold);}}
    private static void awaitShiftFocus(Activity a,int request,int attempt){
        if(request!=shiftRequest||a.isFinishing()||a.isDestroyed()){shiftLog(a,"Shift request canceled by lifecycle/new request");return;}
        if(!a.hasWindowFocus()){
            if(attempt<30){MAIN.postDelayed(()->awaitShiftFocus(a,request,attempt+1),100);return;}
            shiftLog(a,"Shift canceled: focus did not return");Toast.makeText(a,"原作画面にフォーカスが戻らず、Shiftを保持できませんでした",Toast.LENGTH_LONG).show();return;
        }
        releaseShift(shiftHold);
        try{
            ShiftHold hold=new ShiftHold(a);shiftHold=hold;long now=SystemClock.uptimeMillis();
            if(!Boolean.TRUE.equals(hold.deliver.invoke(hold.target,new KeyEvent(now,now,KeyEvent.ACTION_DOWN,KeyEvent.KEYCODE_SHIFT_LEFT,0))))throw new IllegalStateException("Shift rejected");
            MAIN.postDelayed(()->releaseShift(hold),30000);
            Toast.makeText(a,"Shift保持中：New Gameをクリックしてください（クリック後に自動解除）",Toast.LENGTH_LONG).show();
            shiftLog(a,"One-click Shift armed; focus="+a.hasWindowFocus());
        }catch(Exception e){releaseShift(shiftHold);shiftLog(a,"Shift arm failed: "+e);android.util.Log.e(TAG,"Shift arm failed",e);Toast.makeText(a,"Shiftを送信できませんでした",Toast.LENGTH_LONG).show();}
    }
    private static final java.util.WeakHashMap<Activity,Boolean> attached=new java.util.WeakHashMap<>();
    private static int dp(Activity a,int n){return Math.round(n*a.getResources().getDisplayMetrics().density);}
    public static boolean menuKey(Activity a,KeyEvent e){
        int code=e.getKeyCode();
        if(code==KeyEvent.KEYCODE_BACK)return backKey(a,e);
        if(code!=KeyEvent.KEYCODE_MENU&&code!=KeyEvent.KEYCODE_BUTTON_START&&e.getScanCode()!=315)return false;
        if(e.getAction()==KeyEvent.ACTION_DOWN&&e.getRepeatCount()==0){
            android.util.Log.i(TAG,"Menu key text toggle code="+code);
            toggle(a);
        }
        return true;
    }
    public static void attach(Activity a){
        if(attached.containsKey(a))return;
        // Full event tracing is disabled outside the input diagnostic build.
        diagnosticUntil=0;
        android.view.Window.Callback previous=a.getWindow().getCallback();
        android.view.Window.Callback callback=(android.view.Window.Callback)java.lang.reflect.Proxy.newProxyInstance(
            PanelKeyboard.class.getClassLoader(),new Class<?>[]{android.view.Window.Callback.class},(proxy,method,args)->{
                if(method.getName().equals("dispatchTouchEvent")||method.getName().equals("dispatchGenericMotionEvent"))IconInputDiagnostics.motion(a,method.getName(),(android.view.MotionEvent)args[0]);
                if(method.getName().equals("dispatchKeyEvent"))IconInputDiagnostics.key(a,(KeyEvent)args[0]);
                if((method.getName().equals("dispatchKeyEvent")||method.getName().equals("dispatchKeyShortcutEvent"))&&menuKey(a,(KeyEvent)args[0]))return true;
                if(method.getName().equals("onWindowFocusChanged")&&Boolean.FALSE.equals(args[0])){
                    if(backTimer!=null)MAIN.removeCallbacks(backTimer);backTimer=null;backPress.up(SystemClock.uptimeMillis(),true);
                    RenameKeyboard.focusLost(a);
                    PanelPointer.focusLost(a);
                    if(shiftHold!=null&&shiftHold.activity==a)releaseShift(shiftHold);
                }
                try{return method.invoke(previous,args);}catch(java.lang.reflect.InvocationTargetException e){throw e.getCause();}
            });
        a.getWindow().setCallback(callback);attached.put(a,true);
        android.util.Log.i(TAG,"Panel menu-key text entry attached");
    }
    private static Object keyboard(Activity a)throws Exception{
        Field field=a.getClass().getDeclaredField("xServer");field.setAccessible(true);Object server=field.get(a);
        if(server==null)throw new IllegalStateException("Original display is not ready");
        return server.getClass().getField("keyboard").get(server);
    }
    public static void show(Activity a){RenameKeyboard.open(a);}
    static String validateText(String text){
        if(text.isEmpty())return "文字を入力してください";
        if(text.length()>128)return "128文字以内で入力してください";
        for(int i=0;i<text.length();i++)if(text.charAt(i)<32||text.charAt(i)>126)return "原作への送信は英数字・半角記号に対応しています";
        return null;
    }
    private static void key(ArrayList<KeyEvent> out,int key,int meta){long now=SystemClock.uptimeMillis();out.add(new KeyEvent(now,now,KeyEvent.ACTION_DOWN,key,0,meta));out.add(new KeyEvent(now,now,KeyEvent.ACTION_UP,key,0,meta));}
    static ArrayList<KeyEvent> events(String text,boolean replace){
        ArrayList<KeyEvent> out=new ArrayList<>();
        if(replace){key(out,KeyEvent.KEYCODE_MOVE_HOME,0);key(out,KeyEvent.KEYCODE_MOVE_END,KeyEvent.META_SHIFT_ON);key(out,KeyEvent.KEYCODE_DEL,0);}
        for(int i=0;i<text.length();i++)out.add(new KeyEvent(SystemClock.uptimeMillis(),text.substring(i,i+1),-1,0));
        return out;
    }
}
