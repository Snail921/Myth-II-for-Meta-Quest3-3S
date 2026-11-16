package org.mythvr.standalone;
import android.os.Looper;
import android.view.KeyEvent;
import java.util.ArrayList;
public class TestPanelKeyboard {
 public static void main(String[] args){
  Looper.prepareMainLooper();
  PanelKeyboard.OneClickShift shift=new PanelKeyboard.OneClickShift();
  check(!shift.event("onPointerButtonRelease","BUTTON_LEFT"));
  check(!shift.event("onPointerButtonPress","BUTTON_RIGHT"));check(!shift.event("onPointerButtonRelease","BUTTON_RIGHT"));
  check(!shift.event("onPointerButtonPress","BUTTON_LEFT"));check(shift.event("onPointerButtonRelease","BUTTON_LEFT"));
  check(!shift.event("onPointerButtonRelease","BUTTON_LEFT"));
  PanelKeyboard.BackPress p=new PanelKeyboard.BackPress();
  check(p.down(1000));check(!p.down(1100));check(!p.fire(1599));check(p.up(1599,false)==1);
  check(p.down(2000));check(p.fire(2600));check(!p.fire(2700));check(p.up(2800,false)==2);check(p.up(2801,false)==0);
  check(p.down(3000));check(p.up(3600,false)==2);check(p.up(3700,false)==0);
  check(p.down(4000));check(p.up(4100,true)==0);check(!p.fire(4600));
  check(PanelKeyboard.validateText("Film 01 - Test_ABC!")==null);
  check(PanelKeyboard.validateText("")!=null);
  check(PanelKeyboard.validateText("line\nbreak")!=null);
  check(PanelKeyboard.validateText("日本語")!=null);
  ArrayList<KeyEvent> plain=PanelKeyboard.events("Ab9 !",false);check(plain.size()==5);
  for(int i=0;i<plain.size();i++){check(plain.get(i).getAction()==KeyEvent.ACTION_MULTIPLE);check(plain.get(i).getCharacters().equals("Ab9 !".substring(i,i+1)));}
  ArrayList<KeyEvent> replace=PanelKeyboard.events("X",true);check(replace.size()==7);
  check(replace.get(0).getKeyCode()==KeyEvent.KEYCODE_MOVE_HOME&&replace.get(0).getAction()==0&&replace.get(1).getAction()==1);
  check(replace.get(2).getKeyCode()==KeyEvent.KEYCODE_MOVE_END&&replace.get(2).isShiftPressed()&&replace.get(3).getAction()==1);
  check(replace.get(4).getKeyCode()==KeyEvent.KEYCODE_DEL&&replace.get(5).getAction()==1);
  check(replace.get(6).getCharacters().equals("X"));
  for(KeyEvent event:replace)check(event.getKeyCode()!=KeyEvent.KEYCODE_ENTER);
  System.out.println("PASS panel text validation, per-character events, replacement selection, balanced key releases and no implicit save");
 }
 private static void check(boolean v){if(!v)throw new AssertionError();}
}
