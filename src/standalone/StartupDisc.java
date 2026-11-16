package org.mythvr.standalone;
import android.app.Dialog;
import android.content.Context;
import android.graphics.*;
import android.graphics.drawable.ColorDrawable;
import android.os.SystemClock;
import android.view.*;
import java.io.InputStream;

/** Decorative boot animation; original runtime remains responsible for completion. */
public final class StartupDisc extends View {
 private static final String TAG="myth-startup-disc";
 private final Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG|Paint.FILTER_BITMAP_FLAG);
 private Bitmap image;
 private long started;
 public StartupDisc(Context context){
  super(context);setTag(TAG);setBackgroundColor(Color.BLACK);setClickable(true);
  try(InputStream in=new java.io.FileInputStream(new java.io.File(context.getExternalFilesDir(null),"SharedAssets/CD_Face.png"))){image=BitmapFactory.decodeStream(in);}
  catch(Exception e){android.util.Log.e("MythStartupDisc","CD image unavailable",e);}
 }
 public static void prepare(Dialog dialog,int textResource){
  boolean startup="starting_up".equals(dialog.getContext().getResources().getResourceEntryName(textResource));
  Window window=dialog.getWindow();if(window==null)return;
  StartupDisc disc=(StartupDisc)window.getDecorView().findViewWithTag(TAG);
  if(startup){
   if(disc==null){disc=new StartupDisc(dialog.getContext());dialog.addContentView(disc,new ViewGroup.LayoutParams(-1,-1));}
   if(disc.image==null)return;
   disc.started=SystemClock.uptimeMillis();disc.setVisibility(VISIBLE);disc.bringToFront();disc.invalidate();
   window.setBackgroundDrawable(new ColorDrawable(Color.BLACK));window.setLayout(-1,-1);window.setDimAmount(0);
   android.util.Log.i("MythStartupDisc","Boot disc shown: clockwise accelerating 20 to 720 deg/s");
  }else if(disc!=null){disc.setVisibility(GONE);window.setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));}
 }
 static double degrees(double seconds){double t=Math.max(0,seconds);return 20*t+700*(t-4*(1-Math.exp(-t/4)));}
 @Override protected void onDraw(Canvas canvas){
  super.onDraw(canvas);canvas.drawColor(Color.BLACK);if(image==null)return;
  float cx=getWidth()*.5f,cy=getHeight()*.5f,diameter=Math.min(getWidth(),getHeight())*.45f;
  float degrees=(float)(degrees((SystemClock.uptimeMillis()-started)/1000.0)%360);
  canvas.save();canvas.rotate(degrees,cx,cy);canvas.drawBitmap(image,null,new RectF(cx-diameter/2,cy-diameter/2,cx+diameter/2,cy+diameter/2),paint);canvas.restore();
  if(isShown()&&getWindowVisibility()==VISIBLE)postInvalidateOnAnimation();
 }
 @Override protected void onWindowVisibilityChanged(int visibility){super.onWindowVisibilityChanged(visibility);if(visibility==VISIBLE)invalidate();}
}
