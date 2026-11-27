# Changelog

## Preview 4 — unified shadows and noShadow (2026-11-28)

Adds unified terrain/building/sprite shadows, cached static shadows and dynamic map resolutions through 8K; terrain base-color export/loading, interpolated terrain normals and shared mesh shader-state setup; independent shadowed/unshadowed projects, noShadow albedo priority over unedited HD textures, per-image normal inversion and the exit-recovery fix. Publishes a separate noShadow pack (10 collections / 97 images), updated APK and Manager/source ZIP. Existing normal/PBR packs, runtime and skyboxes remain available. Current performance is approximately 50–60 FPS as a guide due to rendering load and incomplete optimization, not a guaranteed rate. [noShadow setup and validation limits](docs/NOSHADOW.md).


## 0.5.0-pbr-like.3 — 2026-11-24

- Share normal/reflection sprite preparation, add conservative early culling and shared reflection geometry selection.
- Fix the film-playback crash at camera-plane/union boundaries.
- Add reflection sprite size LOD (default 50 m / 2 px), optimize its CPU checks and extend the size slider to 32 px.
- Record viewpoint/controllers and restore them for fixed-pose diagnostics, with initial-pose fallback and logging when no valid recording exists.
- Update APK, Manager/source ZIP and English/Japanese guides. Reuse unchanged Preview 2 materials and shared runtime/skyboxes. Main is unchanged.


**English** | [日本語](CHANGELOG.ja.md)

## 0.5.0-beta — 2026-11-20

- Multiplayer Yell/Whisper, two-hand keyboard, fixed/follow placement, relative sliders and blinking caret.
- Stable original-menu/lobby keyboard opening with a one-second B/Y hold. Enter closes; Close sends no key.
- Minimap pen/eraser, team sharing, continuous simplified strokes and rotation lock while drawing. Map rotation defaults OFF.
- Independent trigger, grip, A/X, Y/B and stick swaps, plus pointer/UI-hand swap. All default OFF.
- Tabbed Manager and resizable log, combined base single-player/multiplayer setup. English defaults for app and PC tools.
- Optimized particle batching, shared reflection preparation, terrain visibility culling and repeated detail-material samples.
- Color correction and Bloom, both default OFF. Bloom supports 1/32–1/2, 1–4 iterations and 0–200% strength.
- Updated English/Japanese controls, keyboard, graphics and setup guides.

## 0.4.0-beta — 2026-11-19

- Added optional sprite atlas batching for all levels, with shared normal maps, stereo upload reuse, 2048-pixel pages and a 768 MiB budget with fallback. Record per-level high-water usage for playthrough diagnosis.
- Added optional distant terrain-detail simplification and terrain back-face culling.
- Replaced per-triangle water screen-bound calculations with conservative bounds; skip reflections for water hidden behind terrain/buildings using asynchronous stereo visibility queries.
- Fixed weather atlas sampling and made snow face the camera; retained rain streak orientation.
- Added Auto Switch UI/Gizmo Color (default OFF), configurable accent color (default RGB 245, 182, 66), and independent Thick Pointer UI (1.0–4.0x, default 1.0x).
- Added right grip + B to deselect without cycling groups.
- Fixed original-game launch packaging and clarified the Start Myth II button.

Update the APK and Manager/source ZIP. Runtime, skyboxes and 0.3.2 normal packs are unchanged.

## 0.3.2-beta — 2026-11-19

- Store source-image SHA-256 and normal pixels in MVRNORM2 packs; original image pixels are no longer included. Legacy MVRNORM1 packs remain readable.
- Updated editor export and Manager validation/transfer. Existing packs can be converted without changing the normals; the normal-only library can be unpacked without selecting original images.
- Index hashes once per level and load only matching normals. Added separate hash, lookup, file-read and upload-submission timings.
- Device testing confirmed correct shading and a shorter VR transition. Converting 206 packs preserved all 41,353 normal images and reduced total size from about 1.14 GB to 574 MB.

Update both the APK and Manager ZIP. The normal-map ZIP now contains ready-to-transfer packs. Initial runtime is unchanged.

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
