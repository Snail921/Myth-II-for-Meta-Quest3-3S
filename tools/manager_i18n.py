"""Persistent English/Japanese UI strings shared by manager and conversion stages."""
import json,os
from pathlib import Path
SETTINGS=Path(os.environ.get('APPDATA',Path.home()))/'MythIIVR/manager.json'
PAIRS={
'プラグイン管理':'Plugin manager','原作フォルダ':'Original game folder','プラグインファイル':'Plugin file','作業・保存先':'Work / output folder','変換済み bundle':'Converted bundle','参照':'Browse','Quest シリアル':'Quest serial','接続確認':'Refresh devices','① レベルを検査':'1. Inspect levels','② 全レベルを変換':'2. Convert all levels','③ Questへ配置':'3. Deploy to Quest','保存先を開く':'Open output folder','レベル':'Level','検査結果':'Inspection result','入力':'Input','処理中':'Working','保存先':'Output folder','対応可能':'Supported','不足素材: ':'Missing assets: ','除外（未完成のエントリー）':'Excluded (unfinished entrypoint)','素材数が上限223を超えています':'Asset count exceeds the limit of 223',
'原作フォルダとプラグインを選び、検査してください。':'Select the original game folder and plugin, then inspect.',
'1.8.5 / 単独プラグイン対応。PCで変換し、Questへ配置します。':'1.8.5 / Single gameplay plugin. Convert on PC, then deploy to Quest.',
'配置は原作ゲームを終了してから。旧パックは退避され、APKの更新は不要です。':'Exit the game before deployment. Existing packs are backed up; no APK update is needed.',
'処理を開始しています':'Starting work','原作フォルダ・プラグイン・保存先を指定してください。':'Select the original game folder, plugin and output folder.',
'検査が完了しました。結果を確認してください。':'Inspection complete. Review the results.',
'変換が完了しました。原作を終了して「Questへ配置」を押してください。':'Conversion complete. Exit the game and choose Deploy to Quest.',
'bundleフォルダと元のプラグインを指定してください。':'Select the bundle folder and original plugin.',
'配置完了。記録: ':'Deployment complete. Log: ','接続済み: ':'Connected: ','なし。USBデバッグ許可をご確認ください。':'none. Check USB debugging authorization.',
'エラー: ':'Error: ','処理を完了できませんでした':'Could not complete operation','処理が終わるまでお待ちください。変換ログは保存先にも記録されています。':'Wait for the operation to finish. Conversion logs are also saved in the output folder.',
'原作・プラグインの照合':'Checking game and plugin','素材一覧の読み込み':'Reading asset index','レベル情報の検査':'Inspecting levels','レベルの依存素材検査':'Checking level dependencies','レベル変換':'Converting levels','高解像度素材の対応表を作成':'Building high-resolution bindings','原作ファイルの再検証':'Rechecking source files','外部パックを書き出し':'Exporting level pack','原作素材との対応を記録':'Recording source bindings','完成パックを検証':'Verifying finished pack','アーカイブを検査':'Inspecting archives',
'原作セットアップ':'Base game setup','ISOファイル':'ISO file','1.8.5アップデーター':'1.8.5 updater','7-Zip':'7-Zip','任意のHDアドオンを追加':'Add optional HD addons','原作を展開・全レベルを変換':'Extract game and convert all levels','セットアップデータを配置':'Deploy setup data','セットアップ結果':'Setup output','ランタイムデータ':'Runtime seed',
'Extracting original files':'Extracting original files','Extracting cutscenes':'Extracting cutscenes','Setup data ready':'Setup data ready'
}
PAIRS.update({
 '経過':'Elapsed','進捗通知待ち':'Waiting for progress','依存素材を展開':'Extracting dependencies','素材コレクションを抽出':'Extracting asset collections','変換: ':'Converting: ','ミニマップを作成':'Building minimap','アニメーションモデルを変換':'Converting animated models','パーティクル素材を変換':'Converting particle assets','表面効果を変換':'Converting surface effects','地形タイルを変換':'Converting terrain tiles','スプライト画像を変換':'Converting sprite images','モデル配置を変換':'Converting model placements',
 'セットアップ完了: ':'Setup complete: ','ISO・アップデーター・7-Zipを指定してください。':'Select ISO, updater and 7-Zip.'
})
PAIRS.update({'スカイボックス（任意）':'Skyboxes (optional)','ロゴ画像（任意）':'Logo image (optional)','CD画像（任意）':'CD image (optional)','原作セットアップ':'Base game setup'})
def load():
 try:return 'en' if json.loads(SETTINGS.read_text(encoding='utf8')).get('language')=='en' else 'ja'
 except (OSError,ValueError):return 'ja'
def save(language):
 SETTINGS.parent.mkdir(parents=True,exist_ok=True);temp=SETTINGS.with_suffix('.tmp');temp.write_text(json.dumps({'language':language}),encoding='utf8');temp.replace(SETTINGS)
def translate(text,language):
 if language=='en' and text.startswith('Plugin “') and text.endswith('” を転送しています…'):
  name=text[len('Plugin “'):-len('” を転送しています…')]
  return f'Transferring plugin “{name}”…'
 table=PAIRS if language=='en' else {en:jp for jp,en in PAIRS.items() if en!=jp}
 if language=='ja':table.update({'Extracting original files':'原作ファイルを展開','Extracting cutscenes':'ムービーを展開','Setup data ready':'セットアップデータの準備完了','Copying HD addon':'HDアドオンをコピー'})
 if text in table:return table[text]
 # Preserve filenames and diagnostic payloads verbatim; translate message prefixes only.
 for source,target in sorted(table.items(),key=lambda p:len(p[0]),reverse=True):
  if source.endswith(': ') and text.startswith(source):return target+text[len(source):]
 return text
