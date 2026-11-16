# Installation and Plugin Manager

**English** | [日本語](INSTALL.ja.md) · [Home](../README.md)

## Availability

The public download currently supplies the manager/source and artwork/skyboxes. **A compatible APK and matching `runtime-seed.tar` are still required and are not included in this release.** New users can prepare/convert their own game files, but cannot finish a fresh headset installation from these downloads alone. The procedure below documents the tested development setup; do not substitute arbitrary runtime archives. See [release status](RELEASE_STATUS.md).

## Requirements

- Quest 3 or Quest 3S, developer mode and USB debugging enabled; a USB data cable.
- Windows PC with Python **3.11** including Tkinter, [7-Zip](https://www.7-zip.org/), and [Android SDK Platform Tools](https://developer.android.com/tools/releases/platform-tools). These programs are obtained separately.
- Your own supported Myth II CD ISO and Windows 1.8.5 updater. The importer extracts files; it does not execute the updater.
- At least **8 GiB plus optional HD addon sizes** free on the PC for base setup. More space is needed for plugin conversion and temporary transfer archives; use a drive with ample free space.
- Quest free space for extracted game data, converted levels, optional HD addons, runtime and deployment staging. The manager checks space before deployment; initial setup also requires room to expand the runtime.

Extract the source/manager package into a writable folder, preserving its directory structure. Avoid running from inside a ZIP. Open a terminal there:

```powershell
python -m pip install -r requirements-manager.txt
python -m tkinter
```

Close the Tkinter test window, then open **Plugin Manager.cmd**. Select **English** or **日本語** at the top. The `research/references/mythextract/scripts` folder is required and already included; do not remove it.

## Prepare base game data on the PC

1. Set **Work/output folder** to a writable location with enough space.
2. In the base-setup section, select the **ISO**, **1.8.5 updater**, and `7z.exe` files.
3. Optionally select the supplied **Skyboxes** folder.
4. Optionally select **Logo image** and **CD image** as two separate PNG files (`Assets/Logo.png` and `Assets/CD_Face.png`). Either may be omitted. Source filenames can differ; output names are normalized.
5. Use the HD-addon button to select separately acquired **archive files**, not ZIP downloads. Detail Texture Megapack and HD Megapack files are not supplied by this repository. If an addon is split into parts, select all required parts together. Archives must pass the supported ADDON validation.
6. Start extraction/conversion. The manager creates a new `setup-…` folder and shows internal steps/counts. Keep the complete output: `Game`, `LevelPacks/soulblighter`, `SharedAssets`, optional `Skyboxes`, and `setup-manifest.json`.

The supported 1.8.5 EXE SHA-256 is `aa68153d987bf8a17c30bf8c7711d3b5e0a291a6cd1f8630446496715539181c`. Base archives are checked against `tools/base-game-files.json`; other editions/localizations may be rejected. Renaming an incompatible file will not fix validation.

## Fresh headset setup (requires separately supplied development APK/runtime)

1. Enable developer mode using Meta's developer setup process. Connect USB and accept **Allow USB debugging** inside the headset. `adb devices` must show `device`, not `unauthorized`.
2. Install the compatible external-data APK using your sideloading tool or `adb install path/to/app.apk`. Open the app once from Unknown Sources to create its external layout, then leave the original game stopped.
3. In Manager, set **ADB** to your `adb.exe`; refresh the device list and select the headset. Use only the intended device.
4. Select **Setup result** and the matching **Runtime data** file. The seed hash is pinned in `tools/runtime-seed.sha256`; an arbitrary Winlator export is not interchangeable.
5. Click **Deploy setup data**. Keep USB connected and the headset awake. Files are staged and hash-checked. Existing `Game` data is deliberately not overwritten by fresh setup.
6. On Quest, choose **Initial setup / 初回データ展開**. Wait for completion, then **Start Myth II / 原作を起動**.
7. Start Willow Creek in the original menu. The game should transition into VR. Check terrain, units, buildings, information panel, selection and movement.

Do not uninstall an existing installation just to retry deployment: Android may remove app-specific data. Back up saves first if you intend to remove the app. Updating an APK requires a compatible signing certificate.

## Install a plugin

Use the upper section of Plugin Manager. A working base installation is required.

1. **Original game folder**: select the setup output's `Game` folder containing `Myth II.exe`, `tags` and `plugins`.
2. **Plugin file**: choose the actual gameplay archive (for example `Chimera Single-player` or `Magma - The Fallen Levels v2`), not its ZIP or parent folder.
3. Choose a work/output folder with enough space. Click **1. Inspect levels**, review missing dependencies or unsupported entries, then **2. Convert all levels**.
4. Stop the original game on Quest. Connect USB and choose the device.
5. Confirm **Converted bundle** points to the output folder containing `catalog.tsv` and `source-files.json`. Click **3. Deploy to Quest**. The original plugin and converted pack are checked together; hashes are checked after transfer.
6. Start Myth II and select that plugin's level in the original menu. Matching VR packs are selected automatically; no APK rebuild or initial setup repeat is needed.

For an already converted bundle, set its folder and its original plugin, then deploy directly; reconversion is unnecessary. Keep **TFL Gameplay OFF** for The Fallen Levels. Multiple gameplay-plugin combinations, Unity mode and automatic dependency-plugin resolution are not supported. Installing a skybox alone does not install a level. Plugin cutscene extras are not automatically installed by this gameplay-archive workflow.

## HD textures and sprites

Base setup copies the selected supported HD addons into `Game/plugins`. The app reads external addon images; you do not need to rebuild the APK. The project does not redistribute community packs. Enable the corresponding terrain detail / high-resolution image / detailed shadow options on the Quest launcher and reload the level after changing them. Adding gameplay plugins is a separate manager workflow; its plugin picker is not a general addon installer.

## External layout

Under `/sdcard/Android/data/org.mythvr.standalone/files/`:

```text
Game/                     Original EXE, tags, plugins and user data
LevelPacks/soulblighter/   Converted base campaign
LevelPacks/plugin-…/      Converted gameplay plugin packs
SharedAssets/             Converted UI assets; optional Logo.png/CD_Face.png
Skyboxes/                 Optional <level display name>.png/jpg/jpeg
runtime-seed.tar          Matching initial runtime archive
```

Do not modify pack files individually after conversion: manifests validate sizes and SHA-256. Use the manager to replace a complete pack. Backups in hidden `.backup-…` folders remain on device and consume storage; they are not automatically deleted.

## Troubleshooting

- **No device / unauthorized:** verify developer mode, USB debugging prompt, cable and `adb devices`. Do not start transfers before authorization.
- **Original game running:** exit the game before deployment; Manager does not end active play automatically.
- **VR does not start:** check matching executable, original plugin, pack manifests and active plugin order. Enable launcher diagnostics and report the level and exact error.
- **Transfer appears frozen:** elapsed time alone is not proof of progress. A known development issue can leave `adb push` waiting even after bytes reach Quest. Check `work/output/deployments/*.json`; do not delete installed packs. On Windows, identify the specific `adb.exe push` child of this manager in a process tool and end **only that transfer client**, then wait for Manager's error/cleanup before closing or retrying. Do not kill all ADB/Python processes or unplug during publication. This release documents the issue; it does not claim to contain a new cancellation/timeout fix.
- **Insufficient space / large pack rejected:** choose a larger PC work drive. Current pack limits remain 512 MiB per file and 1 GiB per level; increasing them does not create more runtime memory.

When reporting a failure, include the manager step, level/plugin name, error and relevant log. Avoid sharing original game archives or personal device identifiers.
