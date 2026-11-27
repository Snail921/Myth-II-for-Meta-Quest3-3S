# Using the normal-map collection

**English** | [日本語](NORMAL_LIBRARY.ja.md)

[Download the normal-map collection](https://github.com/Snail921/Myth-II-for-Meta-Quest3-3S/releases/download/v0.3.2-beta/MythIIVR-NormalMaps-0.3.2-beta.zip)

Normal packs for 206 collections. Not every asset or animation has been visually checked. Original images and editable projects are not included.

1. Install the **0.3.2-beta or later APK and Manager**. Install over the existing app; do not repeat Initial setup.
2. Extract the normal-map ZIP.
3. Stop the original game on Quest and connect USB.
4. Open **Plugin Manager.cmd** and select the extracted **NormalPacks** folder as the normal-map folder.
5. Click **Transfer normal maps to Quest**. Same-name packs are replaced; previous copies are not retained on Quest.
6. Open **Myth II for Meta Quest 3 / 3S**, enable enhanced shadows or enhanced lighting, and start a level.

No source-image selection, restoration helper or normal regeneration is needed. Normals apply automatically to matching image contents. Different HD images need matching packs.

Legacy packs remain readable. See the [material editor guide](MATERIALS.md) to convert your own packs. APK versions 0.3.1-beta and earlier cannot read the new format.

[App and Manager downloads](https://github.com/Snail921/Myth-II-for-Meta-Quest3-3S/releases/tag/v0.3.2-beta)


## Preview 4 — unified shadows and noShadow (2026-11-28)

Adds unified terrain/building/sprite shadows, cached static shadows and dynamic map resolutions through 8K; terrain base-color export/loading, interpolated terrain normals and shared mesh shader-state setup; independent shadowed/unshadowed projects, noShadow albedo priority over unedited HD textures, per-image normal inversion and the exit-recovery fix. Publishes a separate noShadow pack (10 collections / 97 images), updated APK and Manager/source ZIP. Existing normal/PBR packs, runtime and skyboxes remain available. Current performance is approximately 50–60 FPS as a guide due to rendering load and incomplete optimization, not a guaranteed rate. [noShadow setup and validation limits](NOSHADOW.md).
