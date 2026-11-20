package org.mythvr.standalone;
import java.io.IOException;
import java.util.HashMap;
public final class TestTextInputDiagnostics {
    static final class Fake implements TextInputDiagnostics.Reader {
        HashMap<Long,Long> data=new HashMap<>();
        void set(long p,long v){data.put(p,v);}
        public int u16(long p)throws IOException{return (int)u32(p)&65535;}
        public long u32(long p)throws IOException{if(!data.containsKey(p))throw new IOException("unexpected read "+Long.toHexString(p));return data.get(p);}
    }
    static void check(boolean ok){if(!ok)throw new AssertionError();}
    static void fails(Fake r,String reason)throws Exception{try{TextInputDiagnostics.snapshot(r);throw new AssertionError("accepted "+reason);}catch(IOException e){check(e.getMessage().equals(reason));}}
    public static void main(String[] args)throws Exception{
        Fake r=new Fake();long d=0x10000,c=0x20000;
        r.set(0x533bce,1280);r.set(0x533bd0,720);
        r.set(d+0xc,100);r.set(d+0xe,200);r.set(d+0x10,500);r.set(d+0x12,800);
        r.set(c+0xc,30);r.set(c+0xe,20);r.set(c+0x10,60);r.set(c+0x12,320);
        r.set(0x6a7510,d);r.set(0x6a750c,d);r.set(d+0x24,0x444c4f47);r.set(d+0x22,7);r.set(d+0x50,c);r.set(d+0x94,1);r.set(d+0x20,1);r.set(d+0x58,0);
        r.set(c+6,7);r.set(c+8,16);r.set(c+10,0);r.set(c+0x24,0x65646974);r.set(c,0x424370);r.set(c+0x1c,0);
        String s=TextInputDiagnostics.snapshot(r);check(s.contains("editCandidate=true")&&s.contains("focused=true")&&s.contains("enabledCandidate=true"));
        check(s.contains("guestSize=1280x720")&&s.contains("localLTRB=(20,30,320,60)")&&s.contains("guestLTRBCandidate=(220,130,520,160)"));
        r.set(c+0xe,65526);check(TextInputDiagnostics.snapshot(r).contains("localLTRB=(-10,30,320,60)"));r.set(c+0xe,20);
        r.set(d+0x22,-1);r.set(c+0x24,0x62757474);check(TextInputDiagnostics.snapshot(r).contains("focused=false"));r.set(d+0x22,7);r.set(c+0x24,0x65646974);
        // Payload +0x14 is deliberately absent: any text/payload read fails the test.
        r.set(c+8,19);check(TextInputDiagnostics.snapshot(r).contains("visibleCandidate=false"));
        r.set(c+0x24,0x70617373);check(TextInputDiagnostics.snapshot(r).contains("editCandidate=true"));
        r.set(c+0x24,0x62757474);check(TextInputDiagnostics.snapshot(r).contains("editCandidate=false"));
        r.set(d+0x94,0x73747074);r.set(d+0x22,3);r.set(c+6,3);r.set(c+0x24,0x65646974);r.set(c+8,16);
        TextInputDiagnostics.RenameState state=TextInputDiagnostics.renameState(r,42);
        check(state!=null&&state.field[0]==220&&state.field[1]==130&&state.bottom==160);
        r.set(c+8,18);check(TextInputDiagnostics.renameState(r,42)==null);r.set(c+8,16);
        r.set(d+0x94,1);check(TextInputDiagnostics.renameState(r,42)==null);
        r.set(d+0x22,7);r.set(c+6,7);
        r.set(c+0x1c,c);fails(r,"control cycle");r.set(c+0x1c,0);
        r.set(d+0x58,d);fails(r,"dialog cycle");r.set(d+0x58,0);
        r.set(d+0x24,0);fails(r,"dialog magic");r.set(d+0x24,0x444c4f47);
        r.set(d+0x50,1);fails(r,"invalid pointer");r.set(d+0x50,c);
        r.set(0x6a750c,0);check(TextInputDiagnostics.snapshot(r).contains("dialog=10000"));
        r.set(0x6a7510,0);check(TextInputDiagnostics.snapshot(r).equals("modal=0 guestSize=1280x720"));
        System.out.println("PASS edit/password/button, flags, no payload reads, cycles, invalid pointers, modal-only, closed");
    }
}
