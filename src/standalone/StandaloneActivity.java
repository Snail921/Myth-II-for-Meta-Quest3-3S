package org.mythvr.standalone;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.widget.*;
import java.io.*;

/** Development launcher. Initial data provisioning is separate from normal play. */
public final class StandaloneActivity extends Activity implements SeekBar.OnSeekBarChangeListener {
    private String ui(String text){return UiLanguage.text(this,text);}
    private TextView skyLabel,soundLabel,shadowAmountLabel,lightingAmountLabel,lightingLimitLabel,lightingObjectLabel,lightingLodLabel;
    private SeekBar shadowAmount,lightingAmount,lightingLimit,lightingObject,lightingLod,shadowSamples,shadowLod;
    private TextView shadowSamplesLabel,shadowLodLabel;
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
        button(layout,ui("Start Myth II / 原作を起動"),()->startEngine());
        button(layout,ui("Enter VR / VRに戻る"),()->StandaloneBridge.openVR(this));
        LinearLayout displayGroup=section(layout,"display","解像度・フレームレート","Resolution & frame rate",true);
        DisplaySettings.add(this,displayGroup);
        Switch fpsDisplay=new Switch(this);fpsDisplay.setText(ui("FPSを表示 / Show FPS"));fpsDisplay.setTextSize(20);fpsDisplay.setChecked(graphics.getBoolean("fps_display",false));
        saveLighting("fps-display.enabled",fpsDisplay.isChecked()?1:0);displayGroup.addView(fpsDisplay);
        fpsDisplay.setOnCheckedChangeListener((button,enabled)->{if(saveLighting("fps-display.enabled",enabled?1:0))graphics.edit().putBoolean("fps_display",enabled).apply();});
        LinearLayout water=section(layout,"water","水面","Water",false);
        LinearLayout texturesGroup=section(layout,"textures","テクスチャ・影","Textures & shadows",false);
        LinearLayout weatherGroup=section(layout,"weather","天候","Weather",false);
        LinearLayout audioGroup=section(layout,"audio","音声","Audio",false);
        LinearLayout cameraGroup=section(layout,"camera","カメラ","Camera",false);
        LinearLayout mapGroup=section(layout,"map","ミニマップ・メニュー","Minimap & menus",false);
        LinearLayout inputGroup=section(layout,"input","操作","Controls",false);
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
        Switch cutoutSprites=new Switch(this);cutoutSprites.setText(ui("ハイレゾスプライトの高品質な輪郭（2パス） / High-quality hi-res sprite edges (2 passes)"));cutoutSprites.setTextSize(20);cutoutSprites.setChecked(graphics.getBoolean("sprite_high_quality_edges",false));
        saveLighting("sprite-cutout-one-pass.enabled",cutoutSprites.isChecked()?0:1);texturesGroup.addView(cutoutSprites);
        cutoutSprites.setOnCheckedChangeListener((button,enabled)->{if(saveLighting("sprite-cutout-one-pass.enabled",enabled?0:1))graphics.edit().putBoolean("sprite_high_quality_edges",enabled).apply();});
        TextView cutoutHelp=new TextView(this);cutoutHelp.setText(ui("OFF（標準）：不透明1パスで軽量描画。ON：2パスで輪郭を滑らかに描画（負荷増）。炎・煙などの半透明効果は維持します。次回VR起動時に反映。 / OFF (default): fast opaque cutout in one pass. ON: smooth edges in two passes (higher cost). Fire, smoke and other translucent effects are preserved. Applies on next VR entry."));texturesGroup.addView(cutoutHelp);
        Switch hires=new Switch(this);hires.setText(ui("高解像度スプライト・モデル / Hi-res sprites and models"));hires.setTextSize(20);hires.setChecked(graphics.getBoolean("hires_sprites",true));texturesGroup.addView(hires);
        hires.setOnCheckedChangeListener((button,enabled)->graphics.edit().putBoolean("hires_sprites",enabled).apply());
        Switch details=new Switch(this);details.setText(ui("地形の高精細テクスチャ / Landscape detail textures"));details.setTextSize(20);details.setChecked(graphics.getBoolean("detail_textures",true));texturesGroup.addView(details);
        details.setOnCheckedChangeListener((button,enabled)->graphics.edit().putBoolean("detail_textures",enabled).apply());
        TextView detailHelp=new TextView(this);detailHelp.setText(ui("plugins内の対応アドオンを使用。レベルの読み込み時に反映されます。"));texturesGroup.addView(detailHelp);
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
        button(setupGroup,ui("Initial setup / 初回データ展開"),()->importSeed());
        button(setupGroup,ui("Status / 状態を更新"),()->show(StandaloneBridge.status()));
        TextView debugHelp=new TextView(this);debugHelp.setText(ui("詳細ログをADBへ出力します。PCへの保存にはUSB接続と収集ツールが必要です。"));setupGroup.addView(debugHelp);
        Switch fixedPose=new Switch(this);fixedPose.setText(ui("計測用：視点・コントローラー固定 / Benchmark: fixed view and controllers"));fixedPose.setTextSize(20);fixedPose.setChecked(graphics.getBoolean("benchmark_pose",false));
        saveLighting("benchmark-pose.enabled",fixedPose.isChecked()?1:0);setupGroup.addView(fixedPose);
        fixedPose.setOnCheckedChangeListener((button,enabled)->{if(saveLighting("benchmark-pose.enabled",enabled?1:0))graphics.edit().putBoolean("benchmark_pose",enabled).apply();});
        TextView fixedHelp=new TextView(this);fixedHelp.setText(ui("シーン初期位置で固定。ボタン入力は有効。診断ログONで1秒ごとに記録します。次回VR起動時に反映。 / Locks to the scene start. Buttons remain active. Enable diagnostic logging for one-second reports. Applies on next VR entry."));setupGroup.addView(fixedHelp);
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
    private String sliderValue(SeekBar bar){
        int v=bar.getProgress();if(bar==shadowSamples)return Integer.toString(v+1);
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
