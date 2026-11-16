# ソースとビルド資料

[English](BUILD.md) | **日本語**

開発ソースのスナップショットです。まだ、このチェックアウトだけでAPKを一括生成する構成ではありません。Plugin Managerは記載の依存ソフトを導入して実行できます。ランタイムのバイナリ・非公開の原作検証データは含めません。

## 1.8.5向けソースの生成

`src/standalone` と入力・状態取得処理の一部は共通／基礎実装です。1.8.5版は段階的な生成処理で差分を適用します。暫定機能を置き換える**完全版の生成処理**を使います。

```powershell
$env:PYTHONUTF8 = '1'
python tools/prepare_185_full.py
```

`build/integration-185/` に出力します。これは原作Myth IIのソースコードではありません。原作EXEはユーザーが別途用意します。

## 開発用ビルド

既存スクリプトはAndroid NDK r27d、JDK 21.0.13+9、Android API 29、Build Tools 30.0.2、OpenXRローダー／ヘッダー、Apktool 2.11.1を使用します。過去のPowerShellスクリプト内のSDKパスは各環境に合わせる必要があります。実機検査・インストール用スクリプトもあるため、すべてを無差別に実行しないでください。

主な流れは、入力ヘルパー・ARM64状態取得／描画のビルド、Java/Dexの生成、開発用ランタイムAPKの加工、署名です。対象スクリプトは英語版資料に列挙しています。

**このチェックアウト単独でのAPK生成は未完成です。** `package_185_full.py` は非公開の開発candidate APKとローカル中間生成物を参照します。古いcandidateには原作データがあるため配布しません。`prepare_standalone_apk.py` も旧バイナリ統合用で、生成済みアセットを要求します。水面ノーマル画像も出所確認のため除外しています。公開入力から完結するビルド入口の整備が残っています。署名鍵は公開しません。将来のビルドでは自分の鍵を作り、更新時の署名互換性に注意してください。

## Managerのテスト

次は模擬データを使い、実機・原作不要で実行できます。

```powershell
python -m unittest discover -s tools -p test_plugin_manager.py
python -m unittest discover -s tools -p test_setup_deploy.py
```

他の開発テストには原作検証データ・コンパイラー・稼働中のゲーム・実機が必要なものがあります。すべてが単独CIテストではありません。原作変換は `tools/base_setup.py`、プラグイン変換は `tools/convert_plugin_levels.py` で、自分の原作入力を指定します。

mythextractはMITライセンス付きで同梱しています。Winlator参照ソースは固定コミット `2c33741d50c5daab0521a098be5c969f63b9e887` です。これは全ランタイム部品の対応ソースが揃ったという意味ではありません。[ライセンス情報](../THIRD_PARTY_NOTICES.md)を参照してください。
