package org.mythvr.standalone;
import android.app.Activity;
import android.content.Context;
import android.widget.*;
import android.view.*;
import java.util.*;
final class UiLanguage {
 static final Map<String,String[]> translations=new HashMap<>();
 static {
  translations.put("カメラ移動（水平・上下）",new String[]{"カメラ移動（水平・上下）","Camera movement (horizontal / vertical)"});
  translations.put("スティック回転（オービット・パン）",new String[]{"スティック回転（オービット・パン）","Stick rotation (orbit / pan)"});
  translations.put("鏡面反射 / Water reflections",new String[]{"鏡面反射","Water reflections"});
  translations.put("フルリフレクション / Full reflections",new String[]{"フルリフレクション","Full reflections"});
  translations.put("高精細反射 / Full-resolution reflections",new String[]{"高精細反射","Full-resolution reflections"});
  translations.put("水面の波・光沢 / Water waves & highlights",new String[]{"水面の波・光沢","Water waves & highlights"});
  translations.put("鏡面反射ON時に有効。波による映り込みの揺れと光沢を追加します。",new String[]{"鏡面反射ON時に有効。波による映り込みの揺れと光沢を追加します。","Requires water reflections. Adds animated waves and highlights."});
  translations.put("高精細な影 / Detailed shadows",new String[]{"高精細な影","Detailed shadows"});
  translations.put("高解像度スプライト・モデル / Hi-res sprites and models",new String[]{"高解像度スプライト・モデル","Hi-res sprites and models"});
  translations.put("地形の高精細テクスチャ / Landscape detail textures",new String[]{"地形の高精細テクスチャ","Landscape detail textures"});
  translations.put("plugins内の対応アドオンを使用。レベルの読み込み時に反映されます。",new String[]{"plugins内の対応アドオンを使用。レベルの読み込み時に反映されます。","Uses supported addons in Game/plugins. Reload the level after changing this setting."});
  translations.put("3Dフォグ / 3D fog",new String[]{"3Dフォグ","3D fog"});
  translations.put("雨：VR向け / ON: VR rain・OFF: 原作の雨",new String[]{"雨：VR向け","Rain: ON = VR / OFF = original"});
  translations.put("VR向け：視点の上下にも雨を展開。両モードとも不透明度30%で表示します。",new String[]{"VR向け：視点の上下にも雨を展開。両モードとも不透明度30%で表示します。","VR rain surrounds the viewpoint vertically. Both modes use 30% opacity."});
  translations.put("雪：VR向け / ON: VR snow・OFF: 原作の雪",new String[]{"雪：VR向け","Snow: ON = VR / OFF = original"});
  translations.put("VR向け：視点の周囲にも雪を展開。原作の雪片・風・降り方を使用します。",new String[]{"VR向け：視点の周囲にも雪を展開。原作の雪片・風・降り方を使用します。","VR snow surrounds the viewpoint using original flakes, wind and snowfall settings."});
  translations.put("0：空の反射なし ／ 1：従来の強さ。ユニット・建物の反射は変わりません。",new String[]{"0：空の反射なし ／ 1：従来の強さ。ユニット・建物の反射は変わりません。","0: no sky reflection / 1: full strength. Unit and building reflections are unchanged."});
  translations.put("1.00：原作の範囲 ／ 初期値1.50。VRへ戻ると反映されます。",new String[]{"1.00：原作の範囲 ／ 初期値1.50。VRへ戻ると反映されます。","1.00: original range / default: 1.50. Applied when returning to VR."});
  translations.put("追跡中に調整した距離を再利用 / Reuse follow distance",new String[]{"追跡中に調整した距離を再利用","Reuse follow distance"});
  translations.put("ON：追跡中に左スティックで調整した距離を、次のジャンプ・追跡開始に使用。調整距離はアプリ実行中保持します。OFF：設定したオービット距離を使用。",new String[]{"ON：追跡中に左スティックで調整した距離を、次のジャンプ・追跡開始に使用。調整距離はアプリ実行中保持します。OFF：設定したオービット距離を使用。","ON: reuse the distance adjusted with the left stick while following, for subsequent jumps and follow starts in this session. OFF: use the configured orbit distance."});
  translations.put("ジャンプ時に注視点へ水平回転 / Face target on jump",new String[]{"ジャンプ時に注視点へ水平回転","Face target on jump"});
  translations.put("OFF：現在の向きを保持。ON：注視点へ水平回転。単発ジャンプ・追跡開始の両方に適用します。",new String[]{"OFF：現在の向きを保持。ON：注視点へ水平回転。単発ジャンプ・追跡開始の両方に適用します。","OFF: keep your heading. ON: turn horizontally toward the target. Applies to jumps and follow starts."});
  translations.put("Y/Bでグループ切替・ジェスチャーで選択解除",new String[]{"Y/Bでグループ切替・ジェスチャーで選択解除","Y/B cycle groups; controller gestures deselect"});
  translations.put("コントローラージェスチャー / Controller gesture",new String[]{"コントローラージェスチャー","Controller gesture"});
  translations.put("ユニット選択時の振動 / Selection vibration",new String[]{"ユニット選択時の振動","Selection vibration"});
  translations.put("1.00：従来の速度。各項目1.00～3.00倍。VRへ戻ると反映されます。\n左手メニューボタン：手首ミニマップの表示切り替え。",new String[]{"1.00：従来の速度。各項目1.00～3.00倍。VRへ戻ると反映されます。\n左手メニューボタン：手首ミニマップの表示切り替え。","1.00: original speed. Range: 1.00–3.00x. Applied when returning to VR.\nLeft menu button toggles the wrist minimap."});
  translations.put("Initial setup / 初回データ展開",new String[]{"初回データ展開","Initial setup"});
  translations.put("Start Myth II / 原作を起動",new String[]{"原作を起動","Start Myth II"});
  translations.put("Enter VR / VRに戻る",new String[]{"VRに戻る","Enter VR"});
  translations.put("Status / 状態を更新",new String[]{"状態を更新","Status"});
  translations.put("詳細ログをADBへ出力します。PCへの保存にはUSB接続と収集ツールが必要です。",new String[]{"詳細ログをADBへ出力します。PCへの保存にはUSB接続と収集ツールが必要です。","Writes detailed logs to ADB. Saving logs on a PC requires USB and a log capture tool."});
  translations.put("診断ログ / Diagnostic logging",new String[]{"診断ログ","Diagnostic logging"});
  translations.put("Myth II Standalone Test\nLoad a campaign level in the original menu. VR opens automatically when the map is ready.\n原作メニューでキャンペーンのレベルをロードするとVRに切り替わります。",new String[]{"Myth II Standalone Test\nLoad a campaign level in the original menu. VR opens automatically when the map is ready.\n原作メニューでキャンペーンのレベルをロードするとVRに切り替わります。","Load a campaign level in the original menu. VR opens automatically when the map is ready."});
  translations.put("ジェスチャー判定距離 / Gesture distance: %.2f倍",new String[]{"ジェスチャー判定距離: %.2f倍","Gesture distance: %.2fx"});
  translations.put("ダイヤルメニュー表示倍率 / Dial size: %.2f倍",new String[]{"ダイヤルメニュー表示倍率: %.2f倍","Dial size: %.2fx"});
  translations.put("未選択時のオービット距離 / Orbit distance: %d m",new String[]{"未選択時のオービット距離: %d m","Orbit distance: %d m"});
  translations.put("ミニマップの傾き調整 / Pitch offset: %+d°",new String[]{"ミニマップの傾き調整: %+d°","Pitch offset: %+d°"});
  translations.put("手首ミニマップ: %.2f倍（幅 約%.1fcm）",new String[]{"手首ミニマップ: %.2f倍（幅 約%.1fcm）","Wrist minimap: %.2fx (width approx. %.1f cm)"});
  translations.put("%s: %.2f倍",new String[]{"%s: %.2f倍","%s: %.2fx"});
  translations.put("カメラ設定の保存に失敗: ",new String[]{"カメラ設定の保存に失敗: ","Could not save settings: "});
  translations.put("可聴範囲 / Sound range: %.2f倍",new String[]{"可聴範囲: %.2f倍","Sound range: %.2fx"});
  translations.put("音声設定の保存に失敗: ",new String[]{"音声設定の保存に失敗: ","Could not save settings: "});
  translations.put("空の水面反射ブレンド / Sky reflection: %.2f",new String[]{"空の水面反射ブレンド: %.2f","Sky reflection: %.2f"});
  translations.put("空の反射設定の保存に失敗しました: ",new String[]{"空の反射設定の保存に失敗しました: ","Could not save settings: "});
  translations.put("水面設定の保存に失敗しました: ",new String[]{"水面設定の保存に失敗しました: ","Could not save settings: "});
  translations.put("フォグ設定の保存に失敗しました: ",new String[]{"フォグ設定の保存に失敗しました: ","Could not save settings: "});
  translations.put("雨設定の保存に失敗しました: ",new String[]{"雨設定の保存に失敗しました: ","Could not save settings: "});
  translations.put("雪設定の保存に失敗しました: ",new String[]{"雪設定の保存に失敗しました: ","Could not save settings: "});
  translations.put("診断ログ設定の保存に失敗しました: ",new String[]{"診断ログ設定の保存に失敗しました: ","Could not save settings: "});
  translations.put("設定の保存に失敗しました: ",new String[]{"設定の保存に失敗しました: ","Could not save settings: "});
  translations.put("Runtime already exists; preserving its data. / 既存のデータを保持します。",new String[]{"既存のデータを保持します。","Runtime already exists; preserving its data."});
  translations.put("Verifying and importing initial data… / 初期データを検証・展開中…",new String[]{"初期データを検証・展開中…","Verifying and importing initial data…"});
  translations.put("Ready. Press Start Myth II. / 準備完了。原作を起動してください。",new String[]{"準備完了。原作を起動してください。","Ready. Press Start Myth II."});
  translations.put("Initial runtime data has not been provisioned. / 初期データの配置が必要です。",new String[]{"初期データの配置が必要です。","Initial runtime data has not been provisioned."});
 }
 static {translations.put("Myth II Standalone Test\nLoad a campaign level in the original menu. VR opens automatically when the map is ready.\n原作メニューでキャンペーンのレベルをロードするとVRに切り替わります。",new String[]{"原作メニューでレベルを開始すると、自動でVRへ切り替わります。","Load a level in the original menu to enter VR automatically."});}
 static boolean english(Context c){return c.getSharedPreferences("launcher_ui",0).getString("language","ja").equals("en");}
 static String text(Context c,String s){String[] v=translations.get(s);return v==null?s:v[english(c)?1:0];}
 static void selector(Activity a,LinearLayout layout,java.util.function.BooleanSupplier busy){
  Spinner spinner=new Spinner(a);spinner.setAdapter(new ArrayAdapter<String>(a,android.R.layout.simple_spinner_dropdown_item,new String[]{"日本語","English"}));
  spinner.setSelection(english(a)?1:0);layout.addView(spinner);
  spinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener(){
   public void onNothingSelected(AdapterView<?> p){}
   public void onItemSelected(AdapterView<?> p,View v,int i,long id){if((i==1)==english(a))return;if(busy.getAsBoolean()){spinner.setSelection(english(a)?1:0);return;}a.getSharedPreferences("launcher_ui",0).edit().putString("language",i==1?"en":"ja").commit();a.getIntent().removeExtra("setup");a.getIntent().removeExtra("start_engine");a.recreate();}
  });
 }
}
