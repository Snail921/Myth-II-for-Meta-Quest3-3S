package org.mythvr.standalone;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.security.SecureRandom;

/** Runs in the compatibility application's own UID; no ADB, run-as, or shell supervisor. */
public final class StandaloneBridge {
    private static volatile boolean monitoring, opened;
    private static volatile String state="Not started";
    private static Process capture;
    private static File base;
    private static final Handler inputHandler=new Handler(Looper.getMainLooper());
    private static Runnable holdAction;
    private static boolean triggerDown;
    private static String lastDiagnostic="";
    private static Context audioContext;
    private static boolean lifecycleRegistered;
    private static java.lang.ref.WeakReference<Activity> immersiveActivity=new java.lang.ref.WeakReference<>(null);
    private static volatile long diagnosticPosted, diagnosticAck;
    private static volatile String diagnosticActivity="none";
    private static void transition(Activity a,String event){
        diagnosticActivity=a.getClass().getName()+":"+event;
        if(!a.getSharedPreferences("vr_graphics",Context.MODE_PRIVATE).getBoolean("diagnostic_logging",false))return;
        Log.i("MythTransition","mono="+android.os.SystemClock.elapsedRealtime()+" "+diagnosticActivity+" focus="+a.hasWindowFocus()+" finishing="+a.isFinishing()+" thread="+Thread.currentThread().getName());
    }
    private static void startDiagnosticWatch(Context context){
        final Context c=context.getApplicationContext();
        Thread watcher=new Thread(()->{long lastStack=0,lastAudio=0;boolean wasEnabled=false;
            for(;;){try{Thread.sleep(1000);}catch(InterruptedException e){return;}
                boolean enabled=c.getSharedPreferences("vr_graphics",Context.MODE_PRIVATE).getBoolean("diagnostic_logging",false);
                if(!enabled){wasEnabled=false;continue;}
                long now=android.os.SystemClock.elapsedRealtime();
                if(!wasEnabled){diagnosticPosted=diagnosticAck=0;wasEnabled=true;}
                if(diagnosticPosted>diagnosticAck&&now-diagnosticPosted>=1500&&now-lastStack>=5000){
                    lastStack=now;StringBuilder b=new StringBuilder("UI stalled ms=").append(now-diagnosticPosted).append(" activity=").append(diagnosticActivity);
                    StackTraceElement[] stack=Looper.getMainLooper().getThread().getStackTrace();for(int i=0;i<Math.min(16,stack.length);i++)b.append("\n  ").append(stack[i]);
                    Log.w("MythTransition",b.toString());
                }
                if(now-lastAudio>=5000){lastAudio=now;try{
                    android.media.AudioManager audio=(android.media.AudioManager)c.getSystemService(Context.AUDIO_SERVICE);
                    Log.i("MythTransition","Audio mono="+now+" activity="+diagnosticActivity+" global_music_active="+audio.isMusicActive()+" visible_playback_configs="+audio.getActivePlaybackConfigurations().size()+" policy="+audioPolicyStatus());
                }catch(Exception e){Log.w("MythTransition","Audio state unavailable",e);}}
                if(diagnosticPosted<=diagnosticAck){final long sent=now;diagnosticPosted=sent;inputHandler.post(()->{
                    long lag=android.os.SystemClock.elapsedRealtime()-sent;diagnosticAck=sent;
                    if(lag>=250)Log.i("MythTransition","UI recovered lag_ms="+lag+" activity="+diagnosticActivity);
                });}
            }
        },"MythTransitionWatch");watcher.setDaemon(true);watcher.start();
    }
    private static synchronized void watchActivities(Context c){
        if(lifecycleRegistered)return;lifecycleRegistered=true;startDiagnosticWatch(c);
        ((android.app.Application)c.getApplicationContext()).registerActivityLifecycleCallbacks(new android.app.Application.ActivityLifecycleCallbacks(){
            public void onActivityCreated(Activity a,android.os.Bundle b){transition(a,"created");if(a instanceof android.app.NativeActivity)immersiveActivity=new java.lang.ref.WeakReference<>(a);}
            public void onActivityDestroyed(Activity a){transition(a,"destroyed");report("Activity destroyed: "+a.getClass().getName()+" finishing="+a.isFinishing());exportDiagnostics(a);if(immersiveActivity.get()==a)immersiveActivity.clear();}
            public void onActivityStarted(Activity a){transition(a,"started");} public void onActivityResumed(Activity a){transition(a,"resumed");Log.i("MythTransition",audioPolicyStatus());} public void onActivityPaused(Activity a){transition(a,"paused");} public void onActivityStopped(Activity a){transition(a,"stopped");} public void onActivitySaveInstanceState(Activity a,android.os.Bundle b){}
        });
    }
    public static boolean keepRuntimeForRemovedTask(Intent rootIntent){
        if(!monitoring||rootIntent==null||rootIntent.getComponent()==null)return false;
        if(!"android.app.NativeActivity".equals(rootIntent.getComponent().getClassName())||findGame()<0)return false;
        report("VR task removed; preserving original runtime and result panel");return true;
    }
    private static void returnToPanel(Activity engine,String reason){
        transition(engine,"return-request:"+reason);
        opened=false;activeDefinition=null;if(capture!=null){capture.destroy();capture=null;}
        report("Returning to original panel: "+reason);
        inputHandler.post(()->{
            Activity vr=immersiveActivity.get();if(vr!=null&&!vr.isFinishing())vr.finish();
            if(!engine.isFinishing()){
                android.app.ActivityManager manager=(android.app.ActivityManager)engine.getSystemService(Context.ACTIVITY_SERVICE);
                for(android.app.ActivityManager.AppTask task:manager.getAppTasks())if(task.getTaskInfo().id==engine.getTaskId()){task.moveToFront();break;}
            }
        });
    }
    private static int gameState(int pid){try(RandomAccessFile f=new RandomAccessFile("/proc/"+pid+"/mem","r")){return shortAt(f,EngineProfile.GAME_STATE);}catch(Exception e){return -1;}}
    private static String audioPolicyStatus(){
        try{if(audioContext==null)return "";android.app.AppOpsManager ops=(android.app.AppOpsManager)audioContext.getSystemService(Context.APP_OPS_SERVICE);
            int mode=ops.checkOpNoThrow("android:play_audio",android.os.Process.myUid(),audioContext.getPackageName());
            return mode==android.app.AppOpsManager.MODE_ALLOWED?"Audio output: allowed / 音声出力は許可されています。":"Audio output is muted by Quest. Unmute this app in the Quest panel. / Quest側でアプリがミュートされています。アプリの音量設定をご確認ください。";
        }catch(Exception e){return "Audio policy check unavailable: "+e;}
    }
    public static void cancelPanelHold(){if(holdAction!=null)inputHandler.removeCallbacks(holdAction);holdAction=null;triggerDown=false;}
    private static void panelTrigger(Activity a,boolean down){
        if(down==triggerDown)return;
        if(!down){cancelPanelHold();return;}
        triggerDown=true;
        holdAction=()->{if(!triggerDown||a.isFinishing())return;try{
            new Thread(()->{try(java.net.DatagramSocket socket=new java.net.DatagramSocket()){
                byte[] packet=new byte[256];packet[0]=11;packet[1]=27;
                java.net.InetAddress local=java.net.InetAddress.getByName("127.0.0.1");
                socket.send(new java.net.DatagramPacket(packet,packet.length,local,7946));
                Thread.sleep(80);packet[2]=2;socket.send(new java.net.DatagramPacket(packet,packet.length,local,7946));
            }catch(Exception e){report("Skip keyboard delivery failed: "+e);}},"MythSkip").start();
            report("Panel trigger held 1 second: sent original Escape key");
        }catch(Exception e){report("Panel skip failed: "+e);}};
        inputHandler.postDelayed(holdAction,1000);
    }
    public static void panelMotion(Activity a,android.view.MotionEvent e){
        PanelKeyboard.traceMotion(e);
        int action=e.getActionMasked();
        if(action==android.view.MotionEvent.ACTION_DOWN)panelTrigger(a,true);
        else if(action==android.view.MotionEvent.ACTION_UP||action==android.view.MotionEvent.ACTION_CANCEL)panelTrigger(a,false);
        else if(action==android.view.MotionEvent.ACTION_BUTTON_PRESS)panelTrigger(a,(e.getButtonState()&android.view.MotionEvent.BUTTON_PRIMARY)!=0);
        else if(action==android.view.MotionEvent.ACTION_BUTTON_RELEASE)panelTrigger(a,false);
        else if((e.getSource()&android.view.InputDevice.SOURCE_JOYSTICK)==android.view.InputDevice.SOURCE_JOYSTICK)panelTrigger(a,e.getAxisValue(android.view.MotionEvent.AXIS_RTRIGGER)>.55f);
    }
    public static boolean panelKey(Activity a,android.view.KeyEvent e){PanelKeyboard.traceKey("activity",e);if(PanelKeyboard.menuKey(a,e))return true;if(e.getKeyCode()==android.view.KeyEvent.KEYCODE_BUTTON_R2)panelTrigger(a,e.getAction()==android.view.KeyEvent.ACTION_DOWN);return false;}
    private static int intAt(RandomAccessFile f,long p)throws IOException{f.seek(p);return Integer.reverseBytes(f.readInt());}
    private static int shortAt(RandomAccessFile f,long p)throws IOException{f.seek(p);return Short.toUnsignedInt(Short.reverseBytes(f.readShort()));}
    private static void diagnose(int pid){try(RandomAccessFile f=new RandomAccessFile("/proc/"+pid+"/mem","r")){
        int n=shortAt(f,EngineProfile.SELECTION),flags=shortAt(f,0x553fda),mode=shortAt(f,0x67ec06);
        String value="Control state="+shortAt(f,EngineProfile.GAME_STATE)+" simFlags="+intAt(f,0x67ec10)+" mode="+mode+" player="+intAt(f,EngineProfile.PLAYER)+" team="+intAt(f,EngineProfile.TEAM)+" selection="+n+" flags="+flags+" camera="+Float.intBitsToFloat(intAt(f,0x54e43c))+","+Float.intBitsToFloat(intAt(f,0x54e440))+","+Float.intBitsToFloat(intAt(f,0x54e444));
        if(n>0&&n<=126){int slot=shortAt(f,0x553fdc);if(slot<400){long actor=Integer.toUnsignedLong(intAt(f,0x529f54))+slot*216;f.seek(actor+7);int af=f.readUnsignedByte();f.seek(actor+22);int team=f.readByte(),owner=f.readByte();value+=" slot="+slot+" id="+shortAt(f,actor)+" actorFlags="+af+" actorTeam="+team+" actorOwner="+owner;}}
        if(!value.equals(lastDiagnostic)){lastDiagnostic=value;report(value);}
    }catch(Exception e){report("Control diagnostics unavailable: "+e);}}
    private static final java.util.HashMap<String,Long> relocated=new java.util.HashMap<>();
    /** Bundled Linux ELF interpreters contain the upstream app's absolute rootfs path.
     * Use a shorter private alias, padded with Unix path separators to retain ELF offsets. */
    public static synchronized void relocate(String command){
        if(base==null)return;
        try{
            File alias=new File(base,"r");if(!Files.isSymbolicLink(alias.toPath()))android.system.Os.symlink(new File(base,"files/rootfs").getPath(),alias.getPath());
            relocateTree(new File(base,"files/rootfs"),alias.getPath());
        }catch(Exception e){throw new IllegalStateException("Guest path relocation failed",e);}
    }
    private static void relocateTree(File f,String alias)throws Exception{
        if(Files.isSymbolicLink(f.toPath()))return;
        if(f.isDirectory()){File[] list=f.listFiles();if(list!=null)for(File child:list)relocateTree(child,alias);return;}
        if(!f.isFile()||f.length()<4||f.length()>67108864)return;
        Long stamp=relocated.get(f.getPath());if(stamp!=null&&stamp==f.lastModified())return;
        boolean elf;try(InputStream in=new FileInputStream(f)){elf=in.read()==127&&in.read()=='E'&&in.read()=='L'&&in.read()=='F';}
        if(elf||f.getName().endsWith(".conf")||f.getName().equals(".asoundrc")){byte[] data=Files.readAllBytes(f.toPath());boolean changed=false;
            for(String prefix:new String[]{"/data/data/com.winlator/files/rootfs","/data/user/0/com.winlator/files/rootfs"}){
                byte[] old=prefix.getBytes(StandardCharsets.US_ASCII);String replacement=alias;while(replacement.length()<old.length)replacement+="/";
                if(replacement.length()!=old.length)throw new IOException("Private ELF alias too long");byte[] next=replacement.getBytes(StandardCharsets.US_ASCII);
                for(int i=0;i<=data.length-old.length;i++){if(data[i]!=old[0])continue;int j=1;while(j<old.length&&data[i+j]==old[j])j++;if(j==old.length){System.arraycopy(next,0,data,i,next.length);changed=true;i+=old.length-1;}}
            }
            if(changed){Files.write(f.toPath(),data);report("Relocated guest path: "+f.getName());}
        }
        relocated.put(f.getPath(),f.lastModified());
    }
    public static String status(){return state+"\n"+audioPolicyStatus();}
    public static void exportDiagnostics(Context c){
        if(!c.getSharedPreferences("vr_graphics",Context.MODE_PRIVATE).getBoolean("diagnostic_logging",false))return;
        for(String name:new String[]{"standalone.log","standalone-native.log","guest-launch.log"})try{
            File src=new File(c.getFilesDir(),name);if(!src.isFile())continue;
            try(RandomAccessFile in=new RandomAccessFile(src,"r");FileOutputStream out=new FileOutputStream(new File(c.getExternalFilesDir(null),"diagnostic-"+name))){long start=Math.max(0,in.length()-262144);in.seek(start);byte[] b=new byte[8192];int n;while((n=in.read(b))>0)out.write(b,0,n);}
        }catch(Exception e){Log.w("MythStandalone","Diagnostic export failed",e);}
    }
    private static void report(String s){state=s;Log.i("MythStandalone",s);try(FileWriter w=new FileWriter(new File(base,"files/standalone.log"),true)){w.write(System.currentTimeMillis()+" "+s+"\n");}catch(Exception ignored){}}
    private static void copy(Context c,String name,File target)throws Exception{
        target.getParentFile().mkdirs();byte[] data;try(InputStream in=c.getAssets().open("standalone/"+name);ByteArrayOutputStream out=new ByteArrayOutputStream()){byte[] b=new byte[65536];int n;while((n=in.read(b))>0)out.write(b,0,n);data=out.toByteArray();}if(target.isFile()&&java.util.Arrays.equals(data,Files.readAllBytes(target.toPath())))return;Files.write(target.toPath(),data);
    }
    public static synchronized void prepare(Context c)throws Exception{
        base=c.getFilesDir().getParentFile();
        watchActivities(c);audioContext=c.getApplicationContext();Log.i("MythStandalone",audioPolicyStatus());
        File dir=new File(base,"files/myth-bridge");dir.mkdirs();
        File guest=new File(base,"files/rootfs/home/xuser-1/.wine/drive_c/mythvr");guest.mkdirs();
        copy(c,"borderless.exe",new File(guest,"borderless.exe"));copy(c,"mythinput.exe",new File(guest,"mythinput.exe"));copy(c,"mythinput-v7.dll",new File(guest,"mythinput-v7.dll"));
        File token=new File(base,"files/bridge.token");
        if(!token.isFile()||token.length()!=32){byte[] bytes=new byte[16];new SecureRandom().nextBytes(bytes);StringBuilder hex=new StringBuilder();for(byte b:bytes)hex.append(String.format("%02x",b&255));Files.write(token.toPath(),hex.toString().getBytes(StandardCharsets.US_ASCII));}
        Files.write(new File(dir,"bridge.token").toPath(),Files.readAllBytes(token.toPath()));
        // Mailbox is private to this application, never a shared-storage command channel.
        report("Prepared private bridge and guest input helper");
    }
    private static Process run(Context c,String tool,String... args)throws Exception{
        String[] command=new String[args.length+1];command[0]=c.getApplicationInfo().nativeLibraryDir+"/lib"+tool+".so";System.arraycopy(args,0,command,1,args.length);
        return new ProcessBuilder(command).directory(base).redirectErrorStream(true).redirectOutput(ProcessBuilder.Redirect.appendTo(new File(base,"files/standalone-native.log"))).start();
    }
    private static int findGame(){
        File[] all=new File("/proc").listFiles();if(all==null)return -1;
        for(File f:all){if(!f.getName().matches("[0-9]+"))continue;
            try{byte[] b=Files.readAllBytes(new File(f,"cmdline").toPath());int n=0;while(n<b.length&&b[n]!=0)n++;String name=new String(b,0,n,StandardCharsets.UTF_8);if(name.equals("Myth II.exe"))return Integer.parseInt(f.getName());}catch(Exception ignored){}
        }return -1;
    }
    private static String activeLevel="";private static long activeMesh;private static String[] activeDefinition;
    private static java.util.List<String[]> levelCatalog;
    private static String[] findLevel(Context c,int pid){
        try(RandomAccessFile f=new RandomAccessFile("/proc/"+pid+"/mem","r")){
            if(levelCatalog==null)levelCatalog=LevelAssets.catalog(c);
            long mesh=Integer.toUnsignedLong(intAt(f,EngineProfile.MESH));if(mesh<65536)return null;
            long landscape=Integer.toUnsignedLong(intAt(f,mesh));int w=shortAt(f,mesh+8)*32,h=shortAt(f,mesh+10)*32;
            String fingerprint=null;java.util.List<String[]> matches=new java.util.ArrayList<>();
            for(String[] v:levelCatalog)if(Long.parseLong(v[1])==landscape&&Integer.parseInt(v[2])==w&&Integer.parseInt(v[3])==h){
                if(fingerprint==null){int[] offsets={0,4,8,16,36,44,52,60,68,72,80,128,136,140,144,148,152,156,160,176,252,256,284,292};java.nio.ByteBuffer bytes=java.nio.ByteBuffer.allocate(offsets.length*4).order(java.nio.ByteOrder.LITTLE_ENDIAN);for(int off:offsets)bytes.putInt(intAt(f,mesh+off));byte[] hash=java.security.MessageDigest.getInstance("SHA-256").digest(bytes.array());StringBuilder text=new StringBuilder();for(byte x:hash)text.append(String.format(java.util.Locale.ROOT,"%02x",x&255));fingerprint=text.toString();}
                if(v[9].equals(fingerprint))matches.add(v);
            }
            // Plugin activation changes occur between maps. Keep the already verified
            // selection during play instead of rereading archive tables every 500 ms.
            if(opened&&mesh==activeMesh&&matches.contains(activeDefinition)&&Integer.toUnsignedLong(intAt(f,EngineProfile.MESH))==mesh)return activeDefinition;
            ActivePlugins.Snapshot loaded=null;
            if(!matches.isEmpty()&&EngineProfile.ACTIVE_ARCHIVES!=0)loaded=ActivePlugins.read((address,size)->{byte[] bytes=new byte[size];f.seek(address);f.readFully(bytes);return bytes;},EngineProfile.ACTIVE_ARCHIVES);
            String[] match=LevelAssets.select(matches,loaded);
            if(Integer.toUnsignedLong(intAt(f,EngineProfile.MESH))==mesh)return match;
        }catch(Exception e){report("Map identification failed: "+e);}return null;
    }
    private static long meshPointer(int pid){try(RandomAccessFile f=new RandomAccessFile("/proc/"+pid+"/mem","r")){return Integer.toUnsignedLong(intAt(f,EngineProfile.MESH));}catch(Exception e){return 0;}}
    private static void prepareLevel(Context c,int pid,String[] level)throws Exception{
        level=LevelAssets.activate(c,level);
        if(EngineProfile.ACTIVE_ARCHIVES!=0){
            try(RandomAccessFile memory=new RandomAccessFile("/proc/"+pid+"/mem","r")){
                ActivePlugins.Snapshot loaded=ActivePlugins.read((address,size)->{byte[] bytes=new byte[size];memory.seek(address);memory.readFully(bytes);return bytes;},EngineProfile.ACTIVE_ARCHIVES);
                org.json.JSONObject evidence=loaded.json();
                File external=LevelAssets.external.get(level[0]);
                org.json.JSONObject requirements=external!=null&&LevelAssets.active!=null?PluginSources.requirements(external,LevelAssets.manifest(external,level)):null;
                try{ActivePlugins.verify(loaded,requirements,ActivePlugins.resourceAddons(DetailTextures.plugins()));evidence.put("verification",requirements!=null&&requirements.has("active_plugins")?"matched":"diagnostic only");}
                catch(Exception mismatch){evidence.put("verification",mismatch.toString());throw mismatch;}
                finally{Files.write(new File(c.getExternalFilesDir(null),"active-plugins-diagnostic.json").toPath(),evidence.toString(2).getBytes(StandardCharsets.UTF_8));}
                report("Loaded archives: "+loaded.archives.toString());
            }
        }
        DetailTextures.prepare(c,level);
        HDCollections.prepare(c,level);
        byte[] table;try(InputStream in=LevelAssets.open(c,level[0],"collections.bin");ByteArrayOutputStream out=new ByteArrayOutputStream()){byte[] b=new byte[4096];int n;while((n=in.read(b))>0)out.write(b,0,n);table=out.toByteArray();}
        CollectionManifest source=new CollectionManifest(table);int count=source.count;
        int[] mapping=new int[256];java.util.Arrays.fill(mapping,255);int matched=0;
        try(RandomAccessFile f=new RandomAccessFile("/proc/"+pid+"/mem","r")){
            long mesh=Integer.toUnsignedLong(intAt(f,EngineProfile.MESH)),pool=Integer.toUnsignedLong(intAt(f,EngineProfile.COLLECTIONS));
            if(mesh<65536||pool<65536)throw new IOException("map unavailable");
            long ids=0;int loaded=255;
            if(source.tagged){
                if(EngineProfile.COLLECTION_IDS==0||EngineProfile.COLLECTION_COUNT==0)throw new IOException("tag binding unsupported by engine profile");
                ids=Integer.toUnsignedLong(intAt(f,EngineProfile.COLLECTION_IDS));loaded=shortAt(f,EngineProfile.COLLECTION_COUNT);
                if(ids<65536||loaded<1||loaded>256)throw new IOException("collection ID registry unavailable");
            }
            int[] offsets={64,96,112,128,252};
            for(int runtime=0;runtime<Math.min(loaded,255);runtime++)for(int i=0;i<count;i++){
                if(source.tagged&&intAt(f,ids+runtime*4)!=source.tag(i))continue;
                boolean same=true;for(int j=0;j<5;j++)if(intAt(f,pool+runtime*320+offsets[j])!=source.field(i,j)){same=false;break;}
                if(same){
                    long refs=Integer.toUnsignedLong(intAt(f,pool+runtime*320+140));int sequences=source.field(i,3),hash=0x811c9dc5;
                    if(sequences<0||sequences>1024||refs<65536)throw new IOException("sequence binding bounds");
                    byte[] row=new byte[72];for(int seq=0;seq<sequences;seq++){f.seek(refs+seq*128);f.readFully(row);for(int k=0;k<72;k++){int value=row[k]&255;if(k<64&&value>='A'&&value<='Z')value+=32;hash=(hash^value)*16777619;}}
                    same=hash==source.field(i,5);
                }
                if(source.tagged&&!same)throw new IOException("collection tag matched but source differs: "+Integer.toHexString(source.tag(i)));
                if(same){int canonical=source.canonical(i);if(mapping[runtime]!=255&&mapping[runtime]!=canonical)throw new IOException("ambiguous collection");mapping[runtime]=canonical;matched++;}
            }
            if(source.tagged&&(Integer.toUnsignedLong(intAt(f,EngineProfile.COLLECTION_IDS))!=ids||shortAt(f,EngineProfile.COLLECTION_COUNT)!=loaded||Integer.toUnsignedLong(intAt(f,EngineProfile.COLLECTIONS))!=pool))throw new IOException("collection registry changed during binding");
            if(source.tagged)report("Collection binding: original tag IDs, loaded="+loaded+", matched="+matched);
            if(Integer.toUnsignedLong(intAt(f,EngineProfile.MESH))!=mesh||matched==0)throw new IOException("map changed during asset binding");
            if(c.getSharedPreferences("vr_graphics",Context.MODE_PRIVATE).getBoolean("diagnostic_logging",false)){
                try(FileOutputStream dump=new FileOutputStream(new File(c.getExternalFilesDir(null),"collections-diagnostic.bin"))){
                    for(int runtime=0;runtime<256;runtime++){byte[] head=new byte[320];f.seek(pool+runtime*320);f.readFully(head);dump.write(head);}
                    java.nio.ByteBuffer map=java.nio.ByteBuffer.allocate(1024).order(java.nio.ByteOrder.LITTLE_ENDIAN);for(int value:mapping)map.putInt(value);dump.write(map.array());
                }
                report("Collection binding diagnostic exported");
                if(EngineProfile.COLLECTION_IDS!=0){
                    int n=shortAt(f,EngineProfile.COLLECTION_COUNT);long p=Integer.toUnsignedLong(intAt(f,EngineProfile.COLLECTION_IDS));
                    if(n>0&&n<=256&&p>=65536){byte[] tags=new byte[n*4];f.seek(p);f.readFully(tags);try(FileOutputStream dump=new FileOutputStream(new File(c.getExternalFilesDir(null),"collection-tags-diagnostic.bin"))){dump.write(java.nio.ByteBuffer.allocate(4).order(java.nio.ByteOrder.LITTLE_ENDIAN).putInt(n).array());dump.write(tags);}}
                }
            }
            activeMesh=mesh;
        }
        java.nio.ByteBuffer b=java.nio.ByteBuffer.allocate(632).order(java.nio.ByteOrder.LITTLE_ENDIAN);
        b.put("MYTHLV01".getBytes(StandardCharsets.US_ASCII));b.put(level[0].getBytes(StandardCharsets.US_ASCII));b.putInt(Integer.parseInt(level[2]));b.putInt(Integer.parseInt(level[3]));
        for(int i=4;i<=7;i++)b.putFloat(Float.parseFloat(level[i]));byte[] name=level[8].getBytes(StandardCharsets.UTF_8);if(name.length>=64)throw new IOException("level name too long");b.put(name);b.position(104);for(int v:mapping)b.putShort((short)v);
        // Read the active original camera after startup/cutscene, not a team-zero marker average.
        try(RandomAccessFile f=new RandomAccessFile("/proc/"+pid+"/mem","r")){
            if(Integer.toUnsignedLong(intAt(f,EngineProfile.MESH))!=activeMesh)throw new IOException("map changed before camera capture");
            for(int off:new int[]{0,4,8,28}){float value=Float.intBitsToFloat(intAt(f,0x54e43c+off));if(Float.isNaN(value)||Float.isInfinite(value))throw new IOException("invalid original camera");if(off==0)value=Math.max(.5f,Math.min(Integer.parseInt(level[2])-1.5f,value));if(off==4)value=Math.max(.5f,Math.min(Integer.parseInt(level[3])-1.5f,value));b.putFloat(value);}
        }
        android.util.AtomicFile file=new android.util.AtomicFile(new File(c.getFilesDir(),"active-level.bin"));FileOutputStream out=file.startWrite();try{out.write(b.array());file.finishWrite(out);}catch(Exception e){file.failWrite(out);throw e;}
        LevelAssets.publish(c);
        activeLevel=level[0];activeDefinition=level;report("Level prepared: "+level[8]+" "+level[2]+"x"+level[3]+" collections="+matched);
    }
    private static boolean simulationStarted(int pid){try(RandomAccessFile f=new RandomAccessFile("/proc/"+pid+"/mem","r")){
        return shortAt(f,EngineProfile.GAME_STATE)==3&&intAt(f,EngineProfile.TICK)>=30&&(intAt(f,0x67ec10)&0x32)==0;
    }catch(Exception e){return false;}}
    public static synchronized void onEngine(Activity activity){
        PanelKeyboard.attach(activity);
        OriginalDialogs.attach(activity);
        if(monitoring)return;monitoring=true;opened=false;levelCatalog=null;
        Context c=activity.getApplicationContext();
        new Thread(()->{int previousPid=-1,borderlessPid=-1,attempt=0;byte[] lastWindowReport=null;try{
            prepare(c);report("Waiting for an original supported map");
            while(monitoring){
                int pid=findGame();
                if(pid>0&&pid!=borderlessPid){run(c,"myth_exec","C:\\mythvr\\borderless.exe").waitFor();borderlessPid=pid;report("Started borderless window helper pid="+pid);}
                File windowReport=new File(c.getFilesDir(),"rootfs/home/xuser-1/.wine/drive_c/mythvr/borderless.bin");
                if(windowReport.isFile()){byte[] data=Files.readAllBytes(windowReport.toPath());if(data.length==40&&!java.util.Arrays.equals(data,lastWindowReport)){lastWindowReport=data;java.nio.ByteBuffer b=java.nio.ByteBuffer.wrap(data).order(java.nio.ByteOrder.LITTLE_ENDIAN);report("Borderless result="+b.getInt(12)+" changes="+b.getInt(8)+" style="+Integer.toHexString(b.getInt(16))+" client="+b.getInt(24)+"x"+b.getInt(28)+" origin="+b.getInt(32)+","+b.getInt(36));}}
                int currentState=pid>0?gameState(pid):-1;
                if(opened&&(pid<0||currentState>=0&&currentState!=3))returnToPanel(activity,"state="+currentState+" pid="+pid);
                if(pid<0&&++attempt%10==0){File log=new File(base,"files/guest-launch.log");if(log.isFile()){String text=new String(Files.readAllBytes(log.toPath()),StandardCharsets.UTF_8);report("Guest startup: "+text.substring(Math.max(0,text.length()-3000)));}}
                String[] level=pid>0&&currentState==3?findLevel(c,pid):null;
                if(opened&&(level==null||!level[0].equals(activeLevel)||meshPointer(pid)!=activeMesh))returnToPanel(activity,"map changed");
                if(pid>0&&currentState==3&&level!=null){
                    if(c.getSharedPreferences("vr_graphics",Context.MODE_PRIVATE).getBoolean("diagnostic_logging",false))diagnose(pid);
                    if(pid!=previousPid){run(c,"myth_exec","C:\\mythvr\\mythinput.exe").waitFor();previousPid=pid;report("Original map ready pid="+pid);}
                    if(!opened&&simulationStarted(pid)){prepareLevel(c,pid,level);opened=true;new Handler(Looper.getMainLooper()).post(()->openVR(activity));Thread.sleep(500);}
                    if(opened&&(capture==null||!capture.isAlive())){
                        if(capture!=null)report("Capture exited code="+capture.exitValue());
                        capture=run(c,"myth_capture",Integer.toString(pid),"@mythvr","0","33",activeDefinition[1],Long.toString(activeMesh));report("Started local capture pid="+pid);
                    }
                }else if(previousPid>0&&pid<0){report("Original game exited");previousPid=-1;opened=false;}
                Thread.sleep(opened?500:1500);
            }
        }catch(Exception e){report("Bridge failed: "+e);}finally{monitoring=false;if(capture!=null)capture.destroy();}},"MythStandalone").start();
    }
    public static void openVR(Activity activity){
        int pid=findGame();
        if(base==null||activeDefinition==null||pid<0||gameState(pid)!=3||meshPointer(pid)!=activeMesh){state="Start a supported original map first";return;}
        Intent i=new Intent(Intent.ACTION_MAIN);i.addCategory("com.oculus.intent.category.VR");i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);i.setClassName(activity,"android.app.NativeActivity");transition(activity,"VR-startActivity-begin");activity.startActivity(i);transition(activity,"VR-startActivity-return");report("Requested immersive VR");
    }
    static boolean dialogGameRunning(){int pid=findGame();return pid>0&&simulationStarted(pid);}
    static void dialogToPanel(Activity engine,String reason){returnToPanel(engine,reason);}
    public static synchronized void stop(){monitoring=false;if(capture!=null)capture.destroy();}
}
