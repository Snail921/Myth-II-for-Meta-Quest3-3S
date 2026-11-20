package org.mythvr.standalone;
public final class TestMultiplayerKeyboard {
 public static void main(String[] args)throws Exception {
  int[][] blocks=new int[][]{{82,195,176,210},{82,151,176,167},{82,173,176,188},{93,236,141,252},{91,340,191,356},{199,340,299,356},{337,238,424,254},{337,216,424,232},{338,216,545,232},{335,280,545,298},{338,299,545,333},{432,237,541,254},{432,214,541,232},{338,234,545,292},{335,262,545,280},{520,103,572,153},{498,149,538,192},{338,151,461,186},{207,253,290,336},{100,253,183,336},{179,195,229,220},{179,173,306,189},{179,151,306,167},{419,339,485,367},{491,339,557,367}};
  for(int[] field:new int[][]{{179,151,306,167},{179,173,306,189}}){
   float[] p=RenameKeyboard.multiplayerPlacement(640,480,field,blocks);
   if(p==null||p[0]<0||p[1]<field[3]||p[0]+p[2]>640||p[1]+p[3]>480)throw new AssertionError("placement");
   for(int[] b:blocks)if(p[0]<b[2]&&p[0]+p[2]>b[0]&&p[1]<b[3]&&p[1]+p[3]>b[1])throw new AssertionError("overlap");
   System.out.println("PASS live geometry: "+java.util.Arrays.toString(p));
  }
  TestTextInputDiagnostics.Fake r=new TestTextInputDiagnostics.Fake();long d=0x10000,c=0x20000;
  r.set(0x6a7510,d);r.set(0x533bce,640);r.set(0x533bd0,480);r.set(d+0x24,0x444c4f47);r.set(d+0x94,0x6e657370);r.set(d+0x20,1);r.set(d+0x50,c);
  for(int o=12;o<20;o+=2){r.set(d+o,0);r.set(c+o,20);}
  r.set(c+8,16);r.set(c+0x1c,0);r.set(c+0x24,0x65646974);r.set(c,0x424370);
  String previous="";
  for(int id:new int[]{3,4,13}){r.set(d+0x22,id);r.set(c+6,id);TextInputDiagnostics.RenameState s=TextInputDiagnostics.renameState(r,42);if(s==null||s.targetToken().equals(previous))throw new AssertionError("focus switch");previous=s.targetToken();}
  r.set(d+0x22,14);r.set(c+6,14);r.set(c+0x24,0x70617373);r.set(c,0x4031d8);
  if(TextInputDiagnostics.renameState(r,42)==null)throw new AssertionError("password");
  r.set(c+0x24,0x65646974);r.set(c,0x424370);if(TextInputDiagnostics.renameState(r,42)!=null)throw new AssertionError("password handler");
  r.set(d+0x22,12);r.set(c+6,12);if(TextInputDiagnostics.renameState(r,42)!=null)throw new AssertionError("unknown field");
  r.set(d+0x94,0x496e6574);r.set(d+0x22,8);r.set(c+6,8);
  if(TextInputDiagnostics.renameState(r,42)==null)throw new AssertionError("Join address");
  r.set(c+8,18);if(TextInputDiagnostics.renameState(r,42)!=null)throw new AssertionError("disabled address");r.set(c+8,16);
  for(int id:new int[]{3,11,7}){
   r.set(d+0x22,id);r.set(c+6,id);r.set(c+0x24,id==7?0x70617373:0x65646974);r.set(c,id==7?0x4031d8:0x424370);
   if(TextInputDiagnostics.renameState(r,42)==null)throw new AssertionError("host field "+id);
   r.set(c+8,18);if(TextInputDiagnostics.renameState(r,42)!=null)throw new AssertionError("disabled host field");r.set(c+8,16);
  }
  r.set(c+0x24,0x65646974);r.set(c,0x424370);if(TextInputDiagnostics.renameState(r,42)!=null)throw new AssertionError("host password type");
  r.set(d+0x22,9);r.set(c+6,9);if(TextInputDiagnostics.renameState(r,42)!=null)throw new AssertionError("unknown Inet field");
  r.set(d+0x94,123);if(TextInputDiagnostics.renameState(r,42)!=null)throw new AssertionError("screen exit");
  r.set(0x6a7510,0);r.set(0x6a750c,d);r.set(EngineProfile.GAME_STATE,0);r.set(d+0x94,0x696d6d36);r.set(d+0x58,0);
  if(!TextInputDiagnostics.renameState(r,42).title())throw new AssertionError("title");
  r.set(EngineProfile.GAME_STATE,3);if(TextInputDiagnostics.renameState(r,42)!=null)throw new AssertionError("gameplay is not title");
  System.out.println("PASS Player/Team targets isolated; other field/screen rejected");
 }
}
