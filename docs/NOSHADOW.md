# noShadow material pack and unified shadows

**English** | [日本語](NOSHADOW.ja.md)

The noShadow pack is a separate optional download, alongside the existing normal-map library and PBR normal-map pack. It does not replace either library. This first pack covers **10 Willow Creek building collections / 97 images**, including the windmill and tavern. It is not a complete noShadow conversion of every level or sprite.

[Download noShadow Materials 1](https://github.com/Snail921/Myth-II-for-Meta-Quest3-3S/releases/download/v0.5.0-pbr-like.4/MythIIVR-noShadow-Materials.1.zip)

## Install

1. Install the **Preview 4 APK (0.5.0-pbr-like.4)** and use its matching Plugin Manager. Earlier releases cannot read this pack format.
2. Keep the existing normal/PBR packs installed. Extract this ZIP separately and select its **NoShadowPacks** folder (directly containing `.mvrnm` files) in Manager's normal-map folder selector, then transfer normal maps to Quest. Preserve the `--unshadowed` filenames.
3. For full terrain support, reconvert the affected level packs from your own game/plugin data with the updated Manager and transfer them. Conversion now includes terrain base colors without the original light/dark layer. Do not rerun Initial setup over an existing Game folder. The material ZIP does not contain converted terrain data.
4. Enable **Enhanced shadows → Unified shadow mode** and re-enter VR. PBR category switches remain independent: enable meshes for mesh PBR shading; noShadow albedo selection itself follows unified shadow mode.

With unified mode ON, matching noShadow albedo/materials take priority. With it OFF (or Enhanced shadows OFF), the noShadow entries are ignored and the normal original-albedo path is used. Keep the previous normal/PBR packs to retain their material settings in that path. Missing noShadow materials leave the original albedo, so unsupported buildings can still contain baked shadows. Edited standard-resolution albedo takes priority over an unedited HD texture for the same source binding.

Old terrain packs still load, but without the new base-color data they retain original baked terrain shading. Reconvert them to avoid combining the old shading with the new approach. Preserve global terrain tint and surface effects; only the baked lighting layer is omitted from the base color.

## Performance and scope

**Expect approximately 50–60 FPS as a rough current guide due to rendering load and incomplete optimization**, especially with unified shadows enabled. This is not a guaranteed frame rate; scene, headset and graphics settings can move performance outside that range. PBR and unified shadows are independently selectable.

Dynamic shadow-map resolution offers **512 / 1024 / 2048 / 4096 / 8192**, default 2048, applied on the next VR entry. 8K is available for future hardware; it increases memory/rendering cost and is not a recommended Quest setting. Static terrain/building/tree shadow casters are cached until invalidated; units and animated meshes update dynamically. Terrain uses interpolated height-derived normals to reduce visible triangle shading boundaries.

The shadow removal is estimated with image generation, not a recovery of an original unlit source. Fine texture details can differ. Newly processed buildings use the tavern's material recipe (normal strength 6, inverted normals and corresponding metallic/smoothness processing); windmill and tavern keep their existing authored settings. The new eight-building exports passed file/identity/alpha validation; every surface has not yet been visually verified on Quest.

## Authoring and files

Keep original and noShadow projects separate. Duplicate the original as an unshadowed project, replace its albedo, then author the material maps. The Editor supports a normal-inversion switch per image and saves it in project version 8. Ordinary exports remain `MVRMAT05`; noShadow exports use `MVRMAT06` with the source identity, original reference pixels, replacement albedo and material data. The pack includes these required reference/replacement images; it does not include editable `.smproj` projects.

Session recovery now follows the active albedo variant without blocking normal exit; ordinary saves still prevent overwriting the opposite variant. Save original and unshadowed projects under separate names.

[Material Editor](MATERIALS.md) · [Graphics settings](GRAPHICS.md) · [Normal library](NORMAL_LIBRARY.md)
