package org.mythvr.standalone;
import java.lang.reflect.*;
public class TestBundledPointer {
 public static void main(String[] args)throws Exception {
  android.os.Looper.prepareMainLooper(); Class<?> c=Class.forName("com.winlator.xserver.Pointer"), x=Class.forName("com.winlator.xserver.XServer"), type=Class.forName("com.winlator.xserver.Pointer$OnPointerMotionListener"),button=Class.forName("com.winlator.xserver.Pointer$Button");
  Object pointer=c.getConstructor(x).newInstance(new Object[]{null});final int[] count={0};
  Object listener=Proxy.newProxyInstance(type.getClassLoader(),new Class[]{type},(p,m,a)->{
   if(m.getName().equals("equals"))return p==a[0];if(m.getName().equals("hashCode"))return System.identityHashCode(p);
   if(m.getName().equals("onPointerButtonPress")||m.getName().equals("onPointerButtonRelease"))count[0]++;return null;
  });
  c.getMethod("addOnPointerMotionListener",type).invoke(pointer,listener);
  Object left=button.getField("BUTTON_LEFT").get(null);Method set=c.getMethod("setButton",button,boolean.class);
  set.invoke(pointer,left,true);set.invoke(pointer,left,false);if(count[0]!=2)throw new AssertionError("callbacks");
  ((java.util.List<?>)PanelKeyboard.pointerListeners(pointer).get(pointer)).remove(listener);
  set.invoke(pointer,left,true);set.invoke(pointer,left,false);if(count[0]!=2)throw new AssertionError("cleanup");
  System.out.println("PASS actual bundled Pointer registration, click callbacks and listener cleanup");
 }
}

