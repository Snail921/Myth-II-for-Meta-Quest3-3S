# Graphics settings and FPS

**English** | [日本語](GRAPHICS.ja.md) · [Home](../README.md)

On Quest, open App Library → Unknown Sources → **Myth II for Meta Quest 3 / 3S**, then expand the graphics settings. Changes apply on the next VR entry.

| Setting | What it does |
| --- | --- |
| Resolution, refresh rate, MSAA | Balance image quality and load. The requested refresh rate is a target, not a guaranteed FPS |
| FPS display | Shows a small FPS readout at the minimap’s upper-left corner |
| High-quality hi-res sprite edges (2 passes) | Not recommended. OFF is the default fast mode. ON smooths edges at additional cost |
| Enhanced shadows | Adds received shadows and approximate self-shadowing to units, vegetation and other sprites |
| Maximum shadow dimming | Controls shadow darkness; 0% disables dimming |
| Shadow samples / Shadow LOD start distance | More samples smooth shadows; distance LOD simplifies distant shadows |
| Enhanced lighting | Lights sprites from fire, explosions and Fetch lightning |
| Lighting influence | Controls the strength of local illumination |
| Maximum lights per view | Limits the light list after frustum culling; 4–128. Around 32 may be sufficient |
| Maximum lights per object | Limits lights affecting each sprite; 1–128, default 8 |
| Lighting LOD start distance | Default 10. With a limit of 8, distant sprites use 8→4→2→1 lights. Set 0 to disable LOD |

Off-screen light centers remain eligible when their illumination reaches the view. Both eyes and water reflections are considered. Each sprite uses only lights that reach it, up to its limit. Light-count LOD switches in discrete steps.

You can also create and add your own normal maps using the [sprite material workflow](MATERIALS.md). Terrain and mesh buildings currently do not receive enhanced lighting or enhanced shadows.

## If performance is low

Image quality and FPS are a trade-off: lower the resolution, disable enhanced effects, reduce rendering detail or lower reflection quality to improve performance.
With Bloom, enhanced shadows, enhanced lighting and other enhanced effects disabled, we expect most scenes to reach 90 FPS even at the highest resolution.

Keep diagnostic fixed-pose mode and detailed drawing-API timing disabled during ordinary play. Performance varies with the level, viewpoint and settings.

## Atlas batching, terrain and water

Enable **Sprite atlas batching (comparison)** under Textures & shadows to improve FPS. Leaving it ON is generally recommended. It works across levels; no collection export or preprocessing is required. Atlas pages normally use 2048×2048 pixels. Color and normal maps remain separate textures; normal images are shared. The 768 MiB budget includes both. Images that cannot be packed use the existing rendering path.

`atlas-high-water.tsv` in the app's external files folder records per-level peak pages, bytes and fallbacks. This file is for debugging and tuning; you can ignore it during ordinary play.

**Simplify distant terrain detail** is optional (default OFF): fades detail between distances 20 and 40 and skips it beyond 40, keeping terrain color and shadows. Terrain back-face culling is automatic. Water bounds use conservative tile bounds. Hidden-water reflections are skipped automatically using completed visibility queries; no synchronous GPU wait is introduced. Reflections can reappear a frame or more after water becomes visible.

## Pointer color and thickness

In **Minimap & menus**, these settings are independent and apply on the next VR entry:

- **Auto Switch UI/Gizmo Color** (default OFF): samples terrain at level load and replaces yellow control highlights on bright/snow-like or yellow terrain with the chosen accent. Default RGB is **245, 182, 66**; use the color picker to change it. Enemy/status colors are retained.
- **Thick Pointer UI**: **1.0–4.0x**, in 0.1 increments; default **1.0x**. Changes pointer rays, ground pointer/move rings, facing arrows and hovered-target rectangle borders. It does not enlarge hit areas or change selected-unit borders, health bars or box-selection lines. The slider value follows its handle.

## Color correction and Bloom (default OFF)

Color correction and Bloom are independent and both default to OFF. Each bypasses its processing when disabled. Choose your own balance of image quality and performance.

| Setting | Range and default |
| --- | --- |
| Color correction strength | 0–100%, default 100%. At 100%, brightens display-encoded scene color by about 1.28x, excluding sky and UI |
| Bloom processing resolution | 1/32, 1/16, 1/8, 1/6, 1/4 or 1/2 per dimension; default 1/8 |
| Bloom blur iterations | 1–4, default 1 |
| Bloom strength | 0–200%, default 15% |

Bloom extracts and blurs bright highlights, excluding sky and UI as glow sources. It does not identify fire or magic by object type. Above zero, increasing strength does not add rendering passes. Resolution and iteration count affect load; low-resolution Bloom still has full-screen capture/composition costs. Lower resolutions may give a coarser, wider glow. Bloom is a relatively expensive effect, so OFF is recommended if FPS is your priority, though a scene-rendering resolution of 1680 × 1760 may still give reasonable performance.

A light scene reached 90 FPS with the lowest scene-rendering resolution and 1/8 Bloom. This is not a guarantee across levels or viewpoints. Adjust the settings to suit your preferred balance of image quality, effects and FPS.

## Reflection sprite LOD (Preview 3)

The launcher water settings include **Reflection sprite LOD** (default ON), **start distance** (0–100 m, default 50 m) and **size threshold** (0–32 px, steps of 0.5 px, default 2 px). Changes apply on the next VR entry. OFF or a zero size threshold disables this rejection.

Beyond the start distance, sprites whose projected size is below the threshold are omitted from water reflections. Size is the longer dimension of the projected bounds overlapping the water draw rectangle, in reflection-texture pixels; the larger result from either eye is used. Hidden sprites reappear at 1.25 times the threshold to reduce flicker. Normal-view sprites, meshes, particles and shadows are unaffected. This is separate from the existing PBR shader distance LOD.

32 px is an aggressive option and can visibly remove reflections. Start with the default and compare the same scene. Reducing sprite counts does not remove terrain, mesh, effect or pass setup costs, and does not guarantee a proportional FPS improvement.


## Preview 4 — unified shadows and noShadow (2026-11-28)

Adds unified terrain/building/sprite shadows, cached static shadows and dynamic map resolutions through 8K; terrain base-color export/loading, interpolated terrain normals and shared mesh shader-state setup; independent shadowed/unshadowed projects, noShadow albedo priority over unedited HD textures, per-image normal inversion and the exit-recovery fix. Publishes a separate noShadow pack (10 collections / 97 images), updated APK and Manager/source ZIP. Existing normal/PBR packs, runtime and skyboxes remain available. Current performance is approximately 50–60 FPS as a guide due to rendering load and incomplete optimization, not a guaranteed rate. [noShadow setup and validation limits](NOSHADOW.md).
