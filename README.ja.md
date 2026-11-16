# Myth II for Meta Quest 3 / 3S

[English](README.md) | **日本語**

**Myth II: Soulblighter** をMeta Quest 3 / Quest 3Sで遊ぶための非公式VR移植です。原作Windows版をヘッドセット内の互換環境で動かし、OpenXRによるVR描画・コントローラー入力を接続します。プレイ中にPCは必要ありません。

> **公開状況 — 2026-11-16:** 今回はプロジェクトのソース、Windows用Python版Plugin Manager、任意の画像素材、160枚のスカイボックスを公開しています。**APKと初回ランタイムは、同梱部品の再配布条件・対応ソースの確認が終わるまで未公開です。** この公開物だけで新しい端末への初回導入は完結しません。[公開状況](docs/RELEASE_STATUS.ja.md)をご確認ください。

[ダウンロード](https://github.com/Snail921/Myth-II-for-Meta-Quest3-3S/releases) · [導入マニュアル](docs/INSTALL.ja.md) · [ビルド資料](docs/BUILD.ja.md) · [ライセンス・クレジット](THIRD_PARTY_NOTICES.md) · [画像集](docs/GALLERY.md)

![Willow Creekのユニットと水面反射](docs/images/screenshot-01.png)

## 主な機能

- 両眼VR描画、コントローラー追従、ユニット選択、命令、隊形、グループ操作。
- 手元の情報パネルとミニマップ、原作の目的表示・ナレーション・シナリオカメラとの同期。
- 水面反射、波、霧、雨、雪、戦闘エフェクト。
- APK更新なしで読み込む外部レベルパックとレベル別スカイボックス。
- 別途入手したアドオンの高精細地形・高解像度スプライト／モデル表面画像・詳細な影。
- 起動画面とPlugin Managerの英語／日本語切り替え。

## 確認状況と対応範囲

外部データ版の開発APKで、新しいQuest 3への初回導入とWillow Creekのプレイを確認しました。Chimeraは全8面のVR移行を確認済みです。The Fallen Levelsは配置・基本描画・操作・FPSを確認しており、通しプレイの検証は継続中です。任意のプラグインの動作を保証するものではありません。

対応する **Myth II 1.8.5** の実行ファイル・原作データを使用してください。**The Fallen LevelsはTFL GameplayをOFF**にして通常のMyth IIルールで使用します。複数ゲームプレイプラグインの組み合わせやUnity/TFL Gameplayは未対応です。巨大なパックはメモリ・容量制限に達することがあり、現時点でMazzarin's Demise VIは対応対象に含めません。

## はじめに

1. [導入マニュアル](docs/INSTALL.ja.md)と[公開上の制約](docs/RELEASE_STATUS.ja.md)を確認します。
2. ご自身のMyth II CD ISOと対応する1.8.5アップデーターを用意します。原作データやコミュニティプラグインは同梱していません。
3. Windows PCにPython 3.11（Tkinter含む）、Pillow、7-Zip、Android Platform Toolsを用意します。ManagerはPythonソース版で、単体EXEではありません。
4. `python -m pip install -r requirements-manager.txt` を実行後、`Plugin Manager.cmd` を開きます。
5. PCで原作データを変換します。実機への配置には対応APKと一致するランタイムが必要ですが、この2点は公開準備中です。

既に稼働する環境がある場合は、別途入手した対応プラグインの検査・変換・配置にManagerを利用できます。

## フォルダ構成

| パス | 内容 |
| --- | --- |
| `src/` | VR描画、Android起動画面、状態取得・入力のソースと1.8.5対応 |
| `tools/` | Manager、変換ツール、開発用ビルドスクリプト、テスト |
| `research/references/mythextract/` | MITライセンスの固定版パーサーとライセンス文 |
| `Assets/` | Snail921作成の任意のロゴ・CD画像、UIマーカー |
| `Skyboxes/` | 160枚の生成パノラマと公開用メタデータ |
| `docs/` | 英日マニュアル、ビルドの制約、サンプル画像 |

非公式コミュニティプロジェクトです。Bungie、Project Magma、Meta、Winlator開発者による公認・提携ではありません。原作の名称・画像などの権利は各権利者に帰属します。画像はプレイの紹介用です。[素材の利用条件](Assets/README.md)と[第三者ライセンス](THIRD_PARTY_NOTICES.md)をご確認ください。
