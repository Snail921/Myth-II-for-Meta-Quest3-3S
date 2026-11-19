# ノーマルマップ集の使い方

[English](NORMAL_LIBRARY.md) | **日本語**


[ノーマルマップ集をダウンロード](https://github.com/Snail921/Myth-II-for-Meta-Quest3-3S/releases/download/v0.3.0-beta/MythIIVR-NormalMaps-2026.11.18.zip)

206コレクション分のノーマルマップです。全素材・全アニメーションを目視確認したものではありません。原作スプライト画像や編集用プロジェクトは含みません。自分の変換済み素材と照合してQuest用パックを作ります。

## 開発版：原作画像の指定なしで展開する

現在のソースから作成したAPKとManagerでは、上のZIPからハッシュ形式のQuest用パックを直接作れます。原作画像の選択・照合用データの復元は不要です。更新済みソースのフォルダで次を実行し、出力先をManagerからQuestへ転送してください。

```powershell
python tools/normal_library.py unpack "MythIIVR-NormalMaps-2026.11.18.zip" "UserData/DownloadedNormalPacks"
```

更新済み`tools/normal_library.py gui`でもZIPと保存先だけを選べます。新形式の`.mvrnm`がそのまま配布されている場合は、展開後すぐに転送できます。

**公開済み0.3.1-betaを使う場合は、以下の旧形式の手順を使ってください。** ダウンロードZIPに入っている従来の復元ツールは変更されていません。

## 1. 準備

- Questアプリは **0.3.0-beta以降**、Plugin Managerも最新ZIPを使います。
- WindowsにPython 3.11をインストールします。「Add python.exe to PATH」をONにしてください。
- このZIPを展開します。そのフォルダでターミナルを開き、`python -m pip install -r requirements-normal-library.txt` を一度実行します。
- Plugin Managerで、自分のMyth II／使用したいプラグインを変換しておきます。

## 2. Quest用パックを作る

1. **Restore Normal Maps.cmd** をダブルクリックします。
2. 自分の変換済みコレクションが入ったフォルダを選びます。変換結果の `levels` フォルダなど、`.bin` が入るフォルダを選んでください。サブフォルダも検索します。
3. 保存先に空のフォルダを選びます。例：`UserData/DownloadedNormalPacks`。
4. 黒いウィンドウに走査数・一致数・復元数が表示されます。`Done` が出るまで待ちます。

対応する素材だけが `.mvrnm` として作成されます。一部のフレームだけが一致した場合は、その部分だけを含むパックになります。`No matching source images` の場合は、ISOや原作のGameフォルダではなく、Managerが変換したコレクションのフォルダを選び直してください。別のプラグインを使う場合は、その変換結果でも繰り返せます。

## 3. Questへ転送

1. Questの原作ゲームを終了し、USB接続します。
2. **Plugin Manager.cmd** を開きます。
3. 「ノーマルマップフォルダ」に、手順2で指定した保存先を指定します。
4. **「ノーマルマップをQuestへ転送」**を押します。同名パックは差し替えられます。
5. Questで **Myth II for Meta Quest 3 / 3S** を開き、強化影または強化ライティングをONにしてレベルを開始します。

対応する画像に自動適用されます。初回データ展開のやり直しは不要です。効果が強すぎる場合はこのパックを外すか、自分の素材からエディタで別の強度のパックを作成してください。

復元中は照合用の一時データをPCに作成します。ダウンロードZIPと出力を含め、数GBの空き容量を確保してください。

[アプリ・Managerのダウンロード](https://github.com/Snail921/Myth-II-for-Meta-Quest3-3S/releases/tag/v0.3.1-beta) · [材質エディタの説明](https://github.com/Snail921/Myth-II-for-Meta-Quest3-3S/blob/main/docs/MATERIALS.ja.md)
