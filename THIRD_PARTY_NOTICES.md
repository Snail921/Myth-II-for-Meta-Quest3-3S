# Third-party notices

Original project code and documentation use the [MIT license](LICENSE). Upstream components retain their own licenses. Winlator integration changes follow the upstream LGPL terms.

## Winlator and runtime

- [Winlator](https://github.com/brunodev85/winlator) 11.2, [application source](https://github.com/brunodev85/winlator-app/tree/2c33741d50c5daab0521a098be5c969f63b9e887), LGPL-2.1. [License text](LICENSES/Winlator-LGPL-2.1.txt).
- [Wine 10.10 Custom](https://github.com/brunodev85/wine-10.10-custom/tree/2b9afc38d9531d384f038bbaaf3f637e515fb297), LGPL-2.1-or-later; individual files retain their notices.
- [Box64](https://github.com/ptitSeb/box64), [Mesa](https://www.mesa3d.org/), [DXVK](https://github.com/doitsujin/dxvk), [VKD3D](https://gitlab.winehq.org/wine/vkd3d), [CNC DDraw](https://github.com/FunkyFr3sh/cnc-ddraw), and [OpenXR](https://github.com/KhronosGroup/OpenXR-SDK) retain their upstream licenses. [OpenXR license](src/vr/licenses/OpenXR-loader.txt).
- The runtime also includes GLib, glibc, ALSA, PulseAudio, GStreamer, GnuTLS, Nettle, libsndfile, mpg123, libmpeg2, 7-Zip and other upstream support libraries. Their source archives include copyright and license texts.

The **Upstream-Sources** release archive contains the collected upstream sources, license texts and an index of archive hashes. Winlator application sources are separated from bundled binaries; the build uses the fixed upstream APK described in [BUILD.md](docs/BUILD.md). Runtime binary paths and hashes are listed in [runtime-components.json](docs/runtime-components.json).

Project modifications include Android manifest/package integration, launch and stop callbacks, background simulation, cursor and controller forwarding, runtime path relocation, and the VR bridge. See `tools/patch_public_runtime.py`, `tools/build_public_runtime.py`, `src/standalone/`, and `tools/prepare_185_full.py`. No restriction is imposed on modifying or replacing the LGPL components for debugging those modifications. Generate your own signing key for a rebuilt APK; Android requires matching signatures for in-place updates.

## mythextract

Copyright 2025 James Wheare. [Source](https://github.com/jwheare/mythextract/tree/786a3e4fc205a037bc2e309667ccfd8ccde3d77a), [MIT license](research/references/mythextract/LICENSE). Python readers are vendored in `research/references/mythextract/scripts/`. The compatibility copy made by `tools/level_assets.py` removes an unused `mons2stats` import for Python 3.11; vendored sources are unchanged.

## Game content

Myth II names, trademarks and game artwork belong to their respective rightsholders. This project is unofficial. Original game files, converted game levels and community gameplay/HD plugin packs are not included. Screenshots show gameplay. Launcher artwork and skyboxes are separate from the source-code license.

## 日本語

独自コード・文書はMIT、Winlatorの改変部分は上流のLGPL条件に従います。同梱ライブラリにはそれぞれのライセンスが適用されます。上流ソースとライセンス文はReleaseの **Upstream-Sources** にまとめています。原作データ、変換済みレベル、第三者のキャンペーン・HDプラグインは配布に含めません。
