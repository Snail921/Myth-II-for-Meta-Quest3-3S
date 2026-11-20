package org.mythvr.standalone;
public final class TestManualKeyboard {
 public static void main(String[] args)throws Exception{
  TestTextInputDiagnostics.Fake r=new TestTextInputDiagnostics.Fake();long d=0x10000,c=0x20000;
  r.set(0x6a7510,d);r.set(d+0x24,0x444c4f47);r.set(d+0x94,123);r.set(d+0x22,99);
  r.set(EngineProfile.GAME_STATE,1);r.set(0x533bce,640);r.set(0x533bd0,480);
  if(TextInputDiagnostics.keyboardState(r,1)==null)throw new AssertionError("unknown menu");
  r.set(EngineProfile.GAME_STATE,3);r.set(d+0x50,c);r.set(c+0x24,0x65646974);r.set(c+0x1c,0);
  if(TextInputDiagnostics.keyboardState(r,1)==null)throw new AssertionError("unfocused edit field");
  r.set(c+0x24,0x70617373);
  if(TextInputDiagnostics.keyboardState(r,1)==null)throw new AssertionError("password field");
  r.set(c+0x24,0x6275746e);
  if(TextInputDiagnostics.keyboardState(r,1)!=null)throw new AssertionError("game dialog without field");
  System.out.println("PASS manual keyboard unknown menu, unfocused edit/password, gameplay exclusion");
 }
}
