package org.mythvr.standalone;

import android.app.Activity;
import android.graphics.*;
import android.os.*;
import android.view.*;
import android.widget.FrameLayout;
import java.io.*;
import java.lang.reflect.*;
import java.util.*;

/** Non-IME keyboard pilot, restricted to the audited Rename Film dialog. */
public final class RenameKeyboard extends View {
    private static RenameKeyboard active;
    private static final boolean AUTO_SHOW_KEYBOARD=false; // Internal option; manual B/Y hold is the default.
    private final Activity activity;
    private final Handler main=new Handler(Looper.getMainLooper());
    private final Object server,keyboard,renderer;
    private final Method deliver;
    private final View surface;
    private final Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG|Paint.FILTER_BITMAP_FLAG);
    private final ArrayList<Key> keys=new ArrayList<>();
    private final ArrayDeque<Stroke> queue=new ArrayDeque<>();
    private final float[] transform=new float[4]; // origin x/y, scale x/y
    private TextInputDiagnostics.RenameState state,placementState;
    private long missingSince;
    private boolean surfaceMapped;
    private String suppressed="";
    private boolean shift,touchOwned,manualOpened;
    private volatile boolean stopped;
    private int pressed=-1;
    private Stroke down;
    private Bitmap font;
    private final int[] advances=new int[256];
    private RectF panel;
    private static final class Key {
        final String low,high;final int code;final RectF bounds;
        Key(String l,String h,int c,RectF b){low=l;high=h;code=c;bounds=b;}
    }
    private static final class Stroke {
        final int code,meta;final String token;
        Stroke(int c,int m,String t){code=c;meta=m;token=t;}
    }
    private static Object call(Object o,String name)throws Exception{return o.getClass().getMethod(name).invoke(o);}
    private static Object field(Object o,String name)throws Exception{Field f=o.getClass().getDeclaredField(name);f.setAccessible(true);return f.get(o);}
    private static int number(Object o,String name)throws Exception{return ((Number)call(o,name)).intValue();}
    private static int publicInt(Object o,String name)throws Exception{return ((Number)o.getClass().getField(name).get(o)).intValue();}
    public static void attach(Activity a){
        a.runOnUiThread(()->{try{
            if(active!=null&&active.activity==a)return;
            if(active!=null)active.stop();
            active=new RenameKeyboard(a);
        }catch(Exception e){android.util.Log.e("MythRenameKeyboard","Attach failed",e);}});
    }
    private RenameKeyboard(Activity a)throws Exception {
        super(a);activity=a;server=field(a,"xServer");keyboard=server.getClass().getField("keyboard").get(server);
        deliver=keyboard.getClass().getMethod("onKeyEvent",KeyEvent.class);surface=(View)field(a,"xServerView");renderer=call(server,"getRenderer");
        setFocusable(false);setVisibility(GONE);setImportantForAccessibility(IMPORTANT_FOR_ACCESSIBILITY_NO);
        ((FrameLayout)surface.getParent()).addView(this,new FrameLayout.LayoutParams(-1,-1));
        new Thread(()->{
            Bitmap loaded=null;
            try{loaded=loadFont();}catch(Exception e){android.util.Log.w("MythRenameKeyboard","Original font unavailable; using readable fallback");}
            final Bitmap atlas=loaded;main.post(()->{if(!stopped)font=atlas;});
            int pid=-1;long scan=0;
            while(!stopped&&!a.isDestroyed()){
                long now=SystemClock.uptimeMillis();if(now>=scan){pid=StandaloneBridge.findGame();scan=now+2000;}
                final TextInputDiagnostics.RenameState next=TextInputDiagnostics.keyboardState(pid);
                main.post(()->{if(!stopped)update(next);});
                try{Thread.sleep(200);}catch(InterruptedException e){break;}
            }
            main.post(this::stop);
        },"MythRenameKeyboard").start();
    }
    private Bitmap loadFont()throws IOException {
        try(DataInputStream in=new DataInputStream(SharedAssets.open(activity,"info-font.rgba"))){
            int w=Integer.reverseBytes(in.readInt()),h=Integer.reverseBytes(in.readInt());if(w!=512||h!=512)throw new IOException("font dimensions");
            int[] pixels=new int[w*h];for(int i=0;i<pixels.length;i++){int r=in.readUnsignedByte(),g=in.readUnsignedByte(),b=in.readUnsignedByte(),a=in.readUnsignedByte();pixels[i]=a<<24|r<<16|g<<8|b;}
            for(int i=0;i<256;i++)advances[i]=Short.toUnsignedInt(Short.reverseBytes(in.readShort()));
            return Bitmap.createBitmap(pixels,w,h,Bitmap.Config.ARGB_8888);
        }
    }
    private void trace(String msg){android.util.Log.i("MythRenameKeyboard",msg);}
    private void update(TextInputDiagnostics.RenameState next){
        if(next==null){long now=SystemClock.uptimeMillis();if(missingSince==0)missingSince=now;
            if(state!=null&&manualOpened&&now-missingSince<1000)return;
            placementState=null;surfaceMapped=false;if(state!=null)trace("Editing ended");if(state!=null&&state.title())PanelKeyboard.customShift(activity,false);state=null;suppressed="";shift=false;manualOpened=false;hide();return;}
        missingSince=0;
        boolean changed=state==null||!state.token().equals(next.token());
        if(state!=null&&!state.targetToken().equals(next.targetToken())){queue.clear();release();if(state.title()){PanelKeyboard.customShift(activity,false);manualOpened=false;}}
        state=next;
        if(changed){placementState=null;surfaceMapped=false;manualOpened=false;shift=false;suppressed="";trace("Rename editing began");}
        if((next.title()||!AUTO_SHOW_KEYBOARD)&&!manualOpened){hide();return;}
        if(!activity.hasWindowFocus()||activity.isFinishing()||suppressed.equals(next.token())){hide();return;}
        try{surfaceMapped=mapSurface(next)||surfaceMapped;}catch(Exception e){/* Keep the last valid transform for this same dialog. */}
        if(!surfaceMapped){hide();return;}
        if(placementState==null||placementState.width!=next.width||placementState.height!=next.height)placementState=next;
        layoutKeys(placementState);if(panel==null){hide();return;}
        if(getVisibility()!=VISIBLE){trace("Keyboard shown; originalFont="+(font!=null));setVisibility(VISIBLE);bringToFront();}
        invalidate();
    }
    // Map original pixels through Wine's game window and the renderer's actual viewport.
    private boolean mapSurface(TextInputDiagnostics.RenameState s)throws Exception {
        Object lock=call(server,"lockAll");try{
            Object manager=server.getClass().getField("windowManager").get(server),win=call(manager,"getFocusedWindow"),game=null;
            for(int n=0;win!=null&&n<32;n++,win=call(win,"getParent")){
                if(number(win,"getWidth")==s.width&&number(win,"getHeight")==s.height&&String.valueOf(call(win,"getName")).toLowerCase(Locale.ROOT).contains("myth")){game=win;break;}
            }
            if(game==null)return false;
            Object screen=server.getClass().getField("screenInfo").get(server);int sw=publicInt(screen,"width"),sh=publicInt(screen,"height");
            float gx=number(game,"getRootX"),gy=number(game,"getRootY"),gw=s.width,gh=s.height;
            Object full=call(game,"getFullscreenTransformation");
            if(full!=null){gx=publicInt(full,"x");gy=publicInt(full,"y");gw=publicInt(full,"width");gh=publicInt(full,"height");}
            boolean stretch=(Boolean)call(renderer,"isFullscreen");
            float scale=Math.min((float)surface.getWidth()/sw,(float)surface.getHeight()/sh);
            float vx=stretch?0:(int)((surface.getWidth()-sw*scale)*.5f),vy=stretch?0:(int)((surface.getHeight()-sh*scale)*.5f);
            float vw=stretch?surface.getWidth():(float)Math.ceil(sw*scale),vh=stretch?surface.getHeight():(float)Math.ceil(sh*scale);
            int[] origin=new int[2],own=new int[2];surface.getLocationInWindow(origin);getLocationInWindow(own);
            transform[0]=origin[0]-own[0]+vx+gx*vw/sw;transform[1]=origin[1]-own[1]+vy+gy*vh/sh;
            transform[2]=gw/s.width*vw/sw;transform[3]=gh/s.height*vh/sh;
            return transform[2]>0&&transform[3]>0;
        }finally{((AutoCloseable)lock).close();}
    }
    static float[] placement(int w,int h,int left,int right,int bottom){
        float width=Math.min(404,w-12),height=94,x=Math.max(6,Math.min(w-width-6,(left+right-width)*.5f)),y=bottom+6;
        return y+height<=h-4?new float[]{x,y,width,height}:null;
    }
    static float[] multiplayerPlacement(int w,int h,int[] field,int[][] blocks){
        float width=Math.min(404,w-12),height=94,center=(field[0]+field[2]-width)*.5f;
        java.util.ArrayList<Float> xs=new java.util.ArrayList<>(),ys=new java.util.ArrayList<>();
        xs.add(center);xs.add(6f);xs.add(w-width-6);ys.add(field[3]+6f);
        for(int[] b:blocks){xs.add(b[0]-width-6);xs.add(b[2]+6f);ys.add(b[3]+6f);}
        float[] best=null;float score=Float.MAX_VALUE;
        for(float y:ys)for(float rawX:xs){float x=Math.max(6,Math.min(w-width-6,rawX));
            if(y<field[3]+6||y+height>h-4)continue;
            boolean clear=true;for(int[] b:blocks)if(x<b[2]+3&&x+width>b[0]-3&&y<b[3]+3&&y+height>b[1]-3){clear=false;break;}
            float cost=(y-field[3])*4+Math.abs(x-center);
            if(clear&&cost<score){score=cost;best=new float[]{x,y,width,height};}
        }
        return best;
    }
    static float[] lobbyPlacement(int w,int h,int[] field,int[][] blocks){
        float width=Math.min(404,w-12),height=94;
        float x=Math.max(6,Math.min(w-width-6,(field[0]+field[2]-width)*.5f)),y=field[1]-6-height;
        if(y<4||y+height>h-4)return null;
        for(int[] b:blocks)if(x<b[2]+3&&x+width>b[0]-3&&y<b[3]+3&&y+height>b[1]-3)return null;
        return new float[]{x,y,width,height};
    }
    private void layoutKeys(TextInputDiagnostics.RenameState s){
        float[] p=s.lobby()?lobbyPlacement(s.width,s.height,s.field,s.obstacles):s.multiplayer?multiplayerPlacement(s.width,s.height,s.field,s.obstacles):placement(s.width,s.height,s.field[0],s.field[2],s.bottom);if(p==null&&manualOpened)p=placement(s.width,s.height,s.width/2,s.width/2,s.height-106);if(p==null){panel=null;return;}
        panel=new RectF(p[0],p[1],p[0]+p[2],p[1]+p[3]);keys.clear();
        row(0,new String[]{"1","2","3","4","5","6","7","8","9","0","-","=","Bksp"},new String[]{"!","@","#","$","%","^","&","*","(",")","_","+","Bksp"});
        row(1,new String[]{"q","w","e","r","t","y","u","i","o","p","[","]","\\"},null);
        row(2,new String[]{"a","s","d","f","g","h","j","k","l",";","'","Enter"},null);
        row(3,new String[]{"Shift","z","x","c","v","b","n","m",",",".","/"},null);
        row(4,new String[]{"`","Space","Left","Right","Close"},null);
    }
    static int keycode(String s){
        if(s.length()==1){char c=s.charAt(0);if(c>='a'&&c<='z')return KeyEvent.KEYCODE_A+c-'a';if(c>='0'&&c<='9')return KeyEvent.KEYCODE_0+c-'0';
            switch(c){case '-':return KeyEvent.KEYCODE_MINUS;case '=':return KeyEvent.KEYCODE_EQUALS;case '[':return KeyEvent.KEYCODE_LEFT_BRACKET;case ']':return KeyEvent.KEYCODE_RIGHT_BRACKET;case '\\':return KeyEvent.KEYCODE_BACKSLASH;case ';':return KeyEvent.KEYCODE_SEMICOLON;case '\'':return KeyEvent.KEYCODE_APOSTROPHE;case ',':return KeyEvent.KEYCODE_COMMA;case '.':return KeyEvent.KEYCODE_PERIOD;case '/':return KeyEvent.KEYCODE_SLASH;case '`':return KeyEvent.KEYCODE_GRAVE;}}
        switch(s){case "Space":return KeyEvent.KEYCODE_SPACE;case "Enter":return KeyEvent.KEYCODE_ENTER;case "Bksp":return KeyEvent.KEYCODE_DEL;case "Left":return KeyEvent.KEYCODE_DPAD_LEFT;case "Right":return KeyEvent.KEYCODE_DPAD_RIGHT;default:return -1;}
    }
    private void row(int row,String[] labels,String[] highs){
        float units=0;for(String s:labels)units+=s.equals("Space")?6:s.length()>1?2:1;
        float unit=(panel.width()-4)/units,x=panel.left+2,y=panel.top+2+row*18;
        for(int i=0;i<labels.length;i++){
            String l=labels[i],h=highs==null?l.toUpperCase(Locale.ROOT):highs[i];
            if(highs==null&&l.length()==1){int k="[]\\;',./`".indexOf(l);if(k>=0)h="{}|:\"<>?~".substring(k,k+1);}
            float w=unit*(l.equals("Space")?6:l.length()>1?2:1);
            keys.add(new Key(l,h,keycode(l),new RectF(x,y,x+w-1,y+17)));x+=w;
        }
    }
    protected void onDraw(Canvas c){
        super.onDraw(c);if(panel==null)return;int saved=c.save();c.translate(transform[0],transform[1]);c.scale(transform[2],transform[3]);
        paint.setColor(0xf0202020);c.drawRect(panel,paint);
        for(int i=0;i<keys.size();i++){
            Key k=keys.get(i);paint.setColor(i==pressed?0xff77705b:shift&&k.low.equals("Shift")?0xff6e613b:0xff3b3b3b);c.drawRect(k.bounds,paint);
            if(k.low.equals("Left")||k.low.equals("Right")){
                float x=k.bounds.centerX(),y=k.bounds.centerY(),dir=k.low.equals("Left")?-1:1;paint.setColor(Color.WHITE);paint.setStrokeWidth(1);c.drawLine(x-5,y,x+5,y,paint);c.drawLine(x+dir*5,y,x+dir,y-4,paint);c.drawLine(x+dir*5,y,x+dir,y+4,paint);
            }else label(c,shift?k.high:k.low,k.bounds);
        }
        c.restoreToCount(saved);
    }
    private void label(Canvas c,String text,RectF box){
        paint.setColor(Color.WHITE);
        if(font==null){paint.setTypeface(Typeface.SERIF);paint.setTextSize(11);paint.setTextAlign(Paint.Align.CENTER);c.drawText(text,box.centerX(),box.centerY()+6,paint);return;}
        float sum=0;for(char ch:text.toCharArray())sum+=advances[ch];float scale=Math.min(.52f,(box.width()-3)/Math.max(sum,1));float x=box.centerX()-sum*scale/2-4*scale,y=box.centerY()-16*scale+2;
        for(char ch:text.toCharArray()){int sx=(ch%16)*32,sy=(ch/16)*32;c.drawBitmap(font,new Rect(sx,sy,sx+32,sy+32),new RectF(x,y,x+32*scale,y+32*scale),paint);x+=advances[ch]*scale;}
    }
    private int hit(float x,float y){
        if(panel==null||getVisibility()!=VISIBLE)return -1;x=(x-transform[0])/transform[2];y=(y-transform[1])/transform[3];
        for(int i=0;i<keys.size();i++)if(keys.get(i).bounds.contains(x,y))return i;return -1;
    }
    private boolean inside(float x,float y){return panel!=null&&getVisibility()==VISIBLE&&panel.contains((x-transform[0])/transform[2],(y-transform[1])/transform[3]);}
    public boolean onTouchEvent(MotionEvent e){
        int action=e.getActionMasked();
        if(action==MotionEvent.ACTION_DOWN){if(!inside(e.getX(),e.getY()))return false;touchOwned=true;pressed=hit(e.getX(),e.getY());StandaloneBridge.cancelPanelHold();invalidate();return true;}
        if(!touchOwned)return false;
        if(action==MotionEvent.ACTION_UP){int k=hit(e.getX(),e.getY());if(k==pressed&&k>=0)click(keys.get(k));touchOwned=false;pressed=-1;invalidate();}
        else if(action==MotionEvent.ACTION_CANCEL){touchOwned=false;pressed=-1;invalidate();}
        return true;
    }
    public boolean onGenericMotionEvent(MotionEvent e){
        if(!inside(e.getX(),e.getY())&&!touchOwned)return false;
        if(e.getActionMasked()==MotionEvent.ACTION_BUTTON_PRESS){touchOwned=true;pressed=hit(e.getX(),e.getY());}
        else if(e.getActionMasked()==MotionEvent.ACTION_BUTTON_RELEASE){int k=hit(e.getX(),e.getY());if(touchOwned&&k>=0&&k==pressed)click(keys.get(k));touchOwned=false;pressed=-1;}
        StandaloneBridge.cancelPanelHold();invalidate();return true;
    }
    private boolean eligible(){return state!=null&&activity.hasWindowFocus()&&getVisibility()==VISIBLE&&!stopped;}
    private void click(Key k){
        if(!eligible())return;
        if(k.low.equals("Close")){dismiss();return;}
        if(k.low.equals("Shift")){shift=!shift;if(state.title())PanelKeyboard.customShift(activity,shift);invalidate();return;}
        if(queue.size()<32){queue.add(new Stroke(k.code,shift?KeyEvent.META_SHIFT_ON:0,state.targetToken()));pump();}
    }
    private void pump(){
        if(down!=null||queue.isEmpty())return;
        Stroke key=queue.remove();
        TextInputDiagnostics.RenameState fresh=state==null?null:TextInputDiagnostics.keyboardState(state.pid);
        if(!eligible()||fresh==null||!fresh.targetToken().equals(key.token)){queue.clear();return;}
        try{
            down=key;send(key,KeyEvent.ACTION_DOWN);main.postDelayed(()->{
                if(down==key){
                    release();
                    if(key.code==KeyEvent.KEYCODE_ENTER&&state!=null&&state.targetToken().equals(key.token)){dismiss();return;}
                }
                main.postDelayed(this::pump,20);
            },40);
        }catch(Exception e){trace("Key delivery failed");release();queue.clear();}
    }
    private void send(Stroke key,int action)throws Exception{
        long now=SystemClock.uptimeMillis();if(!Boolean.TRUE.equals(deliver.invoke(keyboard,new KeyEvent(now,now,action,key.code,0,key.meta))))throw new IOException("key rejected");
    }
    private void release(){Stroke key=down;down=null;if(key!=null)try{send(key,KeyEvent.ACTION_UP);}catch(Exception e){trace("Key release failed");}}
    private void dismiss(){if(state!=null){suppressed=state.token();if(state.title())PanelKeyboard.customShift(activity,false);}manualOpened=false;hide();}
    private void hide(){queue.clear();release();touchOwned=false;pressed=-1;setVisibility(GONE);}
    private void stop(){stopped=true;hide();if(getParent() instanceof android.view.ViewGroup)((android.view.ViewGroup)getParent()).removeView(this);}
    public static boolean open(Activity a){
        RenameKeyboard k=active;if(k==null||k.activity!=a)return false;
        k.update(TextInputDiagnostics.keyboardState(StandaloneBridge.findGame()));if(k.state==null)return false;
        k.manualOpened=true;k.placementState=null;k.suppressed="";k.update(k.state);return true;
    }
    public static void focusLost(Activity a){if(active!=null&&active.activity==a){active.shift=false;PanelKeyboard.customShift(a,false);active.hide();}}
    public static boolean ownsMotion(Activity a,MotionEvent e){
        RenameKeyboard k=active;if(k==null||k.activity!=a||k.getVisibility()!=VISIBLE)return false;
        int[] where=new int[2];k.getLocationOnScreen(where);return k.touchOwned||k.inside(e.getRawX()-where[0],e.getRawY()-where[1]);
    }
}
