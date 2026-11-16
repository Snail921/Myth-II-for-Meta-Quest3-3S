# APKとランタイムのビルド

[English](BUILD.md) | **日本語**

上流のWinlator 11.2 APKと、このリポジトリからビルドします。開発用APK、原作データ、実機から書き出した環境は不要です。

## 準備（Windows）

Python 3.11をインストールし、`pip install -r requirements-build.txt` を実行します。

| ツール | 配置先 |
| --- | --- |
| JDK 21.0.13+9 | `tools/runtime/jdk-21.0.13+9` |
| Android NDK r27d | `tools/runtime/android-ndk-r27d` |
| OpenXR loader AARの展開内容 | `tools/runtime/openxr/loader` |
| Apktool 2.11.1 | `tools/downloads/apktool_2.11.1.jar` |

Android SDK platform 29とbuild-tools 30.0.2をインストールし、`ANDROID_HOME` にSDKのフォルダを指定します。既定値は `%LOCALAPPDATA%/Android/Sdk` です。

[Winlator 11.2](https://github.com/brunodev85/winlator/releases/tag/v11.2.0) を `tools/downloads/Winlator_11.2.apk` に置きます。スクリプトがファイルのSHA-256を検証します。

## 実行

Android署名用のkeystoreを自分で作成してください。リポジトリの外で保管し、更新時にも同じ鍵を使います。

```powershell
$env:PYTHONUTF8 = '1'
python tools/build_public_runtime.py --winlator-apk tools/downloads/Winlator_11.2.apk --out build/runtime-seed.tar
$env:MYTH_SIGNING_PASSWORD = '署名鍵のパスワード'
python tools/build_public_apk.py --winlator-apk tools/downloads/Winlator_11.2.apk --runtime build/runtime-seed.tar --work build/public-apk --out build/MythIIVR.apk --keystore C:/path/to/your.keystore
```

出力先・作業先は未使用のパスを指定します。APKには対応するランタイムのチェックサムが入り、各ファイルの隣にハッシュを記録したJSONが生成されます。

ランタイムは上流の初期コンテナから生成します。APKはWindows用ヘルパー、ARM64描画・接続部、Android画面をコンパイルし、Winlatorへの組み込みと署名を行います。原作データと任意のネイティブWindowsコンポーネントは含めません。

## Managerのテスト

```powershell
python -m unittest discover -s tools -p test_plugin_manager.py
python -m unittest discover -s tools -p test_setup_deploy.py
```

過去の診断スクリプトには実機や原作ファイルが必要なものもあります。APKの生成には上記の `build_public_*` を使ってください。

上流ソースと利用条件は[ライセンス](../THIRD_PARTY_NOTICES.md)を参照してください。
