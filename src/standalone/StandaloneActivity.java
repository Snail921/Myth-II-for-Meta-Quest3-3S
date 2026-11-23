package org.mythvr.standalone;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.widget.*;
import java.io.*;

/** Development launcher. Initial data provisioning is separate from normal play. */
public final class StandaloneActivity extends Activity implements SeekBar.OnSeekBarChangeListener {
    private void addChatKeyboardOptions(android.widget.LinearLayout group,android.content.SharedPreferences prefs){
        final String[] keys={"chat_keyboard_x","chat_keyboard_y","chat_keyboard_distance"};
        final String[] names={"チャットキーボード 左右 / Chat keyboard horizontal","チャットキーボード 上下 / Chat keyboard vertical","チャットキーボード 距離 / Chat keyboard distance"};
        final int[] low={-60,-100,70},high={60,20,280},defaults={0,-40,175},values=new int[3];
        if(!prefs.getBoolean("chat_keyboard_position_v2",false)){if(prefs.getInt("chat_keyboard_y",-35)==-35)prefs.edit().putInt("chat_keyboard_y",-40).apply();prefs.edit().putBoolean("chat_keyboard_position_v2",true).apply();}
        final int[] fixed={prefs.getBoolean("chat_keyboard_fixed",true)?1:0},smooth={Math.max(0,Math.min(986,prefs.getInt("chat_keyboard_smoothing",493)))};
        for(int i=0;i<3;i++)values[i]=Math.max(low[i],Math.min(high[i],prefs.getInt(keys[i],defaults[i])));
        final Runnable save=()->{try(java.io.FileOutputStream f=new java.io.FileOutputStream(new java.io.File(getFilesDir(),"chat-keyboard.offset"))){f.write((values[0]+" "+values[1]+" "+values[2]+" "+fixed[0]+" "+smooth[0]+" ").getBytes("US-ASCII"));}catch(java.io.IOException e){android.util.Log.e("MythVR","Chat keyboard settings",e);}};
        save.run();
        android.util.Log.i("MythKeyboard","Settings x="+values[0]+" y="+values[1]+" distance="+values[2]+" fixed="+fixed[0]+" smoothing="+smooth[0]);
        Switch fixedMode=new Switch(this);fixedMode.setText("チャットキーボードを空間に固定 / Fixed Chat Keyboard");fixedMode.setChecked(fixed[0]==1);group.addView(fixedMode);
        fixedMode.setOnCheckedChangeListener((button,on)->{fixed[0]=on?1:0;prefs.edit().putBoolean("chat_keyboard_fixed",on).apply();save.run();});
        TextView smoothLabel=new TextView(this);group.addView(smoothLabel);smoothLabel.setText("視線追従スムージング / Follow smoothing: "+(smooth[0]-493)+" ms");
        SeekBar smoothing=new ValueSeekBar();smoothing.setTag("keyboard-smoothing");smoothing.setMax(986);smoothing.setProgress(smooth[0]);group.addView(smoothing);
        smoothing.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener(){
            public void onProgressChanged(SeekBar b,int v,boolean user){smooth[0]=v;smoothLabel.setText("視線追従スムージング / Follow smoothing: "+(v-493)+" ms");}
            public void onStartTrackingTouch(SeekBar b){}
            public void onStopTrackingTouch(SeekBar b){prefs.edit().putInt("chat_keyboard_smoothing",smooth[0]).apply();save.run();}
        });
        for(int i=0;i<3;i++){
            final int index=i;TextView label=new TextView(this);label.setTextSize(18);group.addView(label);
            label.setText(names[i]+": "+(values[i]-defaults[i])+" cm");
            SeekBar slider=new ValueSeekBar();slider.setTag("keyboard-offset:"+(low[i]-defaults[i]));slider.setMax(high[i]-low[i]);slider.setProgress(values[i]-low[i]);group.addView(slider);
            slider.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener(){
                public void onProgressChanged(SeekBar b,int v,boolean user){values[index]=v+low[index];label.setText(names[index]+": "+(values[index]-defaults[index])+" cm");}
                public void onStartTrackingTouch(SeekBar b){}
                public void onStopTrackingTouch(SeekBar b){prefs.edit().putInt(keys[index],values[index]).apply();save.run();}
            });
        }
    }

    private String ui(String text){return UiLanguage.text(this,text);}
    private TextView skyLabel,soundLabel,shadowAmountLabel,lightingAmountLabel,lightingLimitLabel,lightingObjectLabel,lightingLodLabel;
    private SeekBar shadowAmount,lightingAmount,lightingLimit,lightingObject,lightingLod,shadowSamples,shadowLod;
    private TextView shadowSamplesLabel,shadowLodLabel;
    private SeekBar pointerThickness,soundRange,mapSize,mapPitch,orbitDistance,radialSize,tapDistance;
    private TextView mapSizeLabel,mapPitchLabel,orbitDistanceLabel,radialSizeLabel,tapDistanceLabel;
    private final SeekBar[] cameraSpeeds=new SeekBar[2];
    private final TextView[] cameraLabels=new TextView[2];
    private static final String[] CAMERA_KEYS={"camera_move","camera_stick_turn"};
    private static final String[] CAMERA_NAMES={"カメラ移動（水平・上下）","スティック回転（オービット・パン）"};
    private Switch reuseFollowDistance,aimOnJump,tapGroups,selectionHaptics,buttonGroups;
    private TextView status;
    private volatile boolean importing;
    private void addSpritePbrDiagnostics(LinearLayout group,android.content.SharedPreferences prefs){
        Switch debug=new Switch(this);debug.setText("DebugSpecularLightingMode");debug.setTextSize(20);
        debug.setChecked(prefs.getBoolean("sprite-pbr.debug-specular",false));group.addView(debug);
        saveLighting("sprite-pbr.debug-specular",debug.isChecked()?1:0);
        Switch debugSss=new Switch(this);debugSss.setText("DebugSSSLightingMode");debugSss.setTextSize(20);
        debugSss.setChecked(prefs.getBoolean("sprite-pbr.debug-sss",false));group.addView(debugSss);
        if(debugSss.isChecked()){debug.setChecked(false);saveLighting("sprite-pbr.debug-specular",0);prefs.edit().putBoolean("sprite-pbr.debug-specular",false).apply();}
        saveLighting("sprite-pbr.debug-sss",debugSss.isChecked()?1:0);
        debug.setOnCheckedChangeListener((button,on)->{if(saveLighting("sprite-pbr.debug-specular",on?1:0)){prefs.edit().putBoolean("sprite-pbr.debug-specular",on).apply();if(on)debugSss.setChecked(false);}});
        debugSss.setOnCheckedChangeListener((button,on)->{if(saveLighting("sprite-pbr.debug-sss",on?1:0)){prefs.edit().putBoolean("sprite-pbr.debug-sss",on).apply();if(on)debug.setChecked(false);}});
    }
    private void addSpritePbrSettings(LinearLayout group,android.content.SharedPreferences prefs){
        String[] categoryKeys={"sprite-pbr.units","sprite-pbr.scene-objects"};
        String[] categoryNames={"ユニットに疑似PBRシェーダーを使用 / Use PBR-like Shader for Units","シーンオブジェクトに疑似PBRシェーダーを使用 / Use PBR-like Shader for Scene objects"};
        for(int i=0;i<categoryKeys.length;i++){
            final String key=categoryKeys[i];Switch toggle=new Switch(this);toggle.setText(ui(categoryNames[i]));toggle.setTextSize(20);
            toggle.setChecked(prefs.getBoolean(key,false));group.addView(toggle);saveLighting(key,toggle.isChecked()?1:0);
            toggle.setOnCheckedChangeListener((button,on)->{if(saveLighting(key,on?1:0))prefs.edit().putBoolean(key,on).apply();});
        }

        Switch meshes=new Switch(this);meshes.setText(ui("メッシュに疑似PBRシェーダーを使用 / Use PBR-like Shader for Meshes"));meshes.setTextSize(20);
        meshes.setChecked(prefs.getBoolean("sprite-pbr.meshes",false));group.addView(meshes);
        saveLighting("sprite-pbr.meshes",meshes.isChecked()?1:0);
        meshes.setOnCheckedChangeListener((button,on)->{if(saveLighting("sprite-pbr.meshes",on?1:0))prefs.edit().putBoolean("sprite-pbr.meshes",on).apply();});

        final String excludeKey="sprite-pbr.exclude-mesh-lod";
        Switch exclude=new Switch(this);
        exclude.setText(ui("疑似PBRメッシュをLODから除外 / Exclude PBR-like Mesh from LOD"));
        exclude.setTextSize(20);
        exclude.setChecked(prefs.getBoolean(excludeKey,false));
        group.addView(exclude);
        saveLighting(excludeKey,exclude.isChecked()?1:0);
        exclude.setOnCheckedChangeListener((button,on)->{if(saveLighting(excludeKey,on?1:0))prefs.edit().putBoolean(excludeKey,on).apply();});
        addPbrDistance(group,prefs,"sprite-pbr.lod-start","疑似PBR LOD開始距離 / PBR-like LOD start distance",20,250);
        addPbrDistance(group,prefs,"sprite-pbr.lod-width","疑似PBR LODスムージング / PBR-like LOD smoothing",4,250);
        String[] keys={"sprite-pbr.reflection","sprite-pbr.specular","sprite-pbr.sss","sprite-pbr.sss-minimum"};
        String[] names={"最大映り込みブレンド / Maximum reflection blend","スペキュラ強度 / Specular strength","SSS強度 / SSS strength","SSSの最低量 / Minimum Amount of SSS"};
        int[] maximum={100,1600,400,100},defaults={50,800,200,35};
        for(int i=0;i<keys.length;i++){
            final String key=keys[i];TextView label=new TextView(this);label.setText(ui(names[i]));group.addView(label);
            SeekBar slider=new ValueSeekBar();slider.setTag(key);slider.setMax(maximum[i]);slider.setProgress(Math.max(0,Math.min(maximum[i],prefs.getInt(key,defaults[i]))));group.addView(slider);
            saveLighting(key,slider.getProgress());
            slider.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener(){public void onProgressChanged(SeekBar b,int v,boolean user){b.invalidate();}public void onStartTrackingTouch(SeekBar b){}public void onStopTrackingTouch(SeekBar b){if(saveLighting(key,b.getProgress()))prefs.edit().putInt(key,b.getProgress()).apply();}});
        }
        TextView help=new TextView(this);help.setText(ui("材質対応パックとスカイボックスが必要。Thicknessが小さいほど光を透過します。次回VR起動時に反映。 / Requires a material pack and skybox. Lower Thickness transmits more light. Applies on next VR entry."));group.addView(help);
    }
    private void addPbrDistance(LinearLayout group,android.content.SharedPreferences prefs,String key,String title,int minimum,int maximum){
        TextView label=new TextView(this);label.setText(ui(title));group.addView(label);
        SeekBar slider=new ValueSeekBar();slider.setTag(key);slider.setMax(maximum-minimum);
        slider.setProgress(Math.max(minimum,Math.min(maximum,prefs.getInt(key,minimum)))-minimum);
        group.addView(slider);saveLighting(key,minimum+slider.getProgress());
        slider.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener(){
            public void onProgressChanged(SeekBar bar,int value,boolean user){bar.invalidate();}
            public void onStartTrackingTouch(SeekBar bar){}
            public void onStopTrackingTouch(SeekBar bar){int meters=minimum+bar.getProgress();if(saveLighting(key,meters))prefs.edit().putInt(key,meters).apply();}
        });
    }
    private void addBloomSettings(LinearLayout group,android.content.SharedPreferences prefs){
        Switch toggle=new Switch(this);toggle.setText(ui("Bloomを有効にする / Enable Bloom"));toggle.setTextSize(20);toggle.setChecked(prefs.getBoolean("bloom_enabled",false));group.addView(toggle);
        String[] keys={"bloom_resolution","bloom_iterations","bloom_strength"};
        String[] files={"bloom.resolution","bloom.iterations","bloom.strength"};
        String[] names={"処理解像度 / Processing resolution","ぼかし反復回数 / Blur iterations","発光強度 / Bloom strength"};
        int[] minimum={0,1,0},maximum={5,4,200},defaults={0,1,15};
        // Keep persisted resolution IDs stable; only reorder their slider positions.
        final int[] resolutionIds={5,4,0,1,2,3};
        SeekBar[] sliders=new SeekBar[3];
        for(int i=0;i<3;i++){
            final int index=i;TextView label=new TextView(this);label.setText(ui(names[i]));group.addView(label);
            SeekBar slider=new ValueSeekBar();slider.setTag(keys[i]);slider.setMax(maximum[i]-minimum[i]);slider.setProgress(Math.max(minimum[i],Math.min(maximum[i],prefs.getInt(keys[i],defaults[i])))-minimum[i]);if(i==0){int saved=slider.getProgress();for(int pos=0;pos<resolutionIds.length;pos++)if(resolutionIds[pos]==saved){slider.setProgress(pos);break;}}slider.setEnabled(toggle.isChecked());group.addView(slider);sliders[i]=slider;
            saveLighting(files[i],i==0?resolutionIds[slider.getProgress()]:slider.getProgress()+minimum[i]);
            slider.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener(){public void onProgressChanged(SeekBar b,int v,boolean user){b.invalidate();}public void onStartTrackingTouch(SeekBar b){}public void onStopTrackingTouch(SeekBar b){int value=index==0?resolutionIds[b.getProgress()]:b.getProgress()+minimum[index];if(saveLighting(files[index],value))prefs.edit().putInt(keys[index],value).apply();}});
        }
        saveLighting("bloom.enabled",toggle.isChecked()?1:0);
        toggle.setOnCheckedChangeListener((button,on)->{if(saveLighting("bloom.enabled",on?1:0)){prefs.edit().putBoolean("bloom_enabled",on).apply();for(SeekBar slider:sliders)slider.setEnabled(on);}});
        TextView help=new TextView(this);help.setText(ui("空とUIを発光源から除外。解像度は縦横それぞれの比率。高解像度・多い反復回数ほど負荷が増えます。次回VR起動時に反映。 / Excludes sky and UI as glow sources. Resolution scales each dimension. Higher resolution and more iterations increase GPU load. Applies on next VR entry."));group.addView(help);
    }
    // Bounded, persistent lifecycle evidence survives sleep and process restart.
    private void recordLauncherLifecycle(String event){
        try{
            android.os.PowerManager power=(android.os.PowerManager)getSystemService(POWER_SERVICE);
            String line=System.currentTimeMillis()+" uptime="+android.os.SystemClock.uptimeMillis()+" pid="+android.os.Process.myPid()+" event="+event+" finishing="+isFinishing()+" changingConfig="+isChangingConfigurations()+" interactive="+(power!=null&&power.isInteractive())+"\n";
            android.util.Log.i("MythLauncher",line.trim());
            File dir=getExternalFilesDir(null);if(dir==null)return;
            File log=new File(dir,"launcher-lifecycle.log");
            if(log.length()>131072){File old=new File(dir,"launcher-lifecycle.previous.log");if(old.exists()&&!old.delete())return;if(!log.renameTo(old))return;}
            try(FileOutputStream out=new FileOutputStream(log,true)){out.write(line.getBytes(java.nio.charset.StandardCharsets.UTF_8));}
        }catch(Exception e){android.util.Log.w("MythLauncher","Could not record lifecycle",e);}
    }
    @Override protected void onStart(){super.onStart();recordLauncherLifecycle("start");}
    @Override protected void onResume(){super.onResume();recordLauncherLifecycle("resume");}
    @Override protected void onPause(){recordLauncherLifecycle("pause");super.onPause();}
    @Override protected void onStop(){recordLauncherLifecycle("stop");super.onStop();}
    @Override protected void onDestroy(){recordLauncherLifecycle("destroy");super.onDestroy();}
    @Override protected void onSaveInstanceState(Bundle state){recordLauncherLifecycle("save-state");super.onSaveInstanceState(state);}
    @Override public void onTrimMemory(int level){recordLauncherLifecycle("trim-memory-"+level);super.onTrimMemory(level);}
    @Override public void onLowMemory(){recordLauncherLifecycle("low-memory");super.onLowMemory();}
    @Override public void onWindowFocusChanged(boolean focused){super.onWindowFocusChanged(focused);recordLauncherLifecycle("focus-"+focused);}
    @Override public void onCreate(Bundle state) {
        super.onCreate(state);
        recordLauncherLifecycle(state==null?"create":"recreate");
        try{java.nio.file.Files.write(new File(getExternalFilesDir(null),"external-layout.version").toPath(),new byte[]{'1'});}catch(IOException e){android.util.Log.e("MythStandalone","Layout marker",e);}
        final String purgeHash=getIntent().getStringExtra("purge_level_cache_sha256");
        if(purgeHash!=null)new Thread(()->{try{LevelAssets.purgeMatchingCache(this,purgeHash);}catch(Exception e){android.util.Log.e("MythStandalone","Cache removal failed",e);}},"RemoveLevelCache").start();
        StandaloneBridge.exportDiagnostics(this);
        LinearLayout layout=new LinearLayout(this);layout.setOrientation(1);layout.setPadding(32,32,32,32);
        UiLanguage.selector(this,layout,()->importing);
        status=new TextView(this);status.setTextSize(20);layout.addView(status);
        android.content.SharedPreferences graphics=getSharedPreferences("vr_graphics",MODE_PRIVATE);
        // One-time adoption of the requested defaults; later user changes remain saved.
        if(!graphics.getBoolean("group_defaults_v2",false))graphics.edit().putBoolean("tap_groups",true).putBoolean("button_groups",true).putBoolean("group_defaults_v2",true).commit();
        button(layout,ui("Start Myth II / Myth II を起動"),()->startEngine());
        LinearLayout displayGroup=section(layout,"display","解像度・フレームレート","Resolution & frame rate",true);
        DisplaySettings.add(this,displayGroup);
        Switch fpsDisplay=new Switch(this);fpsDisplay.setText(ui("FPSを表示 / Show FPS"));fpsDisplay.setTextSize(20);fpsDisplay.setChecked(graphics.getBoolean("fps_display",false));
        saveLighting("fps-display.enabled",fpsDisplay.isChecked()?1:0);displayGroup.addView(fpsDisplay);
        fpsDisplay.setOnCheckedChangeListener((button,enabled)->{if(saveLighting("fps-display.enabled",enabled?1:0))graphics.edit().putBoolean("fps_display",enabled).apply();});
        LinearLayout correction=section(layout,"correction","色補正","Color correction",false);
        Switch correctionOn=new Switch(this);correctionOn.setText(ui("色補正を有効にする / Enable color correction"));correctionOn.setTextSize(20);correctionOn.setChecked(graphics.getBoolean("scene_correction_enabled",false));correction.addView(correctionOn);
        TextView correctionLabel=new TextView(this);correctionLabel.setText(ui("補正強度 / Correction strength"));correction.addView(correctionLabel);
        SeekBar correctionStrength=new ValueSeekBar();correctionStrength.setTag("scene-correction");correctionStrength.setMax(100);correctionStrength.setProgress(Math.max(0,Math.min(100,graphics.getInt("scene_correction_strength",100))));correctionStrength.setEnabled(correctionOn.isChecked());correction.addView(correctionStrength);
        saveLighting("scene-correction.enabled",correctionOn.isChecked()?1:0);saveLighting("scene-correction.strength",correctionStrength.getProgress());
        correctionOn.setOnCheckedChangeListener((button,enabled)->{if(saveLighting("scene-correction.enabled",enabled?1:0)){graphics.edit().putBoolean("scene_correction_enabled",enabled).apply();correctionStrength.setEnabled(enabled);}});
        correctionStrength.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener(){public void onProgressChanged(SeekBar b,int v,boolean user){b.invalidate();}public void onStartTrackingTouch(SeekBar b){}public void onStopTrackingTouch(SeekBar b){if(saveLighting("scene-correction.strength",b.getProgress()))graphics.edit().putInt("scene_correction_strength",b.getProgress()).apply();}});
        TextView correctionHelp=new TextView(this);correctionHelp.setText(ui("空とUIを除く明るさ補正。次回VR起動時に反映。 / Brightens the scene, excluding sky and UI. Applies on next VR entry."));correction.addView(correctionHelp);
        addBloomSettings(section(layout,"bloom","ブルーム","Bloom",false),graphics);
        addSpritePbrSettings(section(layout,"sprite-pbr","スプライト材質","Sprite materials",false),graphics);
        LinearLayout water=section(layout,"water","水面","Water",false);
        LinearLayout texturesGroup=section(layout,"textures","テクスチャ・影","Textures & shadows",false);
        LinearLayout weatherGroup=section(layout,"weather","天候","Weather",false);
        LinearLayout audioGroup=section(layout,"audio","音声","Audio",false);
        LinearLayout cameraGroup=section(layout,"camera","カメラ","Camera",false);
        LinearLayout keyboardGroup=section(layout,"keyboard","キーボード","Keyboard",false);
        LinearLayout mapGroup=section(layout,"map","ミニマップ・メニュー","Minimap & menus",false);
        TextView pointerLabel=new TextView(this);pointerLabel.setText("Thick Pointer UI (1.0x - 4.0x)");mapGroup.addView(pointerLabel);
        pointerThickness=new ValueSeekBar();pointerThickness.setMax(30);pointerThickness.setProgress(Math.max(10,Math.min(40,graphics.getInt("pointer_thickness",10)))-10);mapGroup.addView(pointerThickness);
        saveLighting("pointer-thickness",pointerThickness.getProgress()+10);
        pointerThickness.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener(){public void onProgressChanged(SeekBar b,int v,boolean user){b.invalidate();}public void onStartTrackingTouch(SeekBar b){}public void onStopTrackingTouch(SeekBar b){int value=b.getProgress()+10;if(saveLighting("pointer-thickness",value))graphics.edit().putInt("pointer_thickness",value).apply();}});
        Switch autoContrast=new Switch(this);autoContrast.setText("Auto Switch UI/Gizmo Color");autoContrast.setTextSize(20);autoContrast.setChecked(graphics.getBoolean("auto_ui_contrast",false));
        saveLighting("auto-ui-contrast.enabled",autoContrast.isChecked()?1:0);mapGroup.addView(autoContrast);
        autoContrast.setOnCheckedChangeListener((button,enabled)->{if(saveLighting("auto-ui-contrast.enabled",enabled?1:0))graphics.edit().putBoolean("auto_ui_contrast",enabled).apply();});
        int accent=graphics.getInt("auto_ui_color",0xF5B642)&0xFFFFFF;saveLighting("auto-ui-contrast.color",accent);
        Button accentButton=new Button(this);accentButton.setTextSize(20);showAccentButton(accentButton,accent);mapGroup.addView(accentButton);
        accentButton.setOnClickListener(v->pickAccentColor(accentButton));
        TextView contrastHelp=new TextView(this);contrastHelp.setText(ui("明るい地面では黄色の操作表示を設定した強調色に変更。次回VR起動時に反映。 / Uses the selected accent on bright terrain. Applies on next VR entry."));mapGroup.addView(contrastHelp);
        LinearLayout inputGroup=section(layout,"input","操作","Controls",false);
        String[] swapNames={"swap-triggers","swap-grips","swap-ax","swap-yb","swap-sticks","swap-hands"};
        String[] swapLabels={"左右のトリガーを入れ替える / Swap triggers","左右のグリップを入れ替える / Swap grip buttons","A/Xを入れ替える / Swap A/X","Y/Bを入れ替える / Swap Y/B","左右のスティックを入れ替える / Swap sticks","左右のコントローラーを入れ替える / Swap controllers"};
        for(int i=0;i<swapNames.length;i++){
            final String key=swapNames[i];Switch option=new Switch(this);option.setText(ui(swapLabels[i]));option.setChecked(graphics.getBoolean(key,false));inputGroup.addView(option);saveLighting(key+".enabled",option.isChecked()?1:0);
            option.setOnCheckedChangeListener((button,on)->{if(saveLighting(key+".enabled",on?1:0))graphics.edit().putBoolean(key,on).apply();});
        }
        TextView swapHelp=new TextView(this);swapHelp.setText(ui("キーボード操作は変更しません。次回VR起動時に反映。 / Keyboard controls stay physical. Applies on next VR entry."));inputGroup.addView(swapHelp);

        LinearLayout setupGroup=section(layout,"setup","セットアップ・診断","Setup & diagnostics",false);
        Switch reflections=new Switch(this),fullReflections=new Switch(this),highReflections=new Switch(this);
        reflections.setText(ui("鏡面反射 / Water reflections"));fullReflections.setText(ui("フルリフレクション / Full reflections"));highReflections.setText(ui("高精細反射 / Full-resolution reflections"));
        reflections.setChecked(graphics.getBoolean("reflections",true));fullReflections.setChecked(graphics.getBoolean("reflections_full",false));highReflections.setChecked(graphics.getBoolean("reflections_high",false));
        for(Switch option:new Switch[]{reflections,fullReflections,highReflections}){option.setTextSize(20);water.addView(option);}
        fullReflections.setEnabled(reflections.isChecked());highReflections.setEnabled(reflections.isChecked());
        saveReflections(reflections.isChecked(),fullReflections.isChecked(),highReflections.isChecked());
        CompoundButton.OnCheckedChangeListener graphicsChanged=(button,enabled)->{
            if(saveReflections(reflections.isChecked(),fullReflections.isChecked(),highReflections.isChecked()))graphics.edit().putBoolean("reflections",reflections.isChecked()).putBoolean("reflections_full",fullReflections.isChecked()).putBoolean("reflections_high",highReflections.isChecked()).apply();
            fullReflections.setEnabled(reflections.isChecked());highReflections.setEnabled(reflections.isChecked());
        };
        reflections.setOnCheckedChangeListener(graphicsChanged);fullReflections.setOnCheckedChangeListener(graphicsChanged);highReflections.setOnCheckedChangeListener(graphicsChanged);
        Switch waves=new Switch(this);waves.setText(ui("水面の波・光沢 / Water waves & highlights"));waves.setTextSize(20);waves.setChecked(graphics.getBoolean("water_waves",true));saveWaterWaves(waves.isChecked());water.addView(waves);
        waves.setOnCheckedChangeListener((button,enabled)->{if(saveWaterWaves(enabled))graphics.edit().putBoolean("water_waves",enabled).apply();});
        TextView waveHelp=new TextView(this);waveHelp.setText(ui("鏡面反射ON時に有効。波による映り込みの揺れと光沢を追加します。"));water.addView(waveHelp);
        Switch terrainLod=new Switch(this);terrainLod.setText(ui("遠方の地面の詳細テクスチャを簡略化 / Simplify distant terrain detail"));terrainLod.setTextSize(20);terrainLod.setChecked(graphics.getBoolean("terrain_detail_lod",false));
        saveLighting("terrain-detail-lod.enabled",terrainLod.isChecked()?1:0);texturesGroup.addView(terrainLod);
        terrainLod.setOnCheckedChangeListener((button,enabled)->{if(saveLighting("terrain-detail-lod.enabled",enabled?1:0))graphics.edit().putBoolean("terrain_detail_lod",enabled).apply();});
        TextView terrainLodHelp=new TextView(this);terrainLodHelp.setText(ui("距離20～40で詳細模様を薄め、40以遠では省略。地面の色や影は維持。次回VR起動時に反映。 / Fades detail between 20 and 40; skips it beyond 40. Keeps terrain color and shadows. Applies on next VR entry."));texturesGroup.addView(terrainLodHelp);
        Switch detailShadows=new Switch(this);detailShadows.setText(ui("高精細な影 / Detailed shadows"));detailShadows.setTextSize(20);detailShadows.setChecked(graphics.getBoolean("detail_shadows",true));texturesGroup.addView(detailShadows);
        detailShadows.setOnCheckedChangeListener((button,enabled)->graphics.edit().putBoolean("detail_shadows",enabled).apply());
        Switch enhancedShadows=new Switch(this);enhancedShadows.setText(ui("強化された影 / Enhanced shadows"));enhancedShadows.setTextSize(20);enhancedShadows.setChecked(graphics.getBoolean("enhanced_shadows",true));saveEnhancedShadows(enhancedShadows.isChecked());texturesGroup.addView(enhancedShadows);
        enhancedShadows.setOnCheckedChangeListener((button,enabled)->{if(saveEnhancedShadows(enabled))graphics.edit().putBoolean("enhanced_shadows",enabled).apply();});
        shadowAmountLabel=new TextView(this);shadowAmountLabel.setTextSize(18);texturesGroup.addView(shadowAmountLabel);
        shadowAmount=new ValueSeekBar();shadowAmount.setMax(100);shadowAmount.setProgress(Math.max(0,Math.min(100,graphics.getInt("enhanced_shadow_amount",45))));
        shadowAmountLabel.setText(ui("影の最大減光量 / Maximum shadow dimming")+": "+shadowAmount.getProgress()+"%");saveShadowAmount(shadowAmount.getProgress());texturesGroup.addView(shadowAmount);
        shadowAmount.setOnSeekBarChangeListener(this);
        shadowSamplesLabel=new TextView(this);shadowSamplesLabel.setTextSize(18);texturesGroup.addView(shadowSamplesLabel);
        shadowSamples=new ValueSeekBar();shadowSamples.setMax(8);shadowSamples.setProgress(Math.max(1,Math.min(9,graphics.getInt("enhanced_shadow_samples",9)))-1);
        shadowSamplesLabel.setText(ui("影サンプル数 / Shadow samples")+": "+(shadowSamples.getProgress()+1));saveLighting("enhanced-shadow-samples",shadowSamples.getProgress()+1);texturesGroup.addView(shadowSamples);shadowSamples.setOnSeekBarChangeListener(this);
        shadowLodLabel=new TextView(this);shadowLodLabel.setTextSize(18);texturesGroup.addView(shadowLodLabel);
        shadowLod=new ValueSeekBar();shadowLod.setMax(100);shadowLod.setProgress(Math.max(0,Math.min(100,graphics.getInt("enhanced_shadow_lod",0))));
        shadowLodLabel.setText(ui("強化影LOD開始距離 / Shadow LOD start distance")+": "+sliderValue(shadowLod));saveLighting("enhanced-shadow-lod",shadowLod.getProgress());texturesGroup.addView(shadowLod);shadowLod.setOnSeekBarChangeListener(this);
        TextView lodHelp=new TextView(this);lodHelp.setText(ui("0でLODなし。指定距離までは最大品質、遠方・小さいスプライトはサンプルを段階的に削減。次回のVR起動時に反映。"));texturesGroup.addView(lodHelp);

        TextView enhancedHelp=new TextView(this);enhancedHelp.setText(ui("受ける影と簡易セルフシャドウを調整。0%で減光なし。次回のVR起動時に反映されます。"));texturesGroup.addView(enhancedHelp);
        Switch lighting=new Switch(this);lighting.setText(ui("強化されたライティング / Enhanced lighting"));lighting.setTextSize(20);lighting.setChecked(graphics.getBoolean("enhanced_lighting",true));saveLighting("enhanced-lighting.enabled",lighting.isChecked()?1:0);texturesGroup.addView(lighting);
        lighting.setOnCheckedChangeListener((button,enabled)->{if(saveLighting("enhanced-lighting.enabled",enabled?1:0))graphics.edit().putBoolean("enhanced_lighting",enabled).apply();});
        lightingAmountLabel=new TextView(this);lightingAmountLabel.setTextSize(18);texturesGroup.addView(lightingAmountLabel);
        lightingAmount=new ValueSeekBar();lightingAmount.setMax(100);lightingAmount.setProgress(Math.max(0,Math.min(100,graphics.getInt("enhanced_lighting_amount",100))));
        lightingAmountLabel.setText(ui("ライティングの影響度 / Lighting influence")+": "+lightingAmount.getProgress()+"%");saveLighting("enhanced-lighting-amount",lightingAmount.getProgress());texturesGroup.addView(lightingAmount);lightingAmount.setOnSeekBarChangeListener(this);
        lightingLimitLabel=new TextView(this);lightingLimitLabel.setTextSize(18);texturesGroup.addView(lightingLimitLabel);
        lightingLimit=new ValueSeekBar();lightingLimit.setMax(124);lightingLimit.setProgress(Math.max(4,Math.min(128,graphics.getInt("enhanced_lighting_limit",8)))-4);
        lightingLimitLabel.setText(ui("ビュー全体の最大光源数 / Maximum lights per view")+": "+(lightingLimit.getProgress()+4));saveLighting("enhanced-lighting-limit",lightingLimit.getProgress()+4);texturesGroup.addView(lightingLimit);lightingLimit.setOnSeekBarChangeListener(this);
        lightingObjectLabel=new TextView(this);lightingObjectLabel.setTextSize(18);texturesGroup.addView(lightingObjectLabel);
        lightingObject=new ValueSeekBar();lightingObject.setMax(127);lightingObject.setProgress(Math.max(1,Math.min(128,graphics.getInt("enhanced_lighting_per_object",8)))-1);
        lightingObjectLabel.setText(ui("オブジェクトごとの最大光源数 / Maximum lights per object")+": "+(lightingObject.getProgress()+1));saveLighting("enhanced-lighting-per-object",lightingObject.getProgress()+1);texturesGroup.addView(lightingObject);lightingObject.setOnSeekBarChangeListener(this);
        lightingLodLabel=new TextView(this);lightingLodLabel.setTextSize(18);texturesGroup.addView(lightingLodLabel);
        lightingLod=new ValueSeekBar();lightingLod.setMax(100);lightingLod.setProgress(Math.max(0,Math.min(100,graphics.getInt("enhanced_lighting_lod",10))));
        lightingLodLabel.setText(ui("照明LOD開始距離（0で無効） / Lighting LOD start distance (0 disables)")+": "+lightingLod.getProgress());saveLighting("enhanced-lighting-lod",lightingLod.getProgress());texturesGroup.addView(lightingLod);lightingLod.setOnSeekBarChangeListener(this);
        TextView lightingHelp=new TextView(this);lightingHelp.setText(ui("炎・爆発などの色でスプライトを照らし、影を緩和します。次回のVR起動時に反映されます。"));texturesGroup.addView(lightingHelp);
        Switch atlasSprites=new Switch(this);atlasSprites.setText(ui("スプライトのアトラス化・まとめ描画（比較用） / Sprite atlas batching (comparison)"));atlasSprites.setTextSize(20);atlasSprites.setChecked(graphics.getBoolean("sprite_atlas",false));
        saveLighting("sprite-atlas.enabled",atlasSprites.isChecked()?1:0);texturesGroup.addView(atlasSprites);
        atlasSprites.setOnCheckedChangeListener((button,enabled)->{if(saveLighting("sprite-atlas.enabled",enabled?1:0))graphics.edit().putBoolean("sprite_atlas",enabled).apply();});
        Switch cutoutSprites=new Switch(this);cutoutSprites.setText(ui("ハイレゾスプライトの高品質な輪郭（2パス） / High-quality hi-res sprite edges (2 passes)"));cutoutSprites.setTextSize(20);cutoutSprites.setChecked(graphics.getBoolean("sprite_high_quality_edges",false));
        saveLighting("sprite-cutout-one-pass.enabled",cutoutSprites.isChecked()?0:1);texturesGroup.addView(cutoutSprites);
        cutoutSprites.setOnCheckedChangeListener((button,enabled)->{if(saveLighting("sprite-cutout-one-pass.enabled",enabled?0:1))graphics.edit().putBoolean("sprite_high_quality_edges",enabled).apply();});
        TextView cutoutHelp=new TextView(this);cutoutHelp.setText(ui("OFF（標準）：不透明1パスで軽量描画。ON：2パスで輪郭を滑らかに描画（負荷増）。炎・煙などの半透明効果は維持します。次回VR起動時に反映。 / OFF (default): fast opaque cutout in one pass. ON: smooth edges in two passes (higher cost). Fire, smoke and other translucent effects are preserved. Applies on next VR entry."));texturesGroup.addView(cutoutHelp);
        Switch hires=new Switch(this);hires.setText(ui("高解像度スプライト・モデル / Hi-res sprites and models"));hires.setTextSize(20);hires.setChecked(graphics.getBoolean("hires_sprites",true));texturesGroup.addView(hires);
        hires.setOnCheckedChangeListener((button,enabled)->graphics.edit().putBoolean("hires_sprites",enabled).apply());
        Switch details=new Switch(this);details.setText(ui("地形の高精細テクスチャ / Landscape detail textures"));details.setTextSize(20);details.setChecked(graphics.getBoolean("detail_textures",true));texturesGroup.addView(details);
        details.setOnCheckedChangeListener((button,enabled)->graphics.edit().putBoolean("detail_textures",enabled).apply());
        TextView detailHelp=new TextView(this);detailHelp.setText(ui("plugins内の対応アドオンを使用。レベルの読み込み時に反映されます。"));texturesGroup.addView(detailHelp);
        CheckBox randomSky=new CheckBox(this);randomSky.setText(ui("対戦専用：ランダムスカイボックス / Multiplayer matches: random skybox"));randomSky.setTextSize(20);randomSky.setChecked(graphics.getBoolean("multiplayer_random_skybox",false));weatherGroup.addView(randomSky);
        randomSky.setOnCheckedChangeListener((button,enabled)->graphics.edit().putBoolean("multiplayer_random_skybox",enabled).apply());
        TextView randomSkyHelp=new TextView(this);randomSkyHelp.setText(ui("OFF：マップ専用の空。ON：シナリオを含む配置済みの全スカイボックスから対戦ごとに選択。協力プレイ・シナリオには適用しません。次の対戦から反映。 / OFF: map-specific sky. ON: choose from all installed skies, including campaigns, once per match. Does not affect co-op or campaign levels. Applies to the next match."));weatherGroup.addView(randomSkyHelp);
        Switch fog=new Switch(this);fog.setText(ui("3Dフォグ / 3D fog"));fog.setTextSize(20);fog.setChecked(graphics.getBoolean("fog",true));saveFog(fog.isChecked());weatherGroup.addView(fog);
        fog.setOnCheckedChangeListener((button,enabled)->{if(saveFog(enabled))graphics.edit().putBoolean("fog",enabled).apply();});
        Switch vrRain=new Switch(this);vrRain.setText(ui("雨：VR向け / ON: VR rain・OFF: 原作の雨"));vrRain.setTextSize(20);vrRain.setChecked(graphics.getBoolean("vr_rain",true));saveRain(vrRain.isChecked());
        vrRain.setOnCheckedChangeListener((button,enabled)->{if(saveRain(enabled))graphics.edit().putBoolean("vr_rain",enabled).apply();});weatherGroup.addView(vrRain);
        TextView rainHelp=new TextView(this);rainHelp.setText(ui("VR向け：視点の上下にも雨を展開。両モードとも不透明度30%で表示します。"));weatherGroup.addView(rainHelp);
        Switch vrSnow=new Switch(this);vrSnow.setText(ui("雪：VR向け / ON: VR snow・OFF: 原作の雪"));vrSnow.setTextSize(20);vrSnow.setChecked(graphics.getBoolean("vr_snow",true));saveSnow(vrSnow.isChecked());
        vrSnow.setOnCheckedChangeListener((button,enabled)->{if(saveSnow(enabled))graphics.edit().putBoolean("vr_snow",enabled).apply();});weatherGroup.addView(vrSnow);
        TextView snowHelp=new TextView(this);snowHelp.setText(ui("VR向け：視点の周囲にも雪を展開。原作の雪片・風・降り方を使用します。"));weatherGroup.addView(snowHelp);
        skyLabel=new TextView(this);skyLabel.setTextSize(18);water.addView(skyLabel);
        SeekBar skyBlend=new ValueSeekBar();skyBlend.setMax(100);skyBlend.setProgress(Math.max(0,Math.min(100,graphics.getInt("sky_reflection_blend",25))));
        skyLabel.setText(skyBlendLabel(skyBlend.getProgress()));saveSkyBlend(skyBlend.getProgress());
        skyBlend.setOnSeekBarChangeListener(this);water.addView(skyBlend);
        TextView skyHelp=new TextView(this);skyHelp.setText(ui("0：空の反射なし ／ 1：従来の強さ。ユニット・建物の反射は変わりません。"));water.addView(skyHelp);
        soundLabel=new TextView(this);soundLabel.setTextSize(18);audioGroup.addView(soundLabel);
        soundRange=new ValueSeekBar();soundRange.setMax(100);soundRange.setProgress(Math.max(0,Math.min(100,graphics.getInt("sound_range",150)-100)));
        soundLabel.setText(soundRangeLabel(soundRange.getProgress()));saveSoundRange(soundRange.getProgress());soundRange.setOnSeekBarChangeListener(this);audioGroup.addView(soundRange);
        TextView soundHelp=new TextView(this);soundHelp.setText(ui("1.00：原作の範囲 ／ 初期値1.50。VRへ戻ると反映されます。"));audioGroup.addView(soundHelp);
        for(int i=0;i<2;i++){
            cameraLabels[i]=new TextView(this);cameraLabels[i].setTextSize(18);cameraGroup.addView(cameraLabels[i]);
            cameraSpeeds[i]=new ValueSeekBar();cameraSpeeds[i].setMax(200);cameraSpeeds[i].setProgress(Math.max(0,Math.min(200,graphics.getInt(CAMERA_KEYS[i],100)-100)));
            cameraLabels[i].setText(cameraSpeedLabel(i,cameraSpeeds[i].getProgress()));cameraSpeeds[i].setOnSeekBarChangeListener(this);cameraGroup.addView(cameraSpeeds[i]);
        }
        addChatKeyboardOptions(keyboardGroup,graphics);
        Switch mapRotation=new Switch(this);mapRotation.setText(ui("ミニマップの回転を許可 / Allow Mini Map Rotation"));mapRotation.setTextSize(20);
        mapRotation.setChecked(graphics.getBoolean("allow_mini_map_rotation",false));mapGroup.addView(mapRotation);
        saveLighting("minimap-rotation.enabled",mapRotation.isChecked()?1:0);
        mapRotation.setOnCheckedChangeListener((button,enabled)->{if(saveLighting("minimap-rotation.enabled",enabled?1:0))graphics.edit().putBoolean("allow_mini_map_rotation",enabled).apply();});
        mapSizeLabel=new TextView(this);mapSizeLabel.setTextSize(18);mapGroup.addView(mapSizeLabel);
        mapSize=new ValueSeekBar();mapSize.setMax(150);mapSize.setProgress(Math.max(0,Math.min(150,graphics.getInt("wrist_map_size",100)-50)));
        mapSizeLabel.setText(mapSizeLabel(mapSize.getProgress()));mapSize.setOnSeekBarChangeListener(this);mapGroup.addView(mapSize);
        mapPitchLabel=new TextView(this);mapPitchLabel.setTextSize(18);mapGroup.addView(mapPitchLabel);
        int pitchOffset=graphics.getInt("wrist_map_pitch_offset",Math.max(-45,Math.min(45,graphics.getInt("wrist_map_pitch",-30)+30)));
        if(!graphics.contains("wrist_map_pitch_offset"))graphics.edit().putInt("wrist_map_pitch_offset",pitchOffset).apply();
        mapPitch=new ValueSeekBar();mapPitch.setMax(90);mapPitch.setProgress(Math.max(0,Math.min(90,pitchOffset+45)));
        mapPitchLabel.setText(mapPitchLabel(mapPitch.getProgress()));mapPitch.setOnSeekBarChangeListener(this);mapGroup.addView(mapPitch);
        orbitDistanceLabel=new TextView(this);orbitDistanceLabel.setTextSize(18);cameraGroup.addView(orbitDistanceLabel);
        orbitDistance=new ValueSeekBar();orbitDistance.setMax(100);orbitDistance.setProgress(Math.max(0,Math.min(100,graphics.getInt("orbit_distance",15))));
        orbitDistanceLabel.setText(orbitDistanceLabel(orbitDistance.getProgress()));orbitDistance.setOnSeekBarChangeListener(this);cameraGroup.addView(orbitDistance);
        radialSizeLabel=new TextView(this);radialSizeLabel.setTextSize(18);mapGroup.addView(radialSizeLabel);
        radialSize=new ValueSeekBar();radialSize.setMax(50);radialSize.setProgress(Math.max(0,Math.min(50,graphics.getInt("radial_size",100)-50)));
        radialSizeLabel.setText(radialSizeLabel(radialSize.getProgress()));radialSize.setOnSeekBarChangeListener(this);mapGroup.addView(radialSize);
        reuseFollowDistance=new Switch(this);reuseFollowDistance.setText(ui("追跡中に調整した距離を再利用 / Reuse follow distance"));reuseFollowDistance.setTextSize(20);
        reuseFollowDistance.setChecked(graphics.getBoolean("reuse_follow_distance",false));cameraGroup.addView(reuseFollowDistance);
        reuseFollowDistance.setOnCheckedChangeListener((button,enabled)->{if(saveCameraSpeeds())graphics.edit().putBoolean("reuse_follow_distance",enabled).apply();});
        TextView followHelp=new TextView(this);followHelp.setText(ui("ON：追跡中に左スティックで調整した距離を、次のジャンプ・追跡開始に使用。調整距離はアプリ実行中保持します。OFF：設定したオービット距離を使用。"));cameraGroup.addView(followHelp);
        aimOnJump=new Switch(this);aimOnJump.setText(ui("ジャンプ時に注視点へ水平回転 / Face target on jump"));aimOnJump.setTextSize(20);
        aimOnJump.setChecked(graphics.getBoolean("aim_on_jump",false));cameraGroup.addView(aimOnJump);
        aimOnJump.setOnCheckedChangeListener((button,enabled)->{if(saveCameraSpeeds())graphics.edit().putBoolean("aim_on_jump",enabled).apply();});
        TextView jumpHelp=new TextView(this);jumpHelp.setText(ui("OFF：現在の向きを保持。ON：注視点へ水平回転。単発ジャンプ・追跡開始の両方に適用します。"));cameraGroup.addView(jumpHelp);
        buttonGroups=new Switch(this);buttonGroups.setText(ui("Y/Bでグループ切替・ジェスチャーで選択解除"));buttonGroups.setTextSize(20);buttonGroups.setChecked(graphics.getBoolean("button_groups",true));inputGroup.addView(buttonGroups);
        tapGroups=new Switch(this);tapGroups.setText(ui("コントローラージェスチャー / Controller gesture"));tapGroups.setTextSize(20);tapGroups.setChecked(graphics.getBoolean("tap_groups",true));inputGroup.addView(tapGroups);
        tapDistanceLabel=new TextView(this);tapDistanceLabel.setTextSize(18);inputGroup.addView(tapDistanceLabel);
        tapDistance=new ValueSeekBar();tapDistance.setMax(90);tapDistance.setProgress(Math.max(0,Math.min(90,graphics.getInt("tap_distance",100)-10)));
        tapDistanceLabel.setText(tapDistanceLabel(tapDistance.getProgress()));tapDistance.setOnSeekBarChangeListener(this);inputGroup.addView(tapDistance);
        selectionHaptics=new Switch(this);selectionHaptics.setText(ui("ユニット選択時の振動 / Selection vibration"));selectionHaptics.setTextSize(20);selectionHaptics.setChecked(graphics.getBoolean("selection_haptics",true));inputGroup.addView(selectionHaptics);
        buttonGroups.setOnCheckedChangeListener((button,enabled)->{if(saveCameraSpeeds())graphics.edit().putBoolean("button_groups",enabled).apply();});
        tapGroups.setOnCheckedChangeListener((button,enabled)->{if(saveCameraSpeeds())graphics.edit().putBoolean("tap_groups",enabled).apply();});
        selectionHaptics.setOnCheckedChangeListener((button,enabled)->{if(saveCameraSpeeds())graphics.edit().putBoolean("selection_haptics",enabled).apply();});
        saveCameraSpeeds();
        TextView cameraHelp=new TextView(this);cameraHelp.setText(ui("1.00：従来の速度。各項目1.00～3.00倍。VRへ戻ると反映されます。\n左手メニューボタン：手首ミニマップの表示切り替え。"));cameraGroup.addView(cameraHelp);
        addSpritePbrDiagnostics(setupGroup,graphics);
        button(setupGroup,ui("Initial setup / 初回データ展開"),()->importSeed());
        button(setupGroup,ui("Status / 状態を更新"),()->show(StandaloneBridge.status()));
        TextView debugHelp=new TextView(this);debugHelp.setText(ui("詳細ログをADBへ出力します。PCへの保存にはUSB接続と収集ツールが必要です。"));setupGroup.addView(debugHelp);
        Switch fixedPose=new Switch(this);fixedPose.setText(ui("計測用：視点・コントローラー固定 / Benchmark: fixed view and controllers"));fixedPose.setTextSize(20);fixedPose.setChecked(graphics.getBoolean("benchmark_pose",false));
        saveLighting("benchmark-pose.enabled",fixedPose.isChecked()?1:0);setupGroup.addView(fixedPose);
        fixedPose.setOnCheckedChangeListener((button,enabled)->{if(saveLighting("benchmark-pose.enabled",enabled?1:0))graphics.edit().putBoolean("benchmark_pose",enabled).apply();});
        TextView fixedHelp=new TextView(this);fixedHelp.setText(ui("シーン初期位置で固定。ボタン入力は有効。診断ログONで1秒ごとに記録します。次回VR起動時に反映。 / Locks to the scene start. Buttons remain active. Enable diagnostic logging for one-second reports. Applies on next VR entry."));setupGroup.addView(fixedHelp);
        Switch shadowDebug=new Switch(this);shadowDebug.setText(ui("診断用：強化影を地面に色表示 / Diagnostic: show enhanced shadows on terrain"));shadowDebug.setTextSize(20);shadowDebug.setChecked(graphics.getBoolean("shadow_debug_overlay",false));
        saveLighting("shadow-debug-overlay.enabled",shadowDebug.isChecked()?1:0);setupGroup.addView(shadowDebug);
        shadowDebug.setOnCheckedChangeListener((button,enabled)->{if(saveLighting("shadow-debug-overlay.enabled",enabled?1:0))graphics.edit().putBoolean("shadow_debug_overlay",enabled).apply();});
        TextView shadowDebugHelp=new TextView(this);shadowDebugHelp.setText(ui("水色：固定物・地形、紫：動くモデル、黄：スプライト、白：影の重なり。強化影を有効にし、受影をピクセル単位に固定します。次回VR起動時に反映。 / Cyan: static meshes/terrain; magenta: moving models; yellow: sprites; white: overlap. Enables enhanced shadows and forces pixel shading. Applies on next VR entry."));setupGroup.addView(shadowDebugHelp);
        Switch skipShadowLookup=new Switch(this);skipShadowLookup.setText(ui("診断用：スプライトの影マップ参照を省略 / Diagnostic: skip sprite shadow-map lookup"));skipShadowLookup.setTextSize(20);skipShadowLookup.setChecked(graphics.getBoolean("diagnostic_skip_sprite_shadow",false));
        saveLighting("diagnostic-skip-sprite-shadow.enabled",skipShadowLookup.isChecked()?1:0);setupGroup.addView(skipShadowLookup);
        skipShadowLookup.setOnCheckedChangeListener((button,enabled)->{if(saveLighting("diagnostic-skip-sprite-shadow.enabled",enabled?1:0))graphics.edit().putBoolean("diagnostic_skip_sprite_shadow",enabled).apply();});
        TextView skipShadowHelp=new TextView(this);skipShadowHelp.setText(ui("通常はOFF。ONはスプライトが受ける他物体の影だけを省略。影マップ生成・簡易セルフシャドウ・照明・水面の影は維持。次回VR起動時に反映。 / Normally OFF. ON skips shadows received from other objects on sprites only. Shadow-map generation, self-shading, lighting and water shadows remain enabled. Applies on next VR entry."));setupGroup.addView(skipShadowHelp);
        Switch apiWaits=new Switch(this);apiWaits.setText(ui("診断用：描画APIの待機詳細 / Diagnostic: render API wait details"));apiWaits.setTextSize(20);apiWaits.setChecked(graphics.getBoolean("diagnostic_api_waits",false));
        saveLighting("diagnostic-api-waits.enabled",apiWaits.isChecked()?1:0);setupGroup.addView(apiWaits);
        apiWaits.setOnCheckedChangeListener((button,enabled)->{if(saveLighting("diagnostic-api-waits.enabled",enabled?1:0))graphics.edit().putBoolean("diagnostic_api_waits",enabled).apply();});
        TextView apiHelp=new TextView(this);apiHelp.setText(ui("診断ログONと併用。17パスに1回を計測し、遅い呼び出しを記録します。OSトレースはPCから取得。次回VR起動時に反映。 / Requires diagnostic logging. Samples one in 17 passes and records slow calls. Capture OS traces from the PC. Applies on next VR entry."));setupGroup.addView(apiHelp);
        Switch diagnostics=new Switch(this);diagnostics.setText(ui("診断ログ / Diagnostic logging"));diagnostics.setTextSize(20);diagnostics.setChecked(graphics.getBoolean("diagnostic_logging",false));
        saveDiagnostics(diagnostics.isChecked());diagnostics.setOnCheckedChangeListener((button,enabled)->{if(saveDiagnostics(enabled))graphics.edit().putBoolean("diagnostic_logging",enabled).apply();});setupGroup.addView(diagnostics);
        formatSwitches(layout);
        ScrollView scroll=new ScrollView(this);scroll.addView(layout);setLauncherContent(scroll);
        if(checkSelfPermission(android.Manifest.permission.WRITE_EXTERNAL_STORAGE)!=android.content.pm.PackageManager.PERMISSION_GRANTED)requestPermissions(new String[]{android.Manifest.permission.READ_EXTERNAL_STORAGE,android.Manifest.permission.WRITE_EXTERNAL_STORAGE},1);
        show(ui("Myth II Standalone Test\nLoad a campaign level in the original menu. VR opens automatically when the map is ready.\n原作メニューでキャンペーンのレベルをロードするとVRに切り替わります。"));
        if(getIntent().getBooleanExtra("setup",false))importSeed();
        if(getIntent().getBooleanExtra("start_engine",false))startEngine();
    }
    private LinearLayout section(LinearLayout parent,String key,String ja,String en,boolean initial){
        android.content.SharedPreferences prefs=getSharedPreferences("launcher_ui",MODE_PRIVATE);
        LinearLayout body=new LinearLayout(this);body.setOrientation(1);body.setPadding(20,8,20,20);
        Button header=new Button(this);header.setAllCaps(false);header.setTextSize(20);header.setGravity(android.view.Gravity.START|android.view.Gravity.CENTER_VERTICAL);
        String title=UiLanguage.english(this)?en:ja;
        boolean expanded=prefs.getBoolean("section_"+key,initial);
        body.setVisibility(expanded?android.view.View.VISIBLE:android.view.View.GONE);header.setText((expanded?"▼ ":"▶ ")+title);
        header.setOnClickListener(v->{boolean open=body.getVisibility()!=android.view.View.VISIBLE;body.setVisibility(open?android.view.View.VISIBLE:android.view.View.GONE);header.setText((open?"▼ ":"▶ ")+title);prefs.edit().putBoolean("section_"+key,open).apply();});
        parent.addView(header,new LinearLayout.LayoutParams(-1,-2));parent.addView(body,new LinearLayout.LayoutParams(-1,-2));return body;
    }
    private void formatSwitches(LinearLayout layout){
        for(int i=0;i<layout.getChildCount();i++){android.view.View child=layout.getChildAt(i);if(child instanceof LinearLayout){formatSwitches((LinearLayout)child);continue;}if(!(child instanceof Switch))continue;Switch toggle=(Switch)child;CharSequence label=toggle.getText();toggle.setText("");layout.removeViewAt(i);LinearLayout row=new LinearLayout(this);row.setOrientation(LinearLayout.HORIZONTAL);row.setGravity(android.view.Gravity.CENTER_VERTICAL);row.setLayoutDirection(android.view.View.LAYOUT_DIRECTION_LTR);row.addView(toggle,new LinearLayout.LayoutParams(-2,-2));TextView text=new TextView(this);text.setText(label);text.setTextSize(20);text.setTextColor(toggle.getTextColors());text.setPadding(16,0,0,0);text.setOnClickListener(v->{if(toggle.isEnabled())toggle.toggle();});row.addView(text,new LinearLayout.LayoutParams(0,-2,1));toggle.setContentDescription(label);layout.addView(row,i,new LinearLayout.LayoutParams(-1,-2));}
    }
    private String sliderValue(SeekBar bar){if("sprite-pbr.lod-start".equals(bar.getTag()))return (20+bar.getProgress())+" m";if("sprite-pbr.lod-width".equals(bar.getTag()))return (4+bar.getProgress())+" m";if(bar.getTag() instanceof String&&((String)bar.getTag()).startsWith("sprite-pbr."))return String.format(java.util.Locale.US,"%.2f",bar.getProgress()/100f);if("bloom_resolution".equals(bar.getTag()))return "1/"+new int[]{32,16,8,6,4,2}[bar.getProgress()];if("bloom_iterations".equals(bar.getTag()))return Integer.toString(bar.getProgress()+1);if("bloom_strength".equals(bar.getTag()))return bar.getProgress()+"%";if("scene-correction".equals(bar.getTag()))return bar.getProgress()+"%";if("keyboard-smoothing".equals(bar.getTag()))return (bar.getProgress()-493)+" ms";
        if(bar.getTag() instanceof String&&((String)bar.getTag()).startsWith("keyboard-offset:"))return (bar.getProgress()+Integer.parseInt(((String)bar.getTag()).substring(16)))+" cm";

        if(bar==pointerThickness)return String.format(java.util.Locale.US,"%.1fx",(bar.getProgress()+10)/10f);
        int v=bar.getProgress();if("rgb".equals(bar.getTag()))return Integer.toString(v);if(bar==shadowSamples)return Integer.toString(v+1);
        if(bar==shadowLod)return v==0?"OFF":v+" m";
        if(bar==lightingObject)return Integer.toString(v+1);if(bar==lightingLod)return Integer.toString(v);if(bar==lightingLimit)return Integer.toString(v+4);
        if(bar==shadowAmount||bar==lightingAmount)return v+"%";
        if(bar==orbitDistance)return v+" m";
        if(bar==mapPitch)return String.format(java.util.Locale.ROOT,"%+d°",v-45);
        if(bar==tapDistance)return String.format(java.util.Locale.ROOT,"%.2f×",(v+10)/100f);
        if(bar==radialSize||bar==mapSize)return String.format(java.util.Locale.ROOT,"%.2f×",(v+50)/100f);
        if(bar==soundRange||bar==cameraSpeeds[0]||bar==cameraSpeeds[1])return String.format(java.util.Locale.ROOT,"%.2f×",(v+100)/100f);
        return String.format(java.util.Locale.ROOT,"%.2f",v/100f);
    }
    private void showAccentButton(Button button,int color){
        button.setText(ui("強調色 / Accent color")+String.format(java.util.Locale.ROOT,"  #%06X  (%d, %d, %d)",color,(color>>16)&255,(color>>8)&255,color&255));
        button.setTextColor(0xff000000|color);
    }
    private void pickAccentColor(Button button){
        android.content.SharedPreferences prefs=getSharedPreferences("vr_graphics",MODE_PRIVATE);
        int initial=prefs.getInt("auto_ui_color",0xF5B642)&0xFFFFFF;
        LinearLayout panel=new LinearLayout(this);panel.setOrientation(1);panel.setPadding(24,16,24,16);
        TextView preview=new TextView(this);preview.setTextSize(22);preview.setPadding(20,20,20,20);panel.addView(preview);
        // Keep persisted resolution IDs stable; only reorder their slider positions.
        final int[] resolutionIds={5,4,0,1,2,3};
        SeekBar[] sliders=new SeekBar[3];EditText[] fields=new EditText[3];boolean[] changing={false};
        Runnable refresh=()->{int r=sliders[0].getProgress(),g=sliders[1].getProgress(),b=sliders[2].getProgress();preview.setBackgroundColor(android.graphics.Color.rgb(r,g,b));preview.setTextColor((r*299+g*587+b*114)>145000?0xff000000:0xffffffff);preview.setText(String.format(java.util.Locale.ROOT,"#%02X%02X%02X   RGB %d, %d, %d",r,g,b,r,g,b));};
        String[] names={"R","G","B"};
        for(int i=0;i<3;i++){
            LinearLayout row=new LinearLayout(this);row.setGravity(android.view.Gravity.CENTER_VERTICAL);TextView label=new TextView(this);label.setText(names[i]);row.addView(label);
            sliders[i]=new ValueSeekBar();sliders[i].setTag("rgb");sliders[i].setMax(255);sliders[i].setProgress((initial>>(16-i*8))&255);row.addView(sliders[i],new LinearLayout.LayoutParams(0,-2,1));
            fields[i]=new EditText(this);fields[i].setInputType(android.text.InputType.TYPE_CLASS_NUMBER);fields[i].setSelectAllOnFocus(true);fields[i].setEms(3);fields[i].setText(Integer.toString(sliders[i].getProgress()));row.addView(fields[i]);panel.addView(row);
        }
        for(int i=0;i<3;i++){final int channel=i;
            sliders[i].setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener(){public void onStartTrackingTouch(SeekBar b){}public void onStopTrackingTouch(SeekBar b){}public void onProgressChanged(SeekBar b,int value,boolean user){if(!changing[0]){changing[0]=true;fields[channel].setText(Integer.toString(value));fields[channel].setError(null);changing[0]=false;}refresh.run();}});
            fields[i].addTextChangedListener(new android.text.TextWatcher(){public void beforeTextChanged(CharSequence s,int start,int count,int after){}public void onTextChanged(CharSequence s,int start,int before,int count){if(changing[0])return;try{int value=Integer.parseInt(s.toString());if(value<0||value>255)throw new NumberFormatException();fields[channel].setError(null);changing[0]=true;sliders[channel].setProgress(value);changing[0]=false;refresh.run();}catch(NumberFormatException e){fields[channel].setError("0–255");}}public void afterTextChanged(android.text.Editable e){}});
        }
        refresh.run();ScrollView scroll=new ScrollView(this);scroll.addView(panel);
        android.app.AlertDialog dialog=new android.app.AlertDialog.Builder(this).setTitle(ui("強調色 / Accent color")).setView(scroll).setPositiveButton(ui("適用 / Apply"),null).setNegativeButton(android.R.string.cancel,null).setNeutralButton(ui("初期値 / Default"),null).create();
        dialog.setOnShowListener(d->{dialog.getButton(-3).setOnClickListener(v->{int[] defaults={245,182,66};for(int i=0;i<3;i++){sliders[i].setProgress(defaults[i]);fields[i].setText(Integer.toString(defaults[i]));}refresh.run();});dialog.getButton(-1).setOnClickListener(v->{for(EditText field:fields)if(field.getText().length()==0||field.getError()!=null){field.requestFocus();return;}int color=(sliders[0].getProgress()<<16)|(sliders[1].getProgress()<<8)|sliders[2].getProgress();if(saveLighting("auto-ui-contrast.color",color)){prefs.edit().putInt("auto_ui_color",color).apply();showAccentButton(button,color);dialog.dismiss();}});});dialog.show();
    }
    private final class ValueSeekBar extends SeekBar {
        private final android.graphics.Paint paint=new android.graphics.Paint(android.graphics.Paint.ANTI_ALIAS_FLAG);
        private final float dp=getResources().getDisplayMetrics().density;
        ValueSeekBar(){super(StandaloneActivity.this);setLayoutDirection(android.view.View.LAYOUT_DIRECTION_LTR);setPadding((int)(24*dp),(int)(32*dp),(int)(24*dp),(int)(8*dp));setMinimumHeight((int)(80*dp));}
        @Override protected synchronized void onDraw(android.graphics.Canvas canvas){
            super.onDraw(canvas);String value=sliderValue(this);paint.setTextSize(16*dp);paint.setTypeface(android.graphics.Typeface.DEFAULT_BOLD);
            float width=paint.measureText(value)+16*dp;
            float x=getPaddingLeft()+(getWidth()-getPaddingLeft()-getPaddingRight())*getProgress()/(float)Math.max(1,getMax());
            if(getThumb()!=null)x=getPaddingLeft()-getThumbOffset()+getThumb().getBounds().exactCenterX();
            x=Math.max(width/2,Math.min(getWidth()-width/2,x));
            float bottom=getPaddingTop()-3*dp;paint.setColor(0xff245d91);canvas.drawRoundRect(x-width/2,bottom-25*dp,x+width/2,bottom,5*dp,5*dp,paint);
            paint.setColor(0xffffffff);paint.setTextAlign(android.graphics.Paint.Align.CENTER);canvas.drawText(value,x,bottom-6*dp,paint);
        }
    }
    private static final float LAUNCHER_LOGO_BRIGHTNESS=.35f;
    private void setLauncherContent(ScrollView scroll){
        FrameLayout root=new FrameLayout(this);root.setBackgroundColor(0xff0d0d0d);
        try(InputStream stream=new FileInputStream(new File(getExternalFilesDir(null),"SharedAssets/Logo.png"))){
            final android.graphics.Bitmap logo=android.graphics.BitmapFactory.decodeStream(stream);
            if(logo==null)throw new IOException("Invalid launcher logo");
            ImageView background=new ImageView(this);background.setImageBitmap(logo);background.setScaleType(ImageView.ScaleType.MATRIX);
            float brightness=LAUNCHER_LOGO_BRIGHTNESS;
            background.setColorFilter(new android.graphics.ColorMatrixColorFilter(new float[]{brightness,0,0,0,0, 0,brightness,0,0,0, 0,0,brightness,0,0, 0,0,0,1,0}));
            background.addOnLayoutChangeListener((view,left,top,right,bottom,oldLeft,oldTop,oldRight,oldBottom)->{
                float factor=(right-left)/(float)logo.getWidth();
                android.graphics.Matrix matrix=new android.graphics.Matrix();matrix.setScale(factor,factor);
                matrix.postTranslate(0,((bottom-top)-logo.getHeight()*factor)*.5f);background.setImageMatrix(matrix);
            });
            background.setImportantForAccessibility(android.view.View.IMPORTANT_FOR_ACCESSIBILITY_NO);
            root.addView(background,new FrameLayout.LayoutParams(-1,-1));
        }catch(IOException error){android.util.Log.e("MythStandalone","Launcher logo unavailable",error);}
        scroll.setBackgroundColor(android.graphics.Color.TRANSPARENT);
        root.addView(scroll,new FrameLayout.LayoutParams(-1,-1));setContentView(root);
    }
    public void onProgressChanged(SeekBar bar,int value,boolean fromUser){bar.invalidate();if(bar==lightingObject){lightingObjectLabel.setText(ui("オブジェクトごとの最大光源数 / Maximum lights per object")+": "+(value+1));return;}if(bar==lightingLod){lightingLodLabel.setText(ui("照明LOD開始距離（0で無効） / Lighting LOD start distance (0 disables)")+": "+value);return;}if(bar==shadowSamples){shadowSamplesLabel.setText(ui("影サンプル数 / Shadow samples")+": "+(value+1));return;}if(bar==shadowLod){shadowLodLabel.setText(ui("強化影LOD開始距離 / Shadow LOD start distance")+": "+sliderValue(bar));return;}if(bar==lightingLimit){lightingLimitLabel.setText(ui("ビュー全体の最大光源数 / Maximum lights per view")+": "+(value+4));return;}if(bar==lightingAmount){lightingAmountLabel.setText(ui("ライティングの影響度 / Lighting influence")+": "+value+"%");return;}if(bar==shadowAmount){shadowAmountLabel.setText(ui("影の最大減光量 / Maximum shadow dimming")+": "+value+"%");return;}if(bar==tapDistance){tapDistanceLabel.setText(tapDistanceLabel(value));return;}if(bar==radialSize){radialSizeLabel.setText(radialSizeLabel(value));return;}if(bar==orbitDistance){orbitDistanceLabel.setText(orbitDistanceLabel(value));return;}if(bar==mapPitch){mapPitchLabel.setText(mapPitchLabel(value));return;}if(bar==mapSize){mapSizeLabel.setText(mapSizeLabel(value));return;}for(int i=0;i<2;i++)if(bar==cameraSpeeds[i]){cameraLabels[i].setText(cameraSpeedLabel(i,value));return;}if(bar==soundRange)soundLabel.setText(soundRangeLabel(value));else skyLabel.setText(skyBlendLabel(value));}
    public void onStartTrackingTouch(SeekBar bar){}
    public void onStopTrackingTouch(SeekBar bar){if(bar==lightingObject||bar==lightingLod){boolean object=bar==lightingObject;int value=bar.getProgress()+(object?1:0);if(saveLighting(object?"enhanced-lighting-per-object":"enhanced-lighting-lod",value))getSharedPreferences("vr_graphics",MODE_PRIVATE).edit().putInt(object?"enhanced_lighting_per_object":"enhanced_lighting_lod",value).apply();return;}if(bar==shadowSamples||bar==shadowLod){boolean samples=bar==shadowSamples;int value=bar.getProgress()+(samples?1:0);if(saveLighting(samples?"enhanced-shadow-samples":"enhanced-shadow-lod",value))getSharedPreferences("vr_graphics",MODE_PRIVATE).edit().putInt(samples?"enhanced_shadow_samples":"enhanced_shadow_lod",value).apply();return;}if(bar==lightingLimit){if(saveLighting("enhanced-lighting-limit",bar.getProgress()+4))getSharedPreferences("vr_graphics",MODE_PRIVATE).edit().putInt("enhanced_lighting_limit",bar.getProgress()+4).apply();return;}if(bar==lightingAmount){if(saveLighting("enhanced-lighting-amount",bar.getProgress()))getSharedPreferences("vr_graphics",MODE_PRIVATE).edit().putInt("enhanced_lighting_amount",bar.getProgress()).apply();return;}if(bar==shadowAmount){if(saveShadowAmount(bar.getProgress()))getSharedPreferences("vr_graphics",MODE_PRIVATE).edit().putInt("enhanced_shadow_amount",bar.getProgress()).apply();return;}int value=bar.getProgress();if(bar==tapDistance){if(saveCameraSpeeds())getSharedPreferences("vr_graphics",MODE_PRIVATE).edit().putInt("tap_distance",value+10).apply();return;}if(bar==radialSize){if(saveCameraSpeeds())getSharedPreferences("vr_graphics",MODE_PRIVATE).edit().putInt("radial_size",value+50).apply();return;}if(bar==orbitDistance){if(saveCameraSpeeds())getSharedPreferences("vr_graphics",MODE_PRIVATE).edit().putInt("orbit_distance",value).apply();return;}if(bar==mapPitch){if(saveCameraSpeeds())getSharedPreferences("vr_graphics",MODE_PRIVATE).edit().putInt("wrist_map_pitch_offset",value-45).apply();return;}if(bar==mapSize){if(saveCameraSpeeds())getSharedPreferences("vr_graphics",MODE_PRIVATE).edit().putInt("wrist_map_size",value+50).apply();return;}for(int i=0;i<2;i++)if(bar==cameraSpeeds[i]){if(saveCameraSpeeds())getSharedPreferences("vr_graphics",MODE_PRIVATE).edit().putInt(CAMERA_KEYS[i],value+100).apply();return;}if(bar==soundRange){if(saveSoundRange(value))getSharedPreferences("vr_graphics",MODE_PRIVATE).edit().putInt("sound_range",value+100).apply();return;}if(saveSkyBlend(value))getSharedPreferences("vr_graphics",MODE_PRIVATE).edit().putInt("sky_reflection_blend",value).apply();}
    private String tapDistanceLabel(int value){return String.format(java.util.Locale.ROOT,ui("ジェスチャー判定距離 / Gesture distance: %.2f倍"),(value+10)/100f);}
    private String radialSizeLabel(int value){return String.format(java.util.Locale.ROOT,ui("ダイヤルメニュー表示倍率 / Dial size: %.2f倍"),(value+50)/100f);}
    private String orbitDistanceLabel(int value){return String.format(java.util.Locale.ROOT,ui("未選択時のオービット距離 / Orbit distance: %d m"),value);}
    private String mapPitchLabel(int value){return String.format(java.util.Locale.ROOT,ui("ミニマップの傾き調整 / Pitch offset: %+d°"),value-45);}
    private String mapSizeLabel(int value){return String.format(java.util.Locale.ROOT,ui("手首ミニマップ: %.2f倍（幅 約%.1fcm）"),(value+50)/100f,(value+50)*.15f);}
    private String cameraSpeedLabel(int index,int value){return String.format(java.util.Locale.ROOT,ui("%s: %.2f倍"),ui(CAMERA_NAMES[index]),(value+100)/100f);}
    private boolean saveCameraSpeeds(){
        try{android.util.AtomicFile f=new android.util.AtomicFile(new File(getFilesDir(),"camera.speeds"));FileOutputStream out=f.startWrite();try{
            String values="";for(SeekBar bar:cameraSpeeds)values+=(Math.max(0,Math.min(200,bar.getProgress()))+100)+" ";values+="100 ";values+=(mapSize.getProgress()+50)+" "+(mapPitch.getProgress()-75)+" "+orbitDistance.getProgress()+" "+(radialSize.getProgress()+50)+" "+(reuseFollowDistance.isChecked()?1:0)+" "+(aimOnJump.isChecked()?1:0)+" "+(tapGroups!=null&&tapGroups.isChecked()?1:0)+" "+(selectionHaptics!=null&&selectionHaptics.isChecked()?1:0)+" "+(tapDistance==null?100:tapDistance.getProgress()+10);values+=" "+(buttonGroups!=null&&buttonGroups.isChecked()?1:0);out.write(values.getBytes("US-ASCII"));f.finishWrite(out);
        }catch(Exception e){f.failWrite(out);throw e;}return true;}catch(Exception e){show(ui("カメラ設定の保存に失敗: ")+e);return false;}
    }
    private String soundRangeLabel(int value){return String.format(java.util.Locale.ROOT,ui("可聴範囲 / Sound range: %.2f倍"),(value+100)/100f);}
    private boolean saveSoundRange(int value){
        try{android.util.AtomicFile f=new android.util.AtomicFile(new File(getFilesDir(),"sound.range"));FileOutputStream out=f.startWrite();try{out.write(Integer.toString(Math.max(100,Math.min(200,value+100))).getBytes("US-ASCII"));f.finishWrite(out);}catch(Exception e){f.failWrite(out);throw e;}return true;}
        catch(Exception e){show(ui("音声設定の保存に失敗: ")+e);return false;}
    }
    private String skyBlendLabel(int value){return String.format(java.util.Locale.ROOT,ui("空の水面反射ブレンド / Sky reflection: %.2f"),value/100f);}
    private boolean saveSkyBlend(int value){
        try{android.util.AtomicFile f=new android.util.AtomicFile(new File(getFilesDir(),"sky-reflection.blend"));FileOutputStream stream=f.startWrite();
            try{stream.write(String.format(java.util.Locale.ROOT,"%03d",Math.max(0,Math.min(100,value))).getBytes(java.nio.charset.StandardCharsets.US_ASCII));f.finishWrite(stream);}catch(Exception e){f.failWrite(stream);throw e;}return true;
        }catch(Exception e){Toast.makeText(this,ui("空の反射設定の保存に失敗しました: ")+e,Toast.LENGTH_LONG).show();return false;}
    }
    private boolean saveLighting(String name,int value){
        try{android.util.AtomicFile f=new android.util.AtomicFile(new File(getFilesDir(),name));FileOutputStream out=f.startWrite();try{out.write(Integer.toString(value).getBytes(java.nio.charset.StandardCharsets.US_ASCII));f.finishWrite(out);}catch(Exception e){f.failWrite(out);throw e;}return true;}catch(Exception e){Toast.makeText(this,ui("照明設定の保存に失敗しました: ")+e,Toast.LENGTH_LONG).show();return false;}
    }
    private boolean saveShadowAmount(int value){
        try{android.util.AtomicFile f=new android.util.AtomicFile(new File(getFilesDir(),"enhanced-shadow-amount"));FileOutputStream out=f.startWrite();try{out.write(Integer.toString(Math.max(0,Math.min(100,value))).getBytes(java.nio.charset.StandardCharsets.US_ASCII));f.finishWrite(out);}catch(Exception e){f.failWrite(out);throw e;}return true;}catch(Exception e){Toast.makeText(this,ui("影設定の保存に失敗しました: ")+e,Toast.LENGTH_LONG).show();return false;}
    }
    private boolean saveEnhancedShadows(boolean enabled){
        try{android.util.AtomicFile f=new android.util.AtomicFile(new File(getFilesDir(),"enhanced-shadows.enabled"));FileOutputStream out=f.startWrite();try{out.write(enabled?'1':'0');f.finishWrite(out);}catch(Exception e){f.failWrite(out);throw e;}return true;}catch(Exception e){Toast.makeText(this,ui("影設定の保存に失敗しました: ")+e,Toast.LENGTH_LONG).show();return false;}
    }
    private boolean saveWaterWaves(boolean enabled){
        try{android.util.AtomicFile f=new android.util.AtomicFile(new File(getFilesDir(),"water-waves.enabled"));FileOutputStream out=f.startWrite();try{out.write(enabled?'1':'0');f.finishWrite(out);}catch(Exception e){f.failWrite(out);throw e;}return true;}catch(Exception e){Toast.makeText(this,ui("水面設定の保存に失敗しました: ")+e,Toast.LENGTH_LONG).show();return false;}
    }
    private boolean saveFog(boolean vr){
        try{android.util.AtomicFile f=new android.util.AtomicFile(new File(getFilesDir(),"fog.enabled"));FileOutputStream out=f.startWrite();try{out.write(vr?'1':'0');f.finishWrite(out);}catch(Exception e){f.failWrite(out);throw e;}return true;}
        catch(Exception e){Toast.makeText(this,ui("フォグ設定の保存に失敗しました: ")+e,Toast.LENGTH_LONG).show();return false;}
    }
    private boolean saveRain(boolean vr){
        try{android.util.AtomicFile f=new android.util.AtomicFile(new File(getFilesDir(),"weather.mode"));FileOutputStream out=f.startWrite();try{out.write(vr?'1':'0');f.finishWrite(out);}catch(Exception e){f.failWrite(out);throw e;}return true;}
        catch(Exception e){Toast.makeText(this,ui("雨設定の保存に失敗しました: ")+e,Toast.LENGTH_LONG).show();return false;}
    }
    private boolean saveSnow(boolean vr){
        try{android.util.AtomicFile f=new android.util.AtomicFile(new File(getFilesDir(),"snow.mode"));FileOutputStream out=f.startWrite();try{out.write(vr?'1':'0');f.finishWrite(out);}catch(Exception e){f.failWrite(out);throw e;}return true;}
        catch(Exception e){Toast.makeText(this,ui("雪設定の保存に失敗しました: ")+e,Toast.LENGTH_LONG).show();return false;}
    }
    private boolean saveDiagnostics(boolean enabled){
        try {android.util.AtomicFile f=new android.util.AtomicFile(new File(getFilesDir(),"diagnostic.enabled"));FileOutputStream stream=f.startWrite();try{stream.write(enabled?'1':'0');f.finishWrite(stream);}catch(Exception e){f.failWrite(stream);throw e;}return true;}
        catch(Exception e){Toast.makeText(this,ui("診断ログ設定の保存に失敗しました: ")+e,Toast.LENGTH_LONG).show();return false;}
    }
    private boolean saveReflections(boolean enabled,boolean full,boolean high){
        try { android.util.AtomicFile f=new android.util.AtomicFile(new File(getFilesDir(),"reflection.enabled"));
            FileOutputStream stream=f.startWrite();try{stream.write(new byte[]{(byte)(enabled?'1':'0'),(byte)(full?'1':'0'),(byte)(high?'1':'0')});f.finishWrite(stream);}catch(Exception e){f.failWrite(stream);throw e;}
            android.util.Log.i("MythStandalone","Water reflections="+enabled+" full="+full+" highResolution="+high);return true;
        }catch(Exception e){Toast.makeText(this,ui("設定の保存に失敗しました: ")+e,Toast.LENGTH_LONG).show();return false;}
    }
    private void show(String message){android.util.Log.i("MythStandalone",message);status.setText(message);}
    private void button(LinearLayout layout,String title,Runnable action){
        Button b=new Button(this);b.setText(title);b.setMinWidth(0);b.setMinimumWidth(0);b.setOnClickListener(v->action.run());
        LinearLayout.LayoutParams params=new LinearLayout.LayoutParams(1,LinearLayout.LayoutParams.WRAP_CONTENT);params.gravity=android.view.Gravity.CENTER_HORIZONTAL;layout.addView(b,params);
        layout.addOnLayoutChangeListener((v,left,top,right,bottom,oldLeft,oldTop,oldRight,oldBottom)->{
            int width=Math.max(1,Math.round((right-left)*.20f));if(b.getLayoutParams().width!=width){android.view.ViewGroup.LayoutParams current=b.getLayoutParams();current.width=width;b.setLayoutParams(current);}
        });
    }
    private void importSeed(){
        if(importing)return;
        if(new File(getFilesDir(),"standalone.ready").exists()){show(ui("Runtime already exists; preserving its data. / 既存のデータを保持します。"));return;}
        importing=true;show(ui("Verifying and importing initial data… / 初期データを検証・展開中…"));
        new Thread(()->{try{
            File seed=new File(getExternalFilesDir(null),"runtime-seed.tar");
            byte[] digestText=new byte[64];try(InputStream in=getAssets().open("standalone/seed.sha256")){new DataInputStream(in).readFully(digestText);}String expected=new String(digestText,java.nio.charset.StandardCharsets.US_ASCII);
            java.security.MessageDigest hash=java.security.MessageDigest.getInstance("SHA-256");
            try(InputStream in=new FileInputStream(seed)){byte[] b=new byte[1048576];int n;while((n=in.read(b))>0)hash.update(b,0,n);}
            StringBuilder actual=new StringBuilder();for(byte b:hash.digest())actual.append(String.format("%02x",b&255));
            if(!expected.equals(actual.toString()))throw new IOException("Seed checksum mismatch");
            File log=new File(getFilesDir(),"setup.log");File staging=new File(getFilesDir(),"seed-stage-"+System.currentTimeMillis());staging.mkdirs();
            Process p=new ProcessBuilder("/system/bin/tar","-xf",seed.getPath(),"-C",staging.getPath()).redirectErrorStream(true).redirectOutput(log).start();
            if(p.waitFor()!=0){String error=new String(java.nio.file.Files.readAllBytes(log.toPath()),java.nio.charset.StandardCharsets.UTF_8);throw new IOException("Runtime extraction failed: "+error.substring(Math.max(0,error.length()-3000)));}
            File unpacked=new File(staging,"files/rootfs");if(!new File(unpacked,"home/xuser-1/.container").isFile())throw new IOException("Container missing from seed");
            File existing=new File(getFilesDir(),"rootfs");if(existing.exists()&&!existing.renameTo(new File(getFilesDir(),"rootfs-incomplete-"+System.currentTimeMillis())))throw new IOException("Cannot preserve incomplete data");
            if(!unpacked.renameTo(existing))throw new IOException("Cannot activate runtime");
            File components=new File(staging,"files/installed_components");File target=new File(getFilesDir(),"installed_components");if(!target.exists())components.renameTo(target);
            File config=new File(getFilesDir(),"rootfs/home/xuser-1/.container");
            String text=new String(java.nio.file.Files.readAllBytes(config.toPath()),java.nio.charset.StandardCharsets.UTF_8).replace("com.winlator",getPackageName());
            java.nio.file.Files.write(config.toPath(),text.getBytes(java.nio.charset.StandardCharsets.UTF_8));
            StandaloneBridge.prepare(this);new File(getFilesDir(),"standalone.ready").createNewFile();
            runOnUiThread(()->show(ui("Ready. Press Start Myth II. / 準備完了。Myth II を起動してください。")));
        }catch(Exception e){runOnUiThread(()->show("Setup failed: "+e));}finally{importing=false;}},"MythSetup").start();
    }
    private void startEngine(){
        File root=new File(getFilesDir(),"rootfs/home/xuser-1/.container");
        if(!root.isFile()||!new File(getFilesDir(),"standalone.ready").isFile()){show(ui("Initial runtime data has not been provisioned. / 初期データの配置が必要です。"));return;}
        try {
            StandaloneBridge.prepare(this);
            getSharedPreferences(getPackageName()+"_preferences",MODE_PRIVATE).edit()
                .putBoolean("enable_background_protection",true).putBoolean("capture_pointer_on_external_mouse",false)
                .putBoolean("move_cursor_to_touchpoint",true).apply();
            Intent i=new Intent();i.setClassName(this,"com.winlator.XServerDisplayActivity");
            i.putExtra("container_id",1);i.putExtra("exec_path","/storage/emulated/0/Download/MythII-Standalone/Myth II.exe");startActivity(i);
        }catch(Exception e){show("Start failed: "+e);}
    }
}
