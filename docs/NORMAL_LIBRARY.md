# Using the normal-map collection

**English** | [日本語](NORMAL_LIBRARY.ja.md)


[Download the normal-map collection](https://github.com/Snail921/Myth-II-for-Meta-Quest3-3S/releases/download/v0.3.0-beta/MythIIVR-NormalMaps-2026.11.18.zip)

Normal maps for 206 collections. Not every asset or animation has been visually checked. Original sprite images and editable projects are not included. The helper matches your own converted assets to create Quest packs.

## 1. Prepare

- Use Quest app **0.3.0-beta or later** and the latest Plugin Manager ZIP.
- Install Python 3.11 on Windows with “Add python.exe to PATH”.
- Extract this ZIP. Open a terminal in the extracted folder and run `python -m pip install -r requirements-normal-library.txt` once.
- Convert your own Myth II data or desired plugins with Plugin Manager first.

## 2. Create Quest packs

1. Double-click **Restore Normal Maps.cmd**.
2. Select a folder containing your converted collections, such as the conversion output's `levels` folder. Subfolders are searched for `.bin` collections too.
3. Choose an empty output folder, for example `UserData/DownloadedNormalPacks`.
4. Wait for `Done` in the console. Scanned files, matched images and restored packs are shown as progress.

Only matching assets become `.mvrnm` packs. A partially matching collection produces a pack containing only the matched frames. If you see `No matching source images`, select Manager's converted collections rather than your ISO or original Game folder. Repeat with other plugin conversions if needed.

## 3. Transfer to Quest

1. Stop the original game on Quest and connect USB.
2. Open **Plugin Manager.cmd**.
3. Set the normal-map folder to the output folder from step 2.
4. Click **Transfer normal maps to Quest**. Same-name packs are replaced.
5. Open **Myth II for Meta Quest 3 / 3S** on Quest, enable enhanced shadows or enhanced lighting, and start a level.

Matching images use the normals automatically. Do not repeat Initial setup. If the effect is too strong, remove the pack or generate your own at a different strength in the editor.

Restoration creates temporary matching data on the PC. Allow several GB of free space for downloads, temporary files and output.

[App and Manager downloads](https://github.com/Snail921/Myth-II-for-Meta-Quest3-3S/releases/tag/v0.3.1-beta) · [Material editor guide](https://github.com/Snail921/Myth-II-for-Meta-Quest3-3S/blob/main/docs/MATERIALS.md)
