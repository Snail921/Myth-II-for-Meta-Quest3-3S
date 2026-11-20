# Graphics settings and FPS

**English** | [日本語](GRAPHICS.ja.md) · [Home](../README.md)

On Quest, open App Library → Unknown Sources → **Myth II for Meta Quest 3 / 3S**, then expand the graphics settings. Changes apply on the next VR entry.

| Setting | What it does |
| --- | --- |
| Resolution, refresh rate, MSAA | Balance image quality and load. The requested refresh rate is a target, not a guaranteed FPS |
| FPS display | Shows one decimal place above the minimap's upper-left corner; updates every 0.1 seconds |
| High-quality hi-res sprite edges (2 passes) | OFF is the default fast mode. ON smooths edges at additional cost |
| Enhanced shadows | Adds received shadows and approximate self-shadowing to units, vegetation and other sprites |
| Maximum shadow dimming | Controls shadow darkness; 0% disables dimming |
| Shadow samples / Shadow LOD start distance | More samples smooth shadows; distance LOD simplifies distant shadows |
| Enhanced lighting | Lights sprites from fire, explosions and Fetch lightning |
| Lighting influence | Controls the strength of local illumination |
| Maximum lights per view | Limits the light list after frustum culling; 4–128 |
| Maximum lights per object | Limits lights affecting each sprite; 1–128, default 8 |
| Lighting LOD start distance | Default 10. With a limit of 8, distant sprites use 8→4→2→1 lights. Set 0 to disable LOD |

Off-screen light centers remain eligible when their illumination reaches the view. Both eyes and water reflections are considered. Each sprite uses only lights that reach it, up to its limit. Light-count LOD switches in discrete steps.

You can add normal maps using the [sprite material workflow](MATERIALS.md). Terrain and mesh buildings currently do not receive enhanced lighting or enhanced shadows. Water highlight suppression uses a separate shadow path.

## If performance is low

1. Enable FPS display.
2. Disable high-quality sprite edges.
3. Disable enhanced shadows and enhanced lighting individually to identify their impact.
4. If needed, reduce shadow samples, light counts, resolution, MSAA or reflection quality.

Keep diagnostic fixed-pose mode and detailed drawing-API timing disabled during ordinary play. Performance varies with the level, viewpoint and settings.

## Inspect light counts

Enable enhanced lighting and diagnostic logging, then trigger one explosion. `Lighting population` reports candidates, peak candidates, births/removals and selected lights for normal/reflected views. Selected counts describe the reporting instant and can be zero after a brief explosion has ended. Birth/removal counts compare captured game snapshots.

## Distant sprites and shadow diagnostics

Distant sprites that appear small use vertex shading without normal maps. Large sprites retain detailed shading even at a distance. Diagnostic logs include the number using each path.

Enable the ground shadow-color diagnostic to compare enhanced shadows with scenery shadows. While it is enabled, right stick click toggles sun-angle adjustment; move the right stick to adjust the angle. Values are written to the diagnostic log. Turn this mode off for normal play.

## Atlas batching, terrain and water

Enable **Sprite atlas batching (comparison)** under Textures & shadows to compare with the original rendering path. It works across levels; no collection export or preprocessing is required. Atlas pages normally use 2048×2048 pixels. Color and normal maps remain separate textures; normal images are shared. The 768 MiB budget includes both. Images that cannot be packed use the existing rendering path.

`atlas-high-water.tsv` in the app's external files folder records per-level peak pages, bytes and fallbacks. It updates when maxima rise, including periodic/level-exit checks; it is not a continuous frame log.

**Simplify distant terrain detail** is optional (default OFF): fades detail between distances 20 and 40 and skips it beyond 40, keeping terrain color and shadows. Terrain back-face culling is automatic. Water bounds use conservative tile bounds. Hidden-water reflections are skipped automatically using completed visibility queries; no synchronous GPU wait is introduced. Reflections can reappear a frame or more after water becomes visible.

## Pointer color and thickness

In **Minimap & menus**, these settings are independent and apply on the next VR entry:

- **Auto Switch UI/Gizmo Color** (default OFF): samples terrain at level load and replaces yellow control highlights on bright/snow-like or yellow terrain with the chosen accent. Default RGB is **245, 182, 66**; use the color picker to change it. Enemy/status colors are retained.
- **Thick Pointer UI**: **1.0–4.0x**, in 0.1 increments; default **1.0x**. Changes pointer rays, ground pointer/move rings, facing arrows and hovered-target rectangle borders. It does not enlarge hit areas or change selected-unit borders, health bars or box-selection lines. The slider value follows its handle.

Snow faces the camera; rain retains its streak orientation. Weather textures use their own UVs rather than sampling an entire atlas page.

## Color correction and Bloom (default OFF)

Color correction and Bloom are independent and both default to OFF. Each bypasses its processing when disabled. Choose your own balance of image quality and performance.

| Setting | Range and default |
| --- | --- |
| Color correction strength | 0–100%, default 100%. At 100%, brightens display-encoded scene color by about 1.28x, excluding sky and UI |
| Bloom processing resolution | 1/32, 1/16, 1/8, 1/6, 1/4 or 1/2 per dimension; default 1/8 |
| Bloom blur iterations | 1–4, default 1 |
| Bloom strength | 0–200%, default 15% |

Bloom extracts and blurs bright highlights, excluding sky and UI as glow sources. It does not identify fire or magic by object type. Increasing strength does not add rendering passes. Resolution and iteration count affect load; low-resolution Bloom still has full-screen capture/composition costs. Lower resolutions may give a coarser, wider glow.

A user confirmed 90 FPS in a light scene with the lowest scene-rendering resolution and 1/8 Bloom. This is not a guarantee across levels or viewpoints. Use the FPS display, especially near waterfalls. Changes apply on next VR entry.

0.5.0 also batches particles while retaining per-particle opacity, shares reflection sprite preparation, culls off-screen terrain and consolidates repeated terrain-detail material samples.
