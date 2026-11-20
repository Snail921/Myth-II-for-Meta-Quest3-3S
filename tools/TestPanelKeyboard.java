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
  check(p.down(1000));check(!p.down(1100));check(!p.fire(1999));check(p.up(1999,false)==1);
  check(p.down(2000));check(!p.fire(2999));check(p.fire(3000));check(!p.fire(3100));check(p.up(3200,false)==0);check(p.up(3201,false)==0);
  check(p.down(4000));check(p.up(5000,false)==2);check(!p.fire(5001));
  check(p.down(6000));check(p.up(6100,true)==0);check(!p.fire(7000));
  check(p.down(8000));check(p.fire(9000));check(p.up(9100,true)==0);
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
