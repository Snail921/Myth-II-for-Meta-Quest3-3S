package org.mythvr.standalone;

import android.app.Activity;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import java.lang.reflect.Field;
import java.lang.reflect.Method;

/** Direct primary-button dragging on the guest surface, not on Android overlays. */
public final class PanelPointer {
    interface Sink { void move(float x,float y)throws Exception; void button(boolean down)throws Exception; }
    static final class Drag {
        final Sink sink; int pointer=-1;
        Drag(Sink s){sink=s;}
        void begin(int id,float x,float y)throws Exception{cancel();sink.move(x,y);pointer=id;sink.button(true);}
        void move(int id,float x,float y)throws Exception{if(pointer==id)sink.move(x,y);}
        void end(int id,float x,float y)throws Exception{if(pointer==id){try{sink.move(x,y);}finally{cancel();}}}
        void cancel()throws Exception{if(pointer!=-1){sink.button(false);pointer=-1;}}
    }
    private static PanelPointer current;
    private final Activity activity;
    private final Drag drag;
    private PanelPointer(Activity a,View view)throws Exception{
        activity=a;
        Field field=view.getClass().getDeclaredField("xServer");field.setAccessible(true);Object server=field.get(view);
        Field matrix=view.getClass().getDeclaredField("xform");matrix.setAccessible(true);
        Method transform=Class.forName("com.winlator.math.XForm").getMethod("transformPoint",float[].class,float.class,float.class);
        Method move=server.getClass().getMethod("injectPointerMove",int.class,int.class);
        Class<?> buttons=Class.forName("com.winlator.xserver.Pointer$Button");Object left=buttons.getField("BUTTON_LEFT").get(null);
        Method press=server.getClass().getMethod("injectPointerButtonPress",buttons),release=server.getClass().getMethod("injectPointerButtonRelease",buttons);
        drag=new Drag(new Sink(){
            public void move(float x,float y)throws Exception{float[] p=(float[])transform.invoke(null,matrix.get(view),x,y);move.invoke(server,(int)p[0],(int)p[1]);}
            public void button(boolean down)throws Exception{(down?press:release).invoke(server,left);}
        });
        view.setOnTouchListener((v,e)->{
            // Physical mice retain their native button/scroll handling.
            if(e.isFromSource(android.view.InputDevice.SOURCE_MOUSE))return false;
            try{
                int action=e.getActionMasked(),index=e.getActionIndex(),id=e.getPointerId(index);
                if(action==MotionEvent.ACTION_DOWN)drag.begin(id,e.getX(index),e.getY(index));
                else if(action==MotionEvent.ACTION_MOVE){int i=e.findPointerIndex(drag.pointer);if(i>=0)drag.move(drag.pointer,e.getX(i),e.getY(i));else drag.cancel();}
                else if(action==MotionEvent.ACTION_UP||action==MotionEvent.ACTION_POINTER_UP)drag.end(id,e.getX(index),e.getY(index));
                else if(action==MotionEvent.ACTION_CANCEL)drag.cancel();
            }catch(Exception ex){release();android.util.Log.e("MythPanelPointer","Pointer delivery failed",ex);}
            return true; // Never let TouchpadView synthesize a second tap on release.
        });
        view.addOnAttachStateChangeListener(new View.OnAttachStateChangeListener(){
            public void onViewAttachedToWindow(View v){}
            public void onViewDetachedFromWindow(View v){release();if(current==PanelPointer.this)current=null;}
        });
    }
    private void release(){try{drag.cancel();}catch(Exception e){android.util.Log.e("MythPanelPointer","Pointer release failed",e);}}
    static void focusLost(Activity a){if(current!=null&&current.activity==a)current.release();}
    private static View find(View v){
        if(v.getClass().getName().equals("com.winlator.widget.TouchpadView"))return v;
        if(v instanceof ViewGroup){ViewGroup g=(ViewGroup)v;for(int i=0;i<g.getChildCount();i++){View result=find(g.getChildAt(i));if(result!=null)return result;}}
        return null;
    }
    static void attach(Activity a){
        if(current!=null&&current.activity==a)return;
        if(current!=null)current.release();
        try{View v=find(a.getWindow().getDecorView());if(v==null)throw new IllegalStateException("Guest touch surface missing");current=new PanelPointer(a,v);android.util.Log.i("MythPanelPointer","Direct press/drag/release attached");}
        catch(Exception e){android.util.Log.e("MythPanelPointer","Cannot attach direct pointer",e);}
    }
}
