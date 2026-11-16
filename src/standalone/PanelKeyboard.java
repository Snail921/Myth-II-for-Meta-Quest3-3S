package org.mythvr.standalone;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import android.text.InputType;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.View;
import android.view.WindowManager;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.*;
import java.lang.reflect.Method;
import java.lang.reflect.Field;
import java.util.ArrayList;

/** Explicit text entry for the guest's currently focused single-line field. */
public final class PanelKeyboard {
    private static final String TAG="MythPanelKeyboard";
    private static final Handler MAIN=new Handler(Looper.getMainLooper());
    private static boolean showing,sending;
    static final class BackPress {
        boolean held,fired;long started;
        boolean down(long now){if(held)return false;held=true;fired=false;started=now;return true;}
        boolean fire(long now){if(!held||fired||now-started<600)return false;fired=true;return true;}
        // Commit only after release: an opening BACK-up must never reach the new IME.
        // 0: canceled/unmatched; 1: short; 2: long.
        int up(long now,boolean canceled){int result=!held||canceled?0:fired||now-started>=600?2:1;held=false;fired=false;return result;}
    }
    private static final BackPress backPress=new BackPress();
    private static Runnable backTimer;
    private static void toggle(Activity a){
        if(activeDialog!=null&&activeDialog.isShowing()){if(!sending)activeDialog.dismiss();}
        else show(a);
    }
    private static boolean inputFocused(Activity a){return !a.isFinishing()&&!a.isDestroyed()&&(activeDialog!=null&&activeDialog.isShowing()?activeDialog.getWindow().getDecorView().hasWindowFocus():a.hasWindowFocus());}
    private static boolean backKey(Activity a,KeyEvent e){
        if(e.getAction()==KeyEvent.ACTION_DOWN&&e.getRepeatCount()==0&&backPress.down(e.getEventTime())){
            backTimer=()->{if(inputFocused(a)&&backPress.fire(SystemClock.uptimeMillis()))android.util.Log.i(TAG,"Y/B long press armed; waiting for release");};
            MAIN.postAtTime(backTimer,backPress.started+600);
        }else if(e.getAction()==KeyEvent.ACTION_UP){
            if(backTimer!=null)MAIN.removeCallbacks(backTimer);backTimer=null;
            int action=backPress.up(e.getEventTime(),e.isCanceled()||!inputFocused(a));
            if(action==2){android.util.Log.i(TAG,"Y/B long release: keyboard toggle");MAIN.post(()->{if(inputFocused(a))toggle(a);});}
            else if(action==1){
                if(activeDialog!=null&&activeDialog.isShowing()){if(!sending)activeDialog.dismiss();}
                else{android.util.Log.i(TAG,"Y/B short press: original drawer");a.onBackPressed();}
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
    private static void awaitShiftFocus(Activity a,int request,int attempt){
        if(request!=shiftRequest||a.isFinishing()||a.isDestroyed()){shiftLog(a,"Shift request canceled by lifecycle/new request");return;}
        if(showing){shiftLog(a,"Shift request canceled: input dialog still open");return;}
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
    private static AlertDialog activeDialog;
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
                if((method.getName().equals("dispatchKeyEvent")||method.getName().equals("dispatchKeyShortcutEvent"))&&menuKey(a,(KeyEvent)args[0]))return true;
                if(method.getName().equals("onWindowFocusChanged")&&Boolean.FALSE.equals(args[0])&&shiftHold!=null&&shiftHold.activity==a)releaseShift(shiftHold);
                try{return method.invoke(previous,args);}catch(java.lang.reflect.InvocationTargetException e){throw e.getCause();}
            });
        a.getWindow().setCallback(callback);attached.put(a,true);
        android.util.Log.i(TAG,"Panel menu-key text entry attached");
    }
    /** Original status-bar body and independent end caps, drawn behind the editable text. */
    private static final class OriginalFrame extends android.graphics.drawable.Drawable {
        final android.util.SparseArray<android.graphics.Bitmap> pieces=new android.util.SparseArray<>();
        final android.graphics.Paint paint=new android.graphics.Paint(android.graphics.Paint.FILTER_BITMAP_FLAG);
        OriginalFrame(Activity a)throws Exception{
            try(java.io.DataInputStream in=new java.io.DataInputStream(SharedAssets.open(a,"info-decoration.pack"))){
                int count=Integer.reverseBytes(in.readInt());if(count<8||count>64)throw new java.io.IOException("UI pack count");
                for(int i=0;i<count;i++){
                    int id=Integer.reverseBytes(in.readInt()),w=Integer.reverseBytes(in.readInt()),h=Integer.reverseBytes(in.readInt());
                    if(w<1||h<1||w>2048||h>128)throw new java.io.IOException("UI dimensions");
                    byte[] raw=new byte[w*h*4];in.readFully(raw);
                    if(id<35||id>41)continue;
                    int[] colors=new int[w*h];for(int j=0;j<colors.length;j++)colors[j]=((raw[j*4+3]&255)<<24)|((raw[j*4]&255)<<16)|((raw[j*4+1]&255)<<8)|(raw[j*4+2]&255);
                    pieces.put(id,android.graphics.Bitmap.createBitmap(colors,w,h,android.graphics.Bitmap.Config.ARGB_8888));
                }
            }
        }
        private void part(android.graphics.Canvas c,int id,float l,float t,float r,float b){android.graphics.Bitmap image=pieces.get(id);if(image!=null)c.drawBitmap(image,null,new android.graphics.RectF(l,t,r,b),paint);}
        public void draw(android.graphics.Canvas c){
            android.graphics.Rect b=getBounds();int save=c.save();c.translate(b.left,b.top);float w=b.width(),h=b.height(),px=h/64f,cap=32*px;
            c.drawColor(android.graphics.Color.rgb(18,18,18));
            for(int id=35;id<=37;id++){android.graphics.Bitmap image=pieces.get(id);if(image==null)continue;float tile=image.getWidth()*px,top=id==36?59*px:0,bottom=id==35?h:top+5*px;
                int clip=c.save();c.clipRect(0,top,w,bottom);for(float x=0;x<w;x+=tile)part(c,id,x,top,x+tile,bottom);c.restoreToCount(clip);
            }
            part(c,40,0,0,cap,cap);part(c,41,w-cap,0,w,cap);part(c,38,0,cap,cap,h);part(c,39,w-cap,cap,w,h);c.restoreToCount(save);
        }
        public void setAlpha(int value){paint.setAlpha(value);invalidateSelf();}
        public void setColorFilter(android.graphics.ColorFilter value){paint.setColorFilter(value);invalidateSelf();}
        public int getOpacity(){return android.graphics.PixelFormat.TRANSLUCENT;}
    }
    private static Object keyboard(Activity a)throws Exception{
        Field field=a.getClass().getDeclaredField("xServer");field.setAccessible(true);Object server=field.get(a);
        if(server==null)throw new IllegalStateException("Original display is not ready");
        return server.getClass().getField("keyboard").get(server);
    }
    private static void hideIme(Activity a,View v){((InputMethodManager)a.getSystemService(Context.INPUT_METHOD_SERVICE)).hideSoftInputFromWindow(v.getWindowToken(),0);}
    public static void show(Activity a){
        if(showing||sending||a.isFinishing())return;
        StandaloneBridge.cancelPanelHold();
        try{keyboard(a);}catch(Exception e){Toast.makeText(a,"原作画面の準備ができていません",Toast.LENGTH_LONG).show();return;}
        ++shiftRequest;releaseShift(shiftHold);
        showing=true;
        LinearLayout layout=new LinearLayout(a);layout.setOrientation(1);layout.setPadding(dp(a,20),dp(a,8),dp(a,20),dp(a,8));
        TextView help=new TextView(a);help.setText("先に原作の入力欄をクリックしてください。ここで入力した文字を、その欄へ送ります。\n英数字・半角記号に対応。保存の確定は原作側で行ってください。");layout.addView(help);
        EditText text=new EditText(a){
            @Override public boolean dispatchKeyEventPreIme(KeyEvent event){
                if(event.getKeyCode()==KeyEvent.KEYCODE_BACK)return menuKey(a,event);
                return super.dispatchKeyEventPreIme(event);
            }
        };text.setSingleLine(true);text.setHint("Film name / 入力する文字");text.setInputType(InputType.TYPE_CLASS_TEXT|InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS|InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD);text.setImeOptions(EditorInfo.IME_ACTION_DONE|EditorInfo.IME_FLAG_NO_EXTRACT_UI);text.setFilters(new android.text.InputFilter[]{new android.text.InputFilter.LengthFilter(128)});
        text.setTextColor(android.graphics.Color.rgb(242,235,199));text.setHintTextColor(0xffaaa48b);text.setPadding(dp(a,40),dp(a,12),dp(a,40),dp(a,12));
        try{text.setBackground(new OriginalFrame(a));}catch(Exception e){android.util.Log.e(TAG,"Original input frame unavailable",e);}
        layout.addView(text,new LinearLayout.LayoutParams(-1,dp(a,80)));
        CheckBox replace=new CheckBox(a);replace.setText("入力欄の既存の文字を置き換える");replace.setChecked(false);layout.addView(replace);
        Switch shift=new Switch(a);shift.setText("Shift保持（次のクリックまで）");layout.addView(shift);
        TextView state=new TextView(a);layout.addView(state);
        AlertDialog dialog=new AlertDialog.Builder(a).setTitle("原作へ文字入力").setView(layout).setPositiveButton("入力",null).setNegativeButton("閉じる",null).create();
        text.setOnEditorActionListener((v,action,event)->{if(action==EditorInfo.IME_ACTION_DONE){hideIme(a,text);return true;}return false;});
        dialog.setOnKeyListener((d,key,event)->{traceKey("dialog",event);return menuKey(a,event);});
        shift.setOnCheckedChangeListener((button,checked)->{shiftLog(a,"Shift switch="+checked+" sending="+sending);if(checked&&!sending){hideIme(a,text);dialog.dismiss();}});
        activeDialog=dialog;
        dialog.setOnDismissListener(d->{hideIme(a,text);activeDialog=null;showing=false;StandaloneBridge.cancelPanelHold();
            shiftLog(a,"Input dialog dismissed; Shift="+shift.isChecked());
            if(shift.isChecked()&&!sending)MAIN.post(()->armShift(a));
        });
        dialog.setOnShowListener(d->{
            text.requestFocus();dialog.getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_VISIBLE|WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE);
            text.postDelayed(()->{if(dialog.isShowing()){boolean result=((InputMethodManager)a.getSystemService(Context.INPUT_METHOD_SERVICE)).showSoftInput(text,InputMethodManager.SHOW_IMPLICIT);android.util.Log.i(TAG,"System IME requested, accepted="+result);}},200);
            dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v->{
                String value=text.getText().toString();String error=validateText(value);
                if(error!=null){text.setError(error);return;}
                try{
                    Object target=keyboard(a);Method deliver=target.getClass().getMethod("onKeyEvent",KeyEvent.class);
                    ArrayList<KeyEvent> events=events(value,replace.isChecked());sending=true;shift.setEnabled(false);text.setEnabled(false);replace.setEnabled(false);dialog.setCancelable(false);dialog.getButton(-1).setEnabled(false);dialog.getButton(-2).setEnabled(false);hideIme(a,text);state.setText("原作へ入力中…");
                    android.util.Log.i(TAG,"Sending text length="+value.length()+" replace="+replace.isChecked());
                    send(a,dialog,state,target,deliver,events,0);
                }catch(Exception e){android.util.Log.e(TAG,"Keyboard unavailable",e);state.setText("入力経路を開けませんでした。");}
            });
        });
        dialog.show();dialog.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE|WindowManager.LayoutParams.FLAG_ALT_FOCUSABLE_IM);
    }
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
    private static void send(Activity a,AlertDialog dialog,TextView state,Object target,Method deliver,ArrayList<KeyEvent> events,int index){
        if(a.isFinishing()||a.isDestroyed()){sending=false;dialog.dismiss();return;}
        try{
            if(index==events.size()){sending=false;android.util.Log.i(TAG,"Text delivered");dialog.dismiss();Toast.makeText(a,"入力しました。原作側で文字を確認して保存してください。",Toast.LENGTH_LONG).show();return;}
            if(!Boolean.TRUE.equals(deliver.invoke(target,events.get(index))))throw new IllegalStateException("Guest rejected key event");
            // The upstream character path releases custom keycodes after 30 ms.
            MAIN.postDelayed(()->send(a,dialog,state,target,deliver,events,index+1),60);
        }catch(Exception e){sending=false;android.util.Log.e(TAG,"Text delivery failed",e);state.setText("入力に失敗しました。原作側の文字を確認してください。");dialog.setCancelable(true);dialog.getButton(-2).setEnabled(true);}
    }
}
