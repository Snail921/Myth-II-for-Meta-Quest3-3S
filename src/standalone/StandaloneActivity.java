package org.mythvr.standalone;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.widget.*;
import java.io.*;

/** Development launcher. Initial data provisioning is separate from normal play. */
public final class StandaloneActivity extends Activity implements SeekBar.OnSeekBarChangeListener {
    private String ui(String text){return UiLanguage.text(this,text);}
    private TextView skyLabel,soundLabel;
    private SeekBar soundRange,mapSize,mapPitch,orbitDistance,radialSize,tapDistance;
    private TextView mapSizeLabel,mapPitchLabel,orbitDistanceLabel,radialSizeLabel,tapDistanceLabel;
    private final SeekBar[] cameraSpeeds=new SeekBar[2];
    private final TextView[] cameraLabels=new TextView[2];
    private static final String[] CAMERA_KEYS={"camera_move","camera_stick_turn"};
    private static final String[] CAMERA_NAMES={"カメラ移動（水平・上下）","スティック回転（オービット・パン）"};
    private Switch reuseFollowDistance,aimOnJump,tapGroups,selectionHaptics,buttonGroups;
    private TextView status;
    private volatile boolean importing;
    @Override public void onCreate(Bundle state) {
        super.onCreate(state);
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
        Switch reflections=new Switch(this),fullReflections=new Switch(this),highReflections=new Switch(this);
        reflections.setText(ui("鏡面反射 / Water reflections"));fullReflections.setText(ui("フルリフレクション / Full reflections"));highReflections.setText(ui("高精細反射 / Full-resolution reflections"));
        reflections.setChecked(graphics.getBoolean("reflections",true));fullReflections.setChecked(graphics.getBoolean("reflections_full",false));highReflections.setChecked(graphics.getBoolean("reflections_high",false));
        for(Switch option:new Switch[]{reflections,fullReflections,highReflections}){option.setTextSize(20);layout.addView(option);}
        fullReflections.setEnabled(reflections.isChecked());highReflections.setEnabled(reflections.isChecked());
        saveReflections(reflections.isChecked(),fullReflections.isChecked(),highReflections.isChecked());
        CompoundButton.OnCheckedChangeListener graphicsChanged=(button,enabled)->{
            if(saveReflections(reflections.isChecked(),fullReflections.isChecked(),highReflections.isChecked()))graphics.edit().putBoolean("reflections",reflections.isChecked()).putBoolean("reflections_full",fullReflections.isChecked()).putBoolean("reflections_high",highReflections.isChecked()).apply();
            fullReflections.setEnabled(reflections.isChecked());highReflections.setEnabled(reflections.isChecked());
        };
        reflections.setOnCheckedChangeListener(graphicsChanged);fullReflections.setOnCheckedChangeListener(graphicsChanged);highReflections.setOnCheckedChangeListener(graphicsChanged);
        Switch waves=new Switch(this);waves.setText(ui("水面の波・光沢 / Water waves & highlights"));waves.setTextSize(20);waves.setChecked(graphics.getBoolean("water_waves",true));saveWaterWaves(waves.isChecked());layout.addView(waves);
        waves.setOnCheckedChangeListener((button,enabled)->{if(saveWaterWaves(enabled))graphics.edit().putBoolean("water_waves",enabled).apply();});
        TextView waveHelp=new TextView(this);waveHelp.setText(ui("鏡面反射ON時に有効。波による映り込みの揺れと光沢を追加します。"));layout.addView(waveHelp);
        Switch detailShadows=new Switch(this);detailShadows.setText(ui("高精細な影 / Detailed shadows"));detailShadows.setTextSize(20);detailShadows.setChecked(graphics.getBoolean("detail_shadows",true));layout.addView(detailShadows);
        detailShadows.setOnCheckedChangeListener((button,enabled)->graphics.edit().putBoolean("detail_shadows",enabled).apply());
        Switch hires=new Switch(this);hires.setText(ui("高解像度スプライト・モデル / Hi-res sprites and models"));hires.setTextSize(20);hires.setChecked(graphics.getBoolean("hires_sprites",true));layout.addView(hires);
        hires.setOnCheckedChangeListener((button,enabled)->graphics.edit().putBoolean("hires_sprites",enabled).apply());
        Switch details=new Switch(this);details.setText(ui("地形の高精細テクスチャ / Landscape detail textures"));details.setTextSize(20);details.setChecked(graphics.getBoolean("detail_textures",true));layout.addView(details);
        details.setOnCheckedChangeListener((button,enabled)->graphics.edit().putBoolean("detail_textures",enabled).apply());
        TextView detailHelp=new TextView(this);detailHelp.setText(ui("plugins内の対応アドオンを使用。レベルの読み込み時に反映されます。"));layout.addView(detailHelp);
        Switch fog=new Switch(this);fog.setText(ui("3Dフォグ / 3D fog"));fog.setTextSize(20);fog.setChecked(graphics.getBoolean("fog",true));saveFog(fog.isChecked());layout.addView(fog);
        fog.setOnCheckedChangeListener((button,enabled)->{if(saveFog(enabled))graphics.edit().putBoolean("fog",enabled).apply();});
        Switch vrRain=new Switch(this);vrRain.setText(ui("雨：VR向け / ON: VR rain・OFF: 原作の雨"));vrRain.setTextSize(20);vrRain.setChecked(graphics.getBoolean("vr_rain",true));saveRain(vrRain.isChecked());
        vrRain.setOnCheckedChangeListener((button,enabled)->{if(saveRain(enabled))graphics.edit().putBoolean("vr_rain",enabled).apply();});layout.addView(vrRain);
        TextView rainHelp=new TextView(this);rainHelp.setText(ui("VR向け：視点の上下にも雨を展開。両モードとも不透明度30%で表示します。"));layout.addView(rainHelp);
        Switch vrSnow=new Switch(this);vrSnow.setText(ui("雪：VR向け / ON: VR snow・OFF: 原作の雪"));vrSnow.setTextSize(20);vrSnow.setChecked(graphics.getBoolean("vr_snow",true));saveSnow(vrSnow.isChecked());
        vrSnow.setOnCheckedChangeListener((button,enabled)->{if(saveSnow(enabled))graphics.edit().putBoolean("vr_snow",enabled).apply();});layout.addView(vrSnow);
        TextView snowHelp=new TextView(this);snowHelp.setText(ui("VR向け：視点の周囲にも雪を展開。原作の雪片・風・降り方を使用します。"));layout.addView(snowHelp);
        skyLabel=new TextView(this);skyLabel.setTextSize(18);layout.addView(skyLabel);
        SeekBar skyBlend=new SeekBar(this);skyBlend.setMax(100);skyBlend.setProgress(Math.max(0,Math.min(100,graphics.getInt("sky_reflection_blend",25))));
        skyLabel.setText(skyBlendLabel(skyBlend.getProgress()));saveSkyBlend(skyBlend.getProgress());
        skyBlend.setOnSeekBarChangeListener(this);layout.addView(skyBlend);
        TextView skyHelp=new TextView(this);skyHelp.setText(ui("0：空の反射なし ／ 1：従来の強さ。ユニット・建物の反射は変わりません。"));layout.addView(skyHelp);
        soundLabel=new TextView(this);soundLabel.setTextSize(18);layout.addView(soundLabel);
        soundRange=new SeekBar(this);soundRange.setMax(100);soundRange.setProgress(Math.max(0,Math.min(100,graphics.getInt("sound_range",150)-100)));
        soundLabel.setText(soundRangeLabel(soundRange.getProgress()));saveSoundRange(soundRange.getProgress());soundRange.setOnSeekBarChangeListener(this);layout.addView(soundRange);
        TextView soundHelp=new TextView(this);soundHelp.setText(ui("1.00：原作の範囲 ／ 初期値1.50。VRへ戻ると反映されます。"));layout.addView(soundHelp);
        for(int i=0;i<2;i++){
            cameraLabels[i]=new TextView(this);cameraLabels[i].setTextSize(18);layout.addView(cameraLabels[i]);
            cameraSpeeds[i]=new SeekBar(this);cameraSpeeds[i].setMax(200);cameraSpeeds[i].setProgress(Math.max(0,Math.min(200,graphics.getInt(CAMERA_KEYS[i],100)-100)));
            cameraLabels[i].setText(cameraSpeedLabel(i,cameraSpeeds[i].getProgress()));cameraSpeeds[i].setOnSeekBarChangeListener(this);layout.addView(cameraSpeeds[i]);
        }
        mapSizeLabel=new TextView(this);mapSizeLabel.setTextSize(18);layout.addView(mapSizeLabel);
        mapSize=new SeekBar(this);mapSize.setMax(150);mapSize.setProgress(Math.max(0,Math.min(150,graphics.getInt("wrist_map_size",100)-50)));
        mapSizeLabel.setText(mapSizeLabel(mapSize.getProgress()));mapSize.setOnSeekBarChangeListener(this);layout.addView(mapSize);
        mapPitchLabel=new TextView(this);mapPitchLabel.setTextSize(18);layout.addView(mapPitchLabel);
        int pitchOffset=graphics.getInt("wrist_map_pitch_offset",Math.max(-45,Math.min(45,graphics.getInt("wrist_map_pitch",-30)+30)));
        if(!graphics.contains("wrist_map_pitch_offset"))graphics.edit().putInt("wrist_map_pitch_offset",pitchOffset).apply();
        mapPitch=new SeekBar(this);mapPitch.setMax(90);mapPitch.setProgress(Math.max(0,Math.min(90,pitchOffset+45)));
        mapPitchLabel.setText(mapPitchLabel(mapPitch.getProgress()));mapPitch.setOnSeekBarChangeListener(this);layout.addView(mapPitch);
        orbitDistanceLabel=new TextView(this);orbitDistanceLabel.setTextSize(18);layout.addView(orbitDistanceLabel);
        orbitDistance=new SeekBar(this);orbitDistance.setMax(100);orbitDistance.setProgress(Math.max(0,Math.min(100,graphics.getInt("orbit_distance",15))));
        orbitDistanceLabel.setText(orbitDistanceLabel(orbitDistance.getProgress()));orbitDistance.setOnSeekBarChangeListener(this);layout.addView(orbitDistance);
        radialSizeLabel=new TextView(this);radialSizeLabel.setTextSize(18);layout.addView(radialSizeLabel);
        radialSize=new SeekBar(this);radialSize.setMax(50);radialSize.setProgress(Math.max(0,Math.min(50,graphics.getInt("radial_size",100)-50)));
        radialSizeLabel.setText(radialSizeLabel(radialSize.getProgress()));radialSize.setOnSeekBarChangeListener(this);layout.addView(radialSize);
        reuseFollowDistance=new Switch(this);reuseFollowDistance.setText(ui("追跡中に調整した距離を再利用 / Reuse follow distance"));reuseFollowDistance.setTextSize(20);
        reuseFollowDistance.setChecked(graphics.getBoolean("reuse_follow_distance",false));layout.addView(reuseFollowDistance);
        reuseFollowDistance.setOnCheckedChangeListener((button,enabled)->{if(saveCameraSpeeds())graphics.edit().putBoolean("reuse_follow_distance",enabled).apply();});
        TextView followHelp=new TextView(this);followHelp.setText(ui("ON：追跡中に左スティックで調整した距離を、次のジャンプ・追跡開始に使用。調整距離はアプリ実行中保持します。OFF：設定したオービット距離を使用。"));layout.addView(followHelp);
        aimOnJump=new Switch(this);aimOnJump.setText(ui("ジャンプ時に注視点へ水平回転 / Face target on jump"));aimOnJump.setTextSize(20);
        aimOnJump.setChecked(graphics.getBoolean("aim_on_jump",false));layout.addView(aimOnJump);
        aimOnJump.setOnCheckedChangeListener((button,enabled)->{if(saveCameraSpeeds())graphics.edit().putBoolean("aim_on_jump",enabled).apply();});
        TextView jumpHelp=new TextView(this);jumpHelp.setText(ui("OFF：現在の向きを保持。ON：注視点へ水平回転。単発ジャンプ・追跡開始の両方に適用します。"));layout.addView(jumpHelp);
        buttonGroups=new Switch(this);buttonGroups.setText(ui("Y/Bでグループ切替・ジェスチャーで選択解除"));buttonGroups.setTextSize(20);buttonGroups.setChecked(graphics.getBoolean("button_groups",true));layout.addView(buttonGroups);
        tapGroups=new Switch(this);tapGroups.setText(ui("コントローラージェスチャー / Controller gesture"));tapGroups.setTextSize(20);tapGroups.setChecked(graphics.getBoolean("tap_groups",true));layout.addView(tapGroups);
        tapDistanceLabel=new TextView(this);tapDistanceLabel.setTextSize(18);layout.addView(tapDistanceLabel);
        tapDistance=new SeekBar(this);tapDistance.setMax(90);tapDistance.setProgress(Math.max(0,Math.min(90,graphics.getInt("tap_distance",100)-10)));
        tapDistanceLabel.setText(tapDistanceLabel(tapDistance.getProgress()));tapDistance.setOnSeekBarChangeListener(this);layout.addView(tapDistance);
        selectionHaptics=new Switch(this);selectionHaptics.setText(ui("ユニット選択時の振動 / Selection vibration"));selectionHaptics.setTextSize(20);selectionHaptics.setChecked(graphics.getBoolean("selection_haptics",true));layout.addView(selectionHaptics);
        buttonGroups.setOnCheckedChangeListener((button,enabled)->{if(saveCameraSpeeds())graphics.edit().putBoolean("button_groups",enabled).apply();});
        tapGroups.setOnCheckedChangeListener((button,enabled)->{if(saveCameraSpeeds())graphics.edit().putBoolean("tap_groups",enabled).apply();});
        selectionHaptics.setOnCheckedChangeListener((button,enabled)->{if(saveCameraSpeeds())graphics.edit().putBoolean("selection_haptics",enabled).apply();});
        saveCameraSpeeds();
        TextView cameraHelp=new TextView(this);cameraHelp.setText(ui("1.00：従来の速度。各項目1.00～3.00倍。VRへ戻ると反映されます。\n左手メニューボタン：手首ミニマップの表示切り替え。"));layout.addView(cameraHelp);
        button(layout,ui("Initial setup / 初回データ展開"),()->importSeed());
        button(layout,ui("Start Myth II / 原作を起動"),()->startEngine());
        button(layout,ui("Enter VR / VRに戻る"),()->StandaloneBridge.openVR(this));
        button(layout,ui("Status / 状態を更新"),()->show(StandaloneBridge.status()));
        TextView debugHelp=new TextView(this);debugHelp.setText(ui("詳細ログをADBへ出力します。PCへの保存にはUSB接続と収集ツールが必要です。"));layout.addView(debugHelp);
        Switch diagnostics=new Switch(this);diagnostics.setText(ui("診断ログ / Diagnostic logging"));diagnostics.setTextSize(20);diagnostics.setChecked(graphics.getBoolean("diagnostic_logging",false));
        saveDiagnostics(diagnostics.isChecked());diagnostics.setOnCheckedChangeListener((button,enabled)->{if(saveDiagnostics(enabled))graphics.edit().putBoolean("diagnostic_logging",enabled).apply();});layout.addView(diagnostics);
        // Keep the native switch and its listener, but put its label to the right.
        for(int i=0;i<layout.getChildCount();i++){android.view.View child=layout.getChildAt(i);if(!(child instanceof Switch))continue;Switch toggle=(Switch)child;CharSequence label=toggle.getText();toggle.setText("");layout.removeViewAt(i);LinearLayout row=new LinearLayout(this);row.setOrientation(LinearLayout.HORIZONTAL);row.setGravity(android.view.Gravity.CENTER_VERTICAL);row.setLayoutDirection(android.view.View.LAYOUT_DIRECTION_LTR);row.addView(toggle,new LinearLayout.LayoutParams(-2,-2));TextView text=new TextView(this);text.setText(label);text.setTextSize(20);text.setTextColor(toggle.getTextColors());text.setPadding(16,0,0,0);text.setOnClickListener(v->toggle.toggle());row.addView(text,new LinearLayout.LayoutParams(0,-2,1));toggle.setContentDescription(label);layout.addView(row,i,new LinearLayout.LayoutParams(-1,-2));}
        ScrollView scroll=new ScrollView(this);scroll.addView(layout);setLauncherContent(scroll);
        if(checkSelfPermission(android.Manifest.permission.WRITE_EXTERNAL_STORAGE)!=android.content.pm.PackageManager.PERMISSION_GRANTED)requestPermissions(new String[]{android.Manifest.permission.READ_EXTERNAL_STORAGE,android.Manifest.permission.WRITE_EXTERNAL_STORAGE},1);
        show(ui("Myth II Standalone Test\nLoad a campaign level in the original menu. VR opens automatically when the map is ready.\n原作メニューでキャンペーンのレベルをロードするとVRに切り替わります。"));
        if(getIntent().getBooleanExtra("setup",false))importSeed();
        if(getIntent().getBooleanExtra("start_engine",false))startEngine();
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
    public void onProgressChanged(SeekBar bar,int value,boolean fromUser){if(bar==tapDistance){tapDistanceLabel.setText(tapDistanceLabel(value));return;}if(bar==radialSize){radialSizeLabel.setText(radialSizeLabel(value));return;}if(bar==orbitDistance){orbitDistanceLabel.setText(orbitDistanceLabel(value));return;}if(bar==mapPitch){mapPitchLabel.setText(mapPitchLabel(value));return;}if(bar==mapSize){mapSizeLabel.setText(mapSizeLabel(value));return;}for(int i=0;i<2;i++)if(bar==cameraSpeeds[i]){cameraLabels[i].setText(cameraSpeedLabel(i,value));return;}if(bar==soundRange)soundLabel.setText(soundRangeLabel(value));else skyLabel.setText(skyBlendLabel(value));}
    public void onStartTrackingTouch(SeekBar bar){}
    public void onStopTrackingTouch(SeekBar bar){int value=bar.getProgress();if(bar==tapDistance){if(saveCameraSpeeds())getSharedPreferences("vr_graphics",MODE_PRIVATE).edit().putInt("tap_distance",value+10).apply();return;}if(bar==radialSize){if(saveCameraSpeeds())getSharedPreferences("vr_graphics",MODE_PRIVATE).edit().putInt("radial_size",value+50).apply();return;}if(bar==orbitDistance){if(saveCameraSpeeds())getSharedPreferences("vr_graphics",MODE_PRIVATE).edit().putInt("orbit_distance",value).apply();return;}if(bar==mapPitch){if(saveCameraSpeeds())getSharedPreferences("vr_graphics",MODE_PRIVATE).edit().putInt("wrist_map_pitch_offset",value-45).apply();return;}if(bar==mapSize){if(saveCameraSpeeds())getSharedPreferences("vr_graphics",MODE_PRIVATE).edit().putInt("wrist_map_size",value+50).apply();return;}for(int i=0;i<2;i++)if(bar==cameraSpeeds[i]){if(saveCameraSpeeds())getSharedPreferences("vr_graphics",MODE_PRIVATE).edit().putInt(CAMERA_KEYS[i],value+100).apply();return;}if(bar==soundRange){if(saveSoundRange(value))getSharedPreferences("vr_graphics",MODE_PRIVATE).edit().putInt("sound_range",value+100).apply();return;}if(saveSkyBlend(value))getSharedPreferences("vr_graphics",MODE_PRIVATE).edit().putInt("sky_reflection_blend",value).apply();}
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
            runOnUiThread(()->show(ui("Ready. Press Start Myth II. / 準備完了。原作を起動してください。")));
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
