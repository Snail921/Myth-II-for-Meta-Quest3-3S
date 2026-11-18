# Create sprite normal maps

**English** | [日本語](MATERIALS.ja.md) · [Home](../README.md)

Normal maps are optional. Use your own converted game/HD data; game sprites are not included in the download.

1. Extract the latest Manager/source ZIP and install Python and dependencies as described in the installation guide.
2. Open **Sprite Material Editor.cmd**, or the Sprite Material Editor button in Plugin Manager.
3. Choose **Collection folder**, select a folder containing converted collections, and open a thumbnail. Alternatively, use **Select unit** with the converter's `scene/scene.json`.
4. Review the frames and optionally correct heights with color masks or manual mask painting. Image brightness includes painted colors and baked lighting, so direct conversion can produce unwanted relief.
5. Adjust normal strength and inspect the normal preview. You can enter a value directly.
6. **Save** writes an editable `.smproj` project. **Export all** writes a ZIP containing heights, normals, masks and mapping data.
7. **Export for Quest** writes a `.mvrnm` pack to your chosen folder, for example `UserData/SpriteMaterials`.
8. Stop the game on Quest. In Plugin Manager, select that folder under normal maps and click **Transfer normal maps to Quest**.
9. Enable enhanced shadows or enhanced lighting on Quest and reload the level. Matching normal maps apply automatically.

The transfer reads `.mvrnm` files directly inside the selected folder. Same-name packs are replaced; old copies are not retained on Quest. Give different materials different filenames. Runtime matching uses original image contents, not filenames alone.

**Open collection** reopens the session's cached collection list. Each file operation remembers its own last folder. Adjustment values persist when opening another collection; loading a saved project restores that project's settings.

Quest packs are stored in `Android/data/org.mythvr.standalone/files/SpriteMaterials`. The 64MiB limit is per pack, not for the entire folder. Only needed normal images are loaded, and they are released when moving to another level.

[Install the downloadable normal-map collection](NORMAL_LIBRARY.md)
