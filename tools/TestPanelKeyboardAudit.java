package org.mythvr.standalone;
import java.lang.reflect.*;
/** Isolated probes only: never sends events to a running game or X server. */
public class TestPanelKeyboardAudit {
 public static void main(String[] args)throws Exception{
  android.os.Looper.prepareMainLooper();
  PanelKeyboard.BackPress b=new PanelKeyboard.BackPress();
  b.down(1000); // An UP lost during a focus switch has no reset path.
  if(b.down(5000)||b.up(5050,false)!=2)throw new AssertionError("expected stale hold");
  System.out.println("CONFIRMED state-machine risk: missing release makes a later short press long");
  Class<?> k=Class.forName("com.winlator.xserver.Keyboard"),x=Class.forName("com.winlator.xserver.XServer"),l=Class.forName("com.winlator.xserver.Keyboard$OnKeyboardListener");
  Object keyboard=k.getConstructor(x).newInstance(new Object[]{null});int[] presses={0};
  Object listener=Proxy.newProxyInstance(l.getClassLoader(),new Class[]{l},(p,m,a)->{if(m.getName().equals("onKeyPress"))presses[0]++;return null;});
  k.getMethod("addOnKeyboardListener",l).invoke(keyboard,listener);
  Method press=k.getMethod("setKeyPress",byte.class,int.class),release=k.getMethod("setKeyRelease",byte.class);
  press.invoke(keyboard,(byte)93,97);press.invoke(keyboard,(byte)93,97);
  if(presses[0]!=1)throw new AssertionError("duplicate press");
  release.invoke(keyboard,(byte)93);press.invoke(keyboard,(byte)93,97);
  if(presses[0]!=2)throw new AssertionError("release then press");
  System.out.println("CONFIRMED bundled keyboard suppresses a repeated character key until release; delayed-release scheduling risk remains conditional");
  if(PanelKeyboard.validateText("")==null||PanelKeyboard.validateText("日本語")==null)throw new AssertionError();
  String value=new String(new char[128]).replace('\0','a');
  if(PanelKeyboard.events(value,true).size()!=134)throw new AssertionError();
  System.out.println("CONFIRMED empty/non-ASCII excluded; 128-character replacement queues 134 events (nominal 8.04 seconds to completion)");
 }
}
