package org.mythvr.standalone;

import android.app.Activity;
import android.util.Log;
import java.io.*;
import java.util.HashSet;

/** Read-only Build471b UI metadata probe. Never reads or logs input text. */
public final class TextInputDiagnostics {
    interface Reader { int u16(long p)throws IOException; long u32(long p)throws IOException; }
    private static final long MODAL=0x6a7510L, HEAD=0x6a750cL;
    private static Thread worker;
    private static java.lang.ref.WeakReference<Activity> owner=new java.lang.ref.WeakReference<>(null);
    private static void pointer(long p)throws IOException {
        if(p<0x10000L||p>0xfffff000L||(p&3)!=0)throw new IOException("invalid pointer");
    }
    // Original Rect is four signed shorts: top, left, bottom, right.
    static int[] rect(Reader r,long p)throws IOException {
        return new int[]{(short)r.u16(p+2),(short)r.u16(p),(short)r.u16(p+6),(short)r.u16(p+4)};
    }
    static String box(int[] v,int x,int y){
        return "("+(v[0]+x)+","+(v[1]+y)+","+(v[2]+x)+","+(v[3]+y)+")";
    }
    static final class RenameState {
        final int pid,width,height,focus;final long dialog;final int[] field;final int bottom;
        final int[][] obstacles;final boolean multiplayer;
        RenameState(int p,long d,int w,int h,int[] f,int b,int id,boolean multi,int[][] blocks){pid=p;dialog=d;width=w;height=h;field=f;bottom=b;focus=id;multiplayer=multi;obstacles=blocks;}
        String token(){return pid+":"+dialog;}
        String targetToken(){return token()+":"+focus;}
        boolean title(){return focus==-1;}
        boolean lobby(){return focus==9;} // ID9 is accepted only for ne#1 below.
    }
    // Manual keyboard eligibility does not depend on a blinking caret or known field ID.
    static RenameState keyboardState(int pid){
        if(pid<=0)return null;
        try(Memory m=new Memory(pid)){return m.supported()?keyboardState(m,pid):null;}catch(IOException e){return null;}
    }
    static RenameState keyboardState(Reader m,int pid)throws IOException {
            RenameState precise=null;try{precise=renameState(m,pid,true);}catch(IOException transientProbe){}if(precise!=null)return precise;
            long modal=m.u32(MODAL),d=modal!=0?modal:m.u32(HEAD);
            if(d==0)return null;pointer(d);
            if(m.u32(d+0x24)!=0x444c4f47L)return null;
            boolean menu=m.u16(EngineProfile.GAME_STATE)!=3,hasField=false;
            HashSet<Long> seen=new HashSet<>();
            if(!menu)for(long c=m.u32(d+0x50);c!=0;c=m.u32(c+0x1c)){
                pointer(c);if(!seen.add(c)||seen.size()>128)return null;
                long kind=m.u32(c+0x24);if(kind==0x65646974L||kind==0x70617373L)hasField=true;
            }
            if(!menu&&!hasField)return null;
            int w=m.u16(0x533bceL),h=m.u16(0x533bd0L);
            if(w<320||h<240||w>4096||h>4096)return null;
            if(m.u32(MODAL)!=modal)return null;
            return new RenameState(pid,d,w,h,new int[]{w/2,0,w/2,0},h-106,-2,false,new int[0][]);
    }
    static RenameState renameState(int pid){
        if(pid<=0)return null;
        try(Memory m=new Memory(pid)){if(!m.supported())return null;return renameState(m,pid);}catch(IOException e){return null;}
    }
    static RenameState renameState(Reader m,int pid)throws IOException {return renameState(m,pid,false);}
    static RenameState renameState(Reader m,int pid,boolean manual)throws IOException {
        long modal=m.u32(MODAL),d=modal;
        if(manual&&d==0){long root=m.u32(HEAD);if(root!=0){pointer(root);if(m.u32(root+0x94)==0x6e652331L)d=root;}}
        if(d==0){
            if(m.u16(EngineProfile.GAME_STATE)==3)return null;
            long root=m.u32(HEAD);if(root==0)return null;pointer(root);
            if(m.u32(root+0x24)!=0x444c4f47L||m.u32(root+0x94)!=0x696d6d36L||m.u32(root+0x58)!=0)return null;
            int w=m.u16(0x533bceL),h=m.u16(0x533bd0L);if(w<320||h<240||w>4096||h>4096)return null;
            return new RenameState(pid,root,w,h,new int[]{w/2,0,w/2,0},h-106,-1,false,new int[0][]);
        }
        pointer(d);
        long tag=m.u32(d+0x94);int focus=m.u16(d+0x22);boolean setup=tag==0x6e657370L,address=tag==0x496e6574L,lobby=tag==0x6e652331L;
        if(manual&&lobby)focus=9; // Chat geometry belongs to the lobby, not to the active button.
        boolean multi=setup||address||lobby;
        if(m.u32(d+0x24)!=0x444c4f47L||!(tag==0x73747074L&&focus==3||setup&&(focus==3||focus==4||focus==13||focus==14)||address&&(focus==3||focus==7||focus==8||focus==11)||lobby&&focus==9))return null;
        if(!(manual&&lobby)&&(m.u16(d+0x20)&1)==0)return null;
        int w=m.u16(0x533bceL),h=m.u16(0x533bd0L);if(w<320||h<240||w>4096||h>4096)return null;
        int[] dr=rect(m,d+0xc),field=null;int bottom=0;HashSet<Long> seen=new HashSet<>();
        java.util.ArrayList<int[]> blocks=new java.util.ArrayList<>();
        long first=m.u32(d+0x50);
        for(long c=first;c!=0;c=m.u32(c+0x1c)){
            pointer(c);if(!seen.add(c)||seen.size()>128)throw new IOException("control list");
            int flags=m.u16(c+8);if((flags&1)!=0)continue;
            int[] b=rect(m,c+0xc);b[0]+=dr[0];b[2]+=dr[0];b[1]+=dr[1];b[3]+=dr[1];
            // The lobby keeps an inactive banner far outside the viewport.
            if(lobby&&m.u16(c+6)==1&&m.u32(c+0x24)==0x62616e6eL&&b[0]>=w)continue;
            if(b[0]<0||b[1]<0||b[2]<b[0]||b[3]<b[1]||b[2]>w||b[3]>h)return null;
            bottom=Math.max(bottom,b[3]);
            // Lobby keyboard may cover chat history, never fields or buttons.
            if(!(lobby&&m.u16(c+6)==8&&m.u32(c+0x24)==0x63686174L))blocks.add(b);
            boolean password=setup&&focus==14||address&&focus==7;
            if(((manual&&lobby)||(flags&2)==0)&&m.u16(c+6)==focus&&m.u32(c+0x24)==(password?0x70617373L:0x65646974L)&&m.u32(c)==(password?0x4031d8L:0x424370L))field=b;
        }
        if(field==null||m.u32(MODAL)!=modal||(!(manual&&lobby)&&m.u16(d+0x22)!=focus)||m.u32(d+0x94)!=tag||m.u32(d+0x50)!=first||!java.util.Arrays.equals(dr,rect(m,d+0xc)))return null;
        return new RenameState(pid,d,w,h,field,bottom,focus,multi,blocks.toArray(new int[0][]));
    }
    static String iconState(int pid){
        if(pid<=0)return "no-process";
        try(Memory m=new Memory(pid)){
            if(!m.supported())return "unsupported-profile";
            long d=m.u32(MODAL);if(d==0)return "no-modal";pointer(d);
            if(m.u32(d+0x24)!=0x444c4f47L||m.u32(d+0x94)!=0x6e657370L)return "other-screen";
            HashSet<Long> seen=new HashSet<>();
            for(long c=m.u32(d+0x50);c!=0;c=m.u32(c+0x1c)){
                pointer(c);if(!seen.add(c)||seen.size()>128)throw new IOException("control list");
                if(m.u16(c+6)!=5)continue;
                if(m.u32(c+0x24)!=0x69706f70L||m.u32(c)!=0x4265adL)return "other-control";
                long payload=m.u32(c+0x14);pointer(payload);if(m.u32(payload+4)!=0x23405511L)return "icon-magic-mismatch";
                String result="pid="+pid+" modal="+Long.toHexString(d)+" focus="+m.u16(d+0x22)+" pressed="+(short)m.u16(d+0x1c)
                    +" dialogFlags="+m.u16(d+0x20)+" iconFlags="+m.u16(c+8)+" localLTRB="+box(rect(m,c+0xc),0,0)
                    +" active="+m.u32(payload)+" tracking="+m.u32(payload+8)+" hover="+(short)m.u16(payload+0x22)+" selected="+(short)m.u16(payload+0x24);
                return m.u32(MODAL)==d&&m.u32(c+0x14)==payload?result:"changed-during-read";
            }
            return "no-icon";
        }catch(IOException e){return "unavailable";}
    }
    static String snapshot(Reader r)throws IOException {
        long modal=r.u32(MODAL),head=r.u32(HEAD);
        StringBuilder b=new StringBuilder("modal=").append(Long.toHexString(modal));
        b.append(" guestSize=").append(r.u16(0x533bceL)).append('x').append(r.u16(0x533bd0L));
        HashSet<Long> dialogs=new HashSet<>();long d=head;int nodes=0;
        // Include the modal pointer even if it is temporarily outside the linked list.
        for(int count=0;count<33;count++) {
            if(d==0){if(modal!=0&&!dialogs.contains(modal))d=modal;else break;}
            pointer(d);if(!dialogs.add(d))throw new IOException("dialog cycle");
            if(dialogs.size()>32)throw new IOException("dialog limit");
            if(r.u32(d+0x24)!=0x444c4f47L)throw new IOException("dialog magic");
            int focus=(short)r.u16(d+0x22);long first=r.u32(d+0x50),c=first;
            int[] bounds=rect(r,d+0xc);
            b.append(" | dialog=").append(Long.toHexString(d)).append(" tag=").append(Long.toHexString(r.u32(d+0x94)))
             .append(" flags=").append(r.u16(d+0x20)).append(" focus=").append(focus)
             .append(" dialogLTRB=").append(box(bounds,0,0));
            HashSet<Long> controls=new HashSet<>();boolean matched=false;
            while(c!=0){
                pointer(c);if(!controls.add(c))throw new IOException("control cycle");
                if(controls.size()>128||++nodes>512)throw new IOException("control limit");
                int id=(short)r.u16(c+6),flags=r.u16(c+8);long type=r.u32(c+0x24),handler=r.u32(c);
                boolean edit=type==0x65646974L||type==0x70617373L;
                // Include all modal controls to locate buttons/labels without reading their text.
                if(edit||id==focus||d==modal){
                    int[] cb=rect(r,c+0xc);
                    b.append(" [id=").append(id).append(" type=").append(Long.toHexString(type))
                     .append(" handler=").append(Long.toHexString(handler)).append(" flags=").append(flags)
                     .append(" state=").append(r.u16(c+10)).append(" focused=").append(id==focus)
                     .append(" editCandidate=").append(edit).append(" enabledCandidate=").append((flags&2)==0)
                     .append(" visibleCandidate=").append((flags&1)==0)
                     .append(" localLTRB=").append(box(cb,0,0))
                     .append(" guestLTRBCandidate=").append(box(cb,bounds[0],bounds[1]))
                     .append(" rectValid=").append(cb[2]>=cb[0]&&cb[3]>=cb[1]).append(']');
                }
                if(id==focus)matched=true;
                c=r.u32(c+0x1c);
            }
            if(focus!=-1&&!matched)b.append(" missingFocus=true");
            if((short)r.u16(d+0x22)!=focus||r.u32(d+0x50)!=first)throw new IOException("changed during sample");
            if(!java.util.Arrays.equals(bounds,rect(r,d+0xc)))throw new IOException("geometry changed during sample");
            d=r.u32(d+0x58);
        }
        if(r.u32(MODAL)!=modal||r.u32(HEAD)!=head)throw new IOException("changed during sample");
        return b.toString();
    }
    private static final class Memory implements Reader,Closeable {
        final RandomAccessFile f;
        Memory(int pid)throws IOException{f=new RandomAccessFile("/proc/"+pid+"/mem","r");}
        public int u16(long p)throws IOException{f.seek(p);return Short.toUnsignedInt(Short.reverseBytes(f.readShort()));}
        public long u32(long p)throws IOException{f.seek(p);return Integer.toUnsignedLong(Integer.reverseBytes(f.readInt()));}
        public void close()throws IOException{f.close();}
        boolean signature(long address,String hex)throws IOException{
            f.seek(address);for(int i=0;i<hex.length();i+=2)if(f.readUnsignedByte()!=Integer.parseInt(hex.substring(i,i+2),16))return false;return true;
        }
        boolean supported()throws IOException{
            return EngineProfile.SHA256.equals("aa68153d987bf8a17c30bf8c7711d3b5e0a291a6cd1f8630446496715539181c")
                &&signature(0x404ce2L,"33c056574033ff6683ff10731d0fbfc7c1e003ffb0e4c452008bb0e0c45200e8")
                &&signature(0x424370L,"558bec510fbf45085333db83f8095657895dfc0f874f020000ff2485f6454200")
                &&signature(0x52c4e0L,"747475622b3142007461747355d042006d746962c41f400069646172204c4000");
        }
    }
    private static void log(Activity a,String message){
        Log.i("MythTextInput",message);
        File dir=a.getExternalFilesDir(null);if(dir==null)return;
        File file=new File(dir,"text-input-diagnostic.log"),old=new File(dir,"text-input-diagnostic.previous.log");
        try{
            if(file.length()>2*1024*1024){if(old.exists()&&!old.delete())return;if(!file.renameTo(old))return;}
            try(FileWriter out=new FileWriter(file,true)){out.write(System.currentTimeMillis()+" mono="+android.os.SystemClock.elapsedRealtime()+" "+message+"\n");}
        }catch(IOException e){Log.w("MythTextInput","Cannot write diagnostic log");}
    }
    public static synchronized void attach(Activity a){
        owner=new java.lang.ref.WeakReference<>(a);if(worker!=null&&worker.isAlive())return;
        worker=new Thread(()->{
            String last="";long emitted=0,scan=0;int pid=-1;boolean enabledBefore=false;
            while(true){
                Activity activity=owner.get();if(activity==null||activity.isDestroyed())return;
                boolean enabled=activity.getSharedPreferences("vr_graphics",0).getBoolean("diagnostic_logging",false);
                if(enabled){
                    long now=android.os.SystemClock.elapsedRealtime();
                    if(!enabledBefore){last="";scan=0;log(activity,"probe=Build471b geometry-v2 metadata-only interval_ms=250 rect_order=left,top,right,bottom");}
                    if(now>=scan){pid=StandaloneBridge.findGame();scan=now+2000;}
                    String value;
                    if(pid<0)value="no-original-process";
                    else try(Memory memory=new Memory(pid)){
                        // Revalidate immutable signatures on every read; PID reuse is not trusted.
                        value=memory.supported()?snapshot(memory):"unsupported-profile";
                    }catch(IOException e){value="unavailable="+e.getClass().getSimpleName()+":"+e.getMessage();}
                    value="pid="+pid+" panelFocus="+activity.hasWindowFocus()+" "+value;
                    if(!value.equals(last)||now-emitted>=10000){log(activity,value);last=value;emitted=now;}
                }else if(enabledBefore){log(activity,"diagnostics-disabled");}
                enabledBefore=enabled;
                try{Thread.sleep(enabled?250:1000);}catch(InterruptedException e){return;}
            }
        },"MythTextInputProbe");worker.setDaemon(true);worker.start();
    }
}
