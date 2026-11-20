package org.mythvr.standalone;
import java.util.*;
public final class TestPanelPointer {
    public static void main(String[] args)throws Exception{
        List<String> events=new ArrayList<>();
        PanelPointer.Drag drag=new PanelPointer.Drag(new PanelPointer.Sink(){
            public void move(float x,float y){events.add("move:"+(int)x+","+(int)y);}
            public void button(boolean down){events.add(down?"press":"release");}
        });
        drag.begin(3,10,20);drag.move(3,40,80);drag.end(4,999,999);drag.move(4,999,999);drag.end(3,60,90);drag.cancel();
        expect(events,"move:10,20","press","move:40,80","move:60,90","release");
        events.clear();drag.begin(0,1,2);drag.cancel();drag.end(0,4,5);drag.cancel();
        expect(events,"move:1,2","press","release");
        events.clear();drag.begin(0,1,2);drag.begin(1,3,4);drag.cancel();
        expect(events,"move:1,2","press","release","move:3,4","press","release");
        System.out.println("PASS direct click/drag order, secondary pointer isolation, cancel/focus release, stale release, repeated down");
    }
    static void expect(List<String> actual,String... expected){if(!actual.equals(Arrays.asList(expected)))throw new AssertionError(actual);}
}
