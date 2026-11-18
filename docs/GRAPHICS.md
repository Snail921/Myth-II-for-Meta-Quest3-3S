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
