# Installation guide

Use the APK, PC tools and PBR material pack from this prerelease together. Extract the material ZIP and select the folder directly containing the 89 .mvrnm files in Plugin Manager to transfer them to Quest. A first installation also needs the shared runtime linked in the release.


**English** | [日本語](INSTALL.ja.md) · [Home](../README.md)


## 1. Get what you need

- Quest 3 / 3S and a USB data cable
- A Windows PC
- Your own Myth II CD ISO and Windows 1.8.5 updater
- Manager, APK and initial runtime from [Releases](https://github.com/Snail921/Myth-II-for-Meta-Quest3-3S/releases/tag/v0.5.0-pbr-like.3)

The Myth II 1.8.5 patch, high-resolution map textures and sprites (Twice Born Edition), and plugins such as The Fallen Levels are available from [Project Magma](https://projectmagma.net/).

Install the following software on your PC to create and transfer VR data:

| Software | Purpose |
| --- | --- |
| [Python 3.11](https://www.python.org/downloads/windows/) | Runs Manager. Enable “Add python.exe to PATH” during installation |
| [7-Zip](https://www.7-zip.org/) | Extracts the ISO and updater |
| [Android Platform Tools](https://developer.android.com/tools/releases/platform-tools) | Connects to Quest. Extract its ZIP |

Extract the Manager ZIP. Open a terminal in that folder and run this once:

```powershell
python -m pip install -r requirements-manager.txt
```

Double-click **Plugin Manager.cmd**. Choose English or Japanese at the top right.

Extract the runtime ZIP too. Select **runtime-seed.tar** from it in step 3.

## 2. Convert the game on your PC

Set **Work/output folder** to a drive with enough free space. Base setup needs at least 8 GiB, plus room for HD addons and conversion/transfer files.

In the **Setup** tab, choose:

| Field | Select |
| --- | --- |
| ISO | Your Myth II CD ISO |
| 1.8.5 updater | The Windows updater EXE |
| 7-Zip | Usually `C:\Program Files\7-Zip\7z.exe` |
| Skyboxes (optional) | The `Skyboxes` folder from the extracted skybox ZIP |
| Logo / CD image (optional) | `Assets/Logo.png` and `Assets/CD_Face.png` from Manager. Choose each file separately |

For HD graphics, use **Add optional HD addons** and select the separately obtained archive files. Extract downloaded ZIPs first. For Project Magma’s Twice Born Edition, select both Magma - HD Megapack 1of2 and Magma - HD Megapack 2of2.

Click **Extract original and convert all levels** and wait for completion. **Setup result** will point to the output folder. Keep that folder for later use.

## 3. Transfer to Quest

1. Enable developer mode on Quest, connect USB and accept **Allow USB debugging** inside the headset.
2. Install the compatible APK using your sideloading tool, or run `adb install path/to/app.apk`.
3. Open the Quest app library, select **Unknown Sources** in the list filter, and click **Myth II for Meta Quest 3 / 3S** once. Allow file access when prompted. Do not start the original game yet.
4. In Manager, set **ADB** to **adb.exe** from Platform Tools. Refresh the device list and select your Quest.
5. Set **Setup result** to the output from step 2, and **Runtime data** to the matching **runtime-seed.tar**.
6. Click **Transfer setup data to Quest**. Keep USB connected until completion.

Fresh setup does not overwrite an existing game installation. Use the plugin steps below to add campaigns.

## 4. Start playing

1. Put on your Quest and open the app library.
2. Select **Unknown Sources** in the list filter.
3. Point at **Myth II for Meta Quest 3 / 3S** with the right controller and click it with the right trigger. This opens the app launcher.
4. In the launcher, click **Initial setup** and wait for completion. This is only needed once.
5. In the same launcher, click **Start Myth II**.
6. When the original game menu appears, select **New Game** and start Willow Creek.
7. When the battlefield switches to VR, setup is complete. You can now disconnect the PC.

For later sessions, choose **Unknown Sources** → **Myth II for Meta Quest 3 / 3S** → **Start Myth II**.

Try these controls with the default control settings:

| Action | Input |
| --- | --- |
| Point | Right controller pointer |
| Select a friendly unit | Point at it and press the left trigger |
| Move selected units | Point at the ground and press the left trigger |
| Attack | Point at an enemy and press the left trigger |
| Move the view | Left stick |
| Orders / formations menu | A / X |
| Cycle registered groups | Y for previous, B for next with default settings |

## 5. Add a plugin

Once the base game works, switch to the **Plugins & materials** tab at the top of Manager:

1. Set **Original game folder** to **Game** inside your setup output.
2. Select the extracted gameplay archive as **Plugin file**, such as `Chimera Single-player` or `Magma - The Fallen Levels v2`.
3. Run **1. Inspect levels**, then **2. Convert all levels**.
4. Exit the original game on Quest, connect USB and click **3. Deploy to Quest**.
5. Start the game and select an added level. You do not need to repeat Initial setup.

For an existing conversion, choose its **Converted bundle** folder and deploy directly. Keep **TFL Gameplay OFF** for The Fallen Levels.

## Troubleshooting

| Problem | Check |
| --- | --- |
| Quest not found | USB data cable, developer mode and debugging permission inside the headset |
| Game is running | Exit the original game before transferring |
| Unsupported input file | Use the supported 1.8.5 updater and game data; renaming files will not fix compatibility |
| No VR transition | Original plugin and converted pack must match |
| HD graphics do not change | Enable the corresponding launcher options and reload the level |

[Report a problem](https://github.com/Snail921/Myth-II-for-Meta-Quest3-3S/issues)

## Controls and updates

[Controls](CONTROLS.md) · [Changelog](../CHANGELOG.md)

To update, install the new APK over the existing app. Do not uninstall or repeat Initial setup.

Optional features: [Graphics and FPS](GRAPHICS.md) · [Normal-map creation and transfer](MATERIALS.md)

## Convert base or additional multiplayer maps

Select the original game folder in Manager. Leave the plugin field blank for base maps, or select the gameplay plugin for additional maps. Use **1 Inspect levels** to check the type (single-player, multiplayer or both) and validation result, then **2 Convert all levels** and **3 Deploy to Quest**. Exit the original game before deployment.

Base-pack updates replace LevelPacks only; they do not replace the game, saves or preferences under Game. Standard data contains 27 single-player and 20 multiplayer maps. Incomplete maps, missing game modes and missing assets remain checked. A supported conversion result does not certify every multiplayer rule in VR. Use matching game/plugin data on all peers.

## Manager layout and upgrading to 0.5.0

Manager has **Setup** and **Plugins & materials** tabs. The shared log can be enlarged by dragging the divider. Manager, Sprite Editor and the Quest launcher default to English; saved Japanese preferences are retained.

New base setup converts single-player and multiplayer maps together. Below **Add optional HD addons** are **Extract original and convert all levels**, **Setup result**, then **Transfer setup data to Quest**. Setup result is the PC conversion output folder, reusable for later transfer.

Update the APK in place; do not repeat runtime Initial setup. The new Manager extracts the original pen/eraser artwork into SharedAssets. If upgrading older setup data without those images, convert your own game into a new PC folder with the new Manager, exit the original game, then transfer only that output's SharedAssets as below. Do not overwrite the existing Game folder or saves.

Replace the PC output path with your actual location. Restart the app after transfer.

    adb push "C:/path/to/new-setup/SharedAssets" /sdcard/Android/data/org.mythvr.standalone/files/


For the separate Preview 4 noShadow pack and terrain reconversion steps, see [noShadow setup](NOSHADOW.md). Keep the existing normal/PBR libraries.
