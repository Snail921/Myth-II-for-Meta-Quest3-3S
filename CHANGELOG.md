# Changelog

**English** | [日本語](CHANGELOG.ja.md)

## 0.3.1-beta — 2026-11-19

- Added vertex shading for distant, small sprites, with LOD diagnostic counts.
- Aligned enhanced shadows with scenery shadows and added ground-overlay and sun-angle diagnostics.
- Preserved enhanced shading on airborne death fragments and semi-transparent death animations.
- Added world-knot teleport particles, sprite fading and static model appearance changes.
- Return to the original results screen when a film finishes; disable in-game panel actions during that transition.
- Fixed launcher packaging and a VR black screen caused by mismatched model-sync packet framing.

For world-knot effects, use the updated APK and conversion tools, then reconvert and transfer existing level packs. Fresh conversions include the required data automatically; no extra setup steps are needed. Install the APK over the existing app; do not uninstall or repeat Initial setup. The initial runtime is unchanged.

## 0.3.0-beta — 2026-11-18

- Added enhanced shadows, approximate self-shadowing, shadow strength, sampling and distance LOD.
- Added colored local lighting from fire, explosions and Fetch lightning, with frustum culling, per-object light limits and distance LOD.
- Added Sprite Material Editor, normal-pack export/transfer and on-demand normal loading.
- Kept enhanced shading on death and paralysis sprites.
- Added minimap FPS display (0.1-second updates) and slider values that follow their handles.
- Made opaque one-pass sprites the default; retained two-pass edges as an optional high-quality mode.
- Reduced rendering cost through lightweight terrain/mesh shaders, frustum culling and material-state reuse.
- Improved VR transition/resume event handling and added fixed-pose, stage-timing and light-count diagnostics.

Install the APK over the existing app; do not uninstall or repeat Initial setup. Runtime and skyboxes are unchanged. Update the Manager ZIP to use the new editor. Texture atlases are not implemented.

## 0.2.0-beta — 2026-11-17

- Added resolution, 72–120 Hz and MSAA settings; grouped launcher settings into collapsible sections.
- Added co-op unit transfer/return with right grip + Y and a teammate picker.
- Improved hold-to-turn for stationary and newly selected units, preserving their positions.
- Group cycling now recalls the last group after deselection or manual selection.
- Game speed uses right grip + left/right stick click; the minimap briefly shows the current speed.
- Corrected mirrored sprite directions while retaining left/right-handed variants, including Soulless.
- Suppressed water highlights in unit/model shadows and added cached terrain/static-model shadow maps.
- Restored the CD image during Starting Up.
- Added a separate bilingual controls guide. APK version is now independent of the compatibility runtime's version.

Existing users: install the new APK over the current app. Keep your data and settings; **do not uninstall or repeat Initial setup**. Runtime and skybox downloads are unchanged.

## 0.1.0-beta — 2026-11-17

Initial public release: APK, initial runtime, Plugin Manager, source, skyboxes and bilingual setup guides. Original game data is supplied and converted by the user.
