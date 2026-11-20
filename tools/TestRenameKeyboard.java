package org.mythvr.standalone;
public final class TestRenameKeyboard {
    static void check(boolean yes){if(!yes)throw new AssertionError();}
    public static void main(String[] args){
        float[] p=RenameKeyboard.placement(640,480,177,389,374);
        check(p!=null&&p[0]==81&&p[1]==380&&p[0]+p[2]<=640&&p[1]+p[3]==474);
        check(RenameKeyboard.placement(640,480,177,389,410)==null);
        check(RenameKeyboard.placement(640,480,0,20,200)[0]==6);
        check(RenameKeyboard.keycode("Left")==21&&RenameKeyboard.keycode("Right")==22);
        check(RenameKeyboard.keycode("a")==29&&RenameKeyboard.keycode("z")==54);
        check(RenameKeyboard.keycode("0")==7&&RenameKeyboard.keycode("9")==16);
        check(RenameKeyboard.keycode("Close")==-1&&RenameKeyboard.keycode("Shift")==-1);
        for(String s:new String[]{"-","=","[","]","\\",";","'",",",".","/","`","Space","Enter","Bksp"})check(RenameKeyboard.keycode(s)>=0);
        System.out.println("PASS Rename placement, button clearance, screen bounds, no-room rejection, US key mapping");
    }
}
