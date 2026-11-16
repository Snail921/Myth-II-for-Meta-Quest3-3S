# Licenses and credits / ライセンス・クレジット

## Project code

Original project source and documentation are provided under the [MIT license](LICENSE). This does not relicense upstream code or original game assets. Changes applying to Winlator code retain the applicable upstream LGPL terms. No claim is made that the entire development APK is MIT-licensed.

## mythextract

- Author: James Wheare; copyright 2025.
- Source: https://github.com/jwheare/mythextract
- Pinned commit: `786a3e4fc205a037bc2e309667ccfd8ccde3d77a`.
- Vendored Python readers: `research/references/mythextract/scripts/`.
- [MIT license](research/references/mythextract/LICENSE).
- `tools/level_assets.py` makes a build-directory compatibility copy and removes the unused `mons2stats` import from `mons_tag.py` for Python 3.11. Vendored source remains unmodified.

## Winlator reference and integration changes

- Project: https://github.com/brunodev85/winlator
- Source: https://github.com/brunodev85/winlator-app
- Reference checkout: `2c33741d50c5daab0521a098be5c969f63b9e887` (versionName 11.2).
- [LGPL 2.1 text](LICENSES/Winlator-LGPL-2.1.txt).
- This repository contains the project's integration/patch scripts, not a claim to complete corresponding source for every prebuilt component of the upstream APK.
- Development modifications change the Android manifest/package, launch/stop callbacks, background simulation behavior, cursor/input forwarding and process-path relocation. See `tools/prepare_standalone_apk.py`, `src/standalone/`, and `tools/prepare_185_full.py`.

LGPL licensing permits redistribution when its conditions are met; it is **not a prohibition on publishing an APK**. A generic source homepage is not evidence that the exact source and modifications for every shipped library have been accounted for. The outstanding binary-component audit is described in [release status](docs/RELEASE_STATUS.md).

## Other runtime projects

The development environment uses Wine, Box64, Mesa graphics drivers, DXVK/VKD3D, CNC DDraw, OpenXR, PulseAudio and other libraries. They retain their respective licenses. The OpenXR license text already used by this project is in [src/vr/licenses/OpenXR-loader.txt](src/vr/licenses/OpenXR-loader.txt). The public source/assets release does not redistribute those runtime binaries. The development APK inventory is provided as [runtime-components.json](docs/runtime-components.json), not as a license clearance certificate.

Upstream projects:

- Wine: https://www.winehq.org/ — LGPL-2.1-or-later (other bundled files can carry their own notices).
- Box64: https://github.com/ptitSeb/box64
- Mesa: https://www.mesa3d.org/
- DXVK: https://github.com/doitsujin/dxvk
- VKD3D: https://gitlab.winehq.org/wine/vkd3d
- CNC DDraw: https://github.com/FunkyFr3sh/cnc-ddraw
- OpenXR SDK: https://github.com/KhronosGroup/OpenXR-SDK

## Artwork and game content

`Assets/Logo.png` and `Assets/CD_Face.png` are supplied by Snail921, who identified them as their creations and authorized use/distribution with this project. See [asset notes](Assets/README.md). Skyboxes are generated artistic backgrounds prepared for this project; see [skybox notes](Skyboxes/README.md). Screenshots are user-supplied gameplay illustrations. These materials are separate from the MIT code license. Third-party Myth names, trademarks and underlying imagery remain with their respective rightsholders.

No Myth II ISO, game executable, proprietary DLL, archive, converted level pack, commercial/community HD pack or full scenario text is included. Users supply their own game and plugin files. The project is unofficial and does not imply endorsement by original game, runtime or headset developers.

## 日本語要約

独自コード・文書はMIT、mythextractは同梱のMIT、Winlatorの改変部分は上流のLGPL条件を引き継ぎます。原作画像・商標や第三者アドオンの権利をこれらのライセンスで再許諾するものではありません。WineのLGPLはAPK配布禁止ではなく、ソース提供等の条件を満たす必要があります。本公開物にはランタイムのバイナリを含めていません。ロゴ・CD画像は作者Snail921の利用許可に基づく同梱、スカイボックスは本プロジェクト用の生成画像です。原作データはユーザーが用意します。
