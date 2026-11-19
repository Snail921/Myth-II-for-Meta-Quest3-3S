# Myth II for Meta Quest 3 / 3S

[English](README.md) | **日本語**

Myth II: SoulblighterをQuest 3 / 3Sで遊ぶ非公式VR版です。戦場を立体で見渡し、コントローラーで部隊を選択・操作できます。プレイ中のPC接続は不要です。

**APK・初回ランタイム・Plugin Managerを下のリンクからダウンロードできます。** 原作のISOと1.8.5アップデーターは別途ご用意ください。

[ダウンロード](https://github.com/Snail921/Myth-II-for-Meta-Quest3-3S/releases/tag/v0.4.0-beta) · [導入手順](docs/INSTALL.ja.md) · [操作方法](docs/CONTROLS.ja.md)

<p>
  <a href="https://raw.githubusercontent.com/Snail921/Myth-II-for-Meta-Quest3-3S/main/docs/images/screenshot-01.png"><img src="docs/images/screenshot-01.png" height="120" alt=""></a>
  <a href="https://raw.githubusercontent.com/Snail921/Myth-II-for-Meta-Quest3-3S/main/docs/images/screenshot-02.png"><img src="docs/images/screenshot-02.png" height="120" alt=""></a>
  <a href="https://raw.githubusercontent.com/Snail921/Myth-II-for-Meta-Quest3-3S/main/docs/images/screenshot-03.png"><img src="docs/images/screenshot-03.png" height="120" alt=""></a>
  <a href="https://raw.githubusercontent.com/Snail921/Myth-II-for-Meta-Quest3-3S/main/docs/images/screenshot-04.png"><img src="docs/images/screenshot-04.png" height="120" alt=""></a>
  <a href="https://raw.githubusercontent.com/Snail921/Myth-II-for-Meta-Quest3-3S/main/docs/images/screenshot-05.png"><img src="docs/images/screenshot-05.png" height="120" alt=""></a>
  <a href="https://raw.githubusercontent.com/Snail921/Myth-II-for-Meta-Quest3-3S/main/docs/images/screenshot-06.png"><img src="docs/images/screenshot-06.png" height="120" alt=""></a>
</p>

## 遊ぶまでの流れ

1. **PCで準備する** — 自分のMyth II CD ISOと1.8.5アップデーターを用意し、Plugin Managerで変換します。
2. **Questへ入れる** — APKをインストールし、USB接続してManagerからデータを転送します。
3. **起動する** — Questのアプリライブラリ →「提供元不明」→ **「Myth II for Meta Quest 3 / 3S」** を開き、「初回データ展開」を一度実行し、「原作を起動」を押します。
4. **遊ぶ** — 原作メニューでレベルを開始するとVRに切り替わります。

必要なソフトと各画面の入力内容は、[導入手順](docs/INSTALL.ja.md)にまとめています。原作ゲームと追加プラグインは別途必要です。

Myth II 1.8.5パッチ、高精細マップテクスチャ・ハイレゾスプライト（Twice Born Edition）、The Fallen Levelsなどのプラグインは、[Project Magma](https://projectmagma.net/)から入手できます。

## 対応状況

| ゲーム | 確認状況 |
| --- | --- |
| Myth II: Soulblighter 1.8.5 | チュートリアル完了、Willow Creek等でプレイ確認済み |
| Chimera | 全8面のVR起動を確認済み |
| The Fallen Levels | 基本動作を確認済み。通しプレイは確認中。TFL GameplayはOFFで使用 |

すべてのプラグインに対応しているわけではありません。まず原作で動作を確認してから追加してください。

## 追加できるもの

- **キャンペーン：** Managerでプラグインを変換・転送します。
- **高精細画像：** 別途入手したHDアドオンをセットアップ時に指定します。
- **スカイボックス：** 配布ZIPを展開し、Managerでそのフォルダを指定します。

---

開発者向け：[ソースのビルド](docs/BUILD.ja.md) · [リリース内容](docs/RELEASE_STATUS.ja.md) · [ライセンス](THIRD_PARTY_NOTICES.md)

## 操作方法

[操作方法を開く](docs/CONTROLS.ja.md) — 選択・移動・グループ・Co-op割譲・ゲーム速度の操作をまとめています。

## 更新履歴

**0.4.0-beta** — アトラス化・まとめ描画、水面反射の軽量化、天候修正、UI色・太さ設定、右グリップ＋Bの選択解除。

**0.3.2-beta** — ハッシュ形式のノーマルパック。復元不要で転送でき、ファイル容量とVR読み込み時間を削減。

**0.2.0-beta** — 描画品質設定、Co-op割譲、方向指定・グループ操作の改善、影とスプライト表示の修正。

[すべての更新履歴](CHANGELOG.ja.md)

[描画設定・FPS](docs/GRAPHICS.ja.md) · [スプライト材質エディタ](docs/MATERIALS.ja.md)

[ノーマルマップ集（206コレクション）の導入](docs/NORMAL_LIBRARY.ja.md)
