# Create sprite materials and normal maps

**English** | [日本語](MATERIALS.ja.md) · [Home](../README.md)

Normal maps are optional. Use your own converted game/HD data; game sprites are not included in the download.

The collection browser can track two export folders: **Normal export folder** and **PBR export folder**. Each thumbnail shows separate Normal/PBR marks: ✓ means a same-basename `.mvrnm` exists directly in that folder, — means missing, and ? means the folder is unset or unavailable. Folder choices persist. Reopening the browser or clicking **Refresh** updates the marks. These indicate file existence, not editing completeness or freshness; renamed exports will not match.

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

## Hash-only packs (0.3.2-beta and later)

The 0.3.2–0.5.0-beta normal-only exporter writes MVRNORM2: normal pixels and the source image SHA-256. Recipients simply transfer the folder of `.mvrnm` files with Manager; adding or restoring original images is unnecessary. The published 0.3.1-beta APK cannot read this new format.

Quest matches dimensions and the hash of the complete RGBA image. Different images with the same name do not match, while identical images can share normals across levels. HD replacements with different pixels need matching packs. Legacy packs remain supported.

To convert existing packs, run this from the updated source folder. All `.mvrnm` files directly in the input folder are converted without changing normal pixels or strength. Transfer the output folder with Manager.

```powershell
python tools/normal_library.py convert "old pack folder" "new pack folder"
```

## Material color references and direct connection

Use **Material colors** to register five material names with three representative colors each. Color buttons open the picker; **Pick** samples the selected source image, and × clears a slot. Transparent pixels are ignored; Escape cancels sampling. References are saved in .smproj projects and image-export settings. They do not change existing height masks or Quest normal output. These are classification hints, not automatic segmentation or PBR generation yet.

Click **Direct connection** and choose a connection JSON destination to enable an authenticated localhost API. Click again or close Editor to stop it. The JSON contains a token; do not publish it. Give its location to your chat assistant so the client can retrieve project state, references, frame identities and source images.

The collection catalog covers folders opened in Editor and the last saved collection folder. Initially select the folder through **Collection folder**. A request such as “load Bowman” can be resolved against catalog names and submitted by ID. The API does not guess names; the caller must resolve ambiguous candidates. Unsaved changes return 409 and busy work returns 503 without discarding edits. Opening returns 202; poll state to confirm completion.

An already running older Editor does not update automatically. Save your work, close it and restart using the updated code.

Developer client: tools/sprite_editor_client.py --connection connection.json state (or collections). open requires --collection-id and the current --session; source requires --session, --index and a new --out path. The session is project_session from /state. The API also supports guarded live material edits, preview, undo, project save/load, and material-layer PNG reads. See [Direct editing API](SPRITE_EDITOR_API.md).

## Material views and session restoration

Dedicated Metallic & Smoothness and Thickness modes show grayscale values (black=0, white=1) with source alpha preserved. New layers default to Metallic & Smoothness=0 and Thickness=1. Existing saved layer values are preserved. These modes are currently view-only; mask brushes do not edit them. Project format v3 preserves the layers; Export all includes metallic_smoothness and thickness grayscale PNGs. Quest export packs both channels together with normal XY. Existing v1/v2 projects remain readable.

Normal exit saves recovery data to %APPDATA%/MythIIVR/sprite-editor-last-session.smproj. On the next launch, all collection images, edited layers, material references, selected image, display mode and dirty state are restored. Explicit --project/--collection startup takes precedence. User-saved project files are not overwritten. This is normal-exit recovery, not continuous crash backup. Direct connection must be enabled again manually.

Live material edits work on the open project, including unsaved work, and refresh the material view immediately. No intermediate save or reload is needed. Representative colors are references for visual part identification, including neighboring shades, shape, position and animation context; they are not exact-match selection rules. The caller supplies semantic masks. Saving and switching projects can also be requested through the connection; unsaved work must be saved before a switch.

## Packed materials (development build, 2026-11-21)

Use the updated APK, Editor and Manager together. **Export for Quest** now writes `MVRMAT04` inside `.mvrnm`, with a PBR flag per image. ON stores R/G = normal X/Y, B = Thickness, A = shared Metallic & Smoothness; OFF stores ordinary normal RGBA. The original color texture still supplies transparency. Normal Z is reconstructed on the positive hemisphere. This keeps one material texture lookup and four bytes per pixel. `.smproj` keeps the editable layers unchanged; old runtime packs remain readable as normal-only materials. Published 0.5.0-beta and earlier cannot read the new material formats.

On Quest, **Sprite materials** contains maximum reflection blend (0–1, default 0.5), specular strength (0–16, default 8), and SSS strength (0–4, default 2). Values apply on the next VR entry. Set all three to zero to bypass the PBR shader path. Enhanced lighting/shadows remain independently selectable. A matching material pack is required. Sunlight and sky reflections require a skybox; local-light specular and SSS do not.

Metallic & Smoothness controls both metallic color response and highlight/reflection sharpness. Thickness=1 blocks transmission; lower values increase SSS, suppressed on metal. Enhanced shadows attenuate sunlight specular and SSS using received-shadow visibility. The skybox's local 3×3 neighborhood in the shadow-light direction supplies linear RGB sunlight (color and intensity). Missing skyboxes disable sunlight and environment additions rather than inventing a light color. This is an artistic approximation, not physical PBR or HDR illumination.

V4 packs take priority over V3 and legacy normals (including explicit OFF); V3 takes priority over legacy normals for identical source hashes. Among duplicate packs of the same format, the lexicographically later filename wins. Avoid conflicting packs. The library converter and export/restore tools preserve the material format and channels; material libraries require the updated tools.

PBR-ineligible draw batches use a separate shader with the PBR function, call and dedicated uniforms removed. This covers legacy/missing material data, missing sky with no local specular or SSS, zero strengths, and channel summaries that cannot contribute (for example, SSS-only with all Thickness=1). Normal lighting and shadows remain available. Specular amplitude is multiplied by M&S: M&S=0 produces no specular; intermediate values also reduce amplitude. Direction-dependent zero highlights are still evaluated in the PBR shader.

Enhanced lighting point and lightning-segment lights also produce specular highlights on packed materials. This reuses per-sprite light masks, light limits/LOD, range and squared falloff. The specular slider controls both sun and local highlights. Sun and local highlights are added after the sky blend. Received sun shadows attenuate sunlight only, not unrelated local lights. No local shadow maps, extra texture lookups or render passes are added; active lights add per-pixel math.

### Per-image PBR

In **Source** view, the small checkbox at the top-right of each gallery image toggles PBR for that image. **Batch PBR** accepts zero-based indices/ranges such as `0-20,25`, or `all`; Yes enables and No disables. Gallery captions mark `[PBR]` and the control row shows the enabled count. New images and old projects without flags default to OFF. Layer edits do not implicitly enable PBR. OFF retains editable M&S and Thickness, but Quest uses the ordinary normal-only shader path, preserving normal lighting and shadows. Project v4 and Export all manifests retain the flag. Restart an already-running Editor after saving work; re-export and transfer materials with the updated Editor/Manager and APK. This update does not modify existing user projects or deployed material packs.

MVRMAT04 layout: 8-byte magic, uint32 frame count; each frame has uint32 width, height, flags (bit 0=PBR, other bits rejected), 32-byte original RGBA hash, then width×height×4 payload bytes. OFF payload is normal XYZ plus coverage alpha; ON payload is packed XY/Thickness/M&S. Flags survive library export, conversion and restore. No additional shader lookup is needed.

`DebugSpecularLightingMode` defaults to OFF and applies on next VR entry. ON displays only the colored specular contribution at its actual intensity on PBR sprites, with zero contribution shown as black. Sunlight, local lights, strength, shadow visibility, fog attenuation and sky-independent sunlight specular are preserved. Base color, diffuse light, environment reflection and SSS are excluded from this diagnostic view. Non-PBR sprites retain their normal appearance; alpha and reflection compositing remain intact. Debug mode deliberately uses the PBR shader even for zero-valued material so it can display black.

### Direct material painting

Open **Material paint** below the preview, then select **Thickness** or **Metallic & Smoothness** in the main window. Paint on the preview with Replace, Add, Sub, or Screen. Value is 0–1; the palette provides 0.1 steps. Radius and softness are adjustable; Eyedropper samples the selected material layer. Source alpha-zero pixels are protected in every material operation. Values are stored as 8-bit grayscale.

The tools window also copies the current generated Height map (including masks, blur and enabled shape correction), inverts values, or applies contrast around 0.5 (1 = unchanged, 0 = flat 0.5). Scope accepts `current`, `all`, or zero-based image ranges such as `0-20,25`. Apply edits the selected material layer. Undo restores the last material edit if no intervening edits occurred. PBR flags are enabled separately.

Material painting and bulk adjustments are recorded in `edit-history.json` inside saved projects, including brush paths, parameters, image indices and before/after hashes. History survives save/load and session recovery. Existing projects without history remain supported.

Material Undo also supports **Ctrl+Z** in the main and Material paint windows. **Brightness** adds an offset from -1 to +1 (0 = unchanged), clamps to 0–1, and previews live while dragging. Releasing the slider commits the selected image scope as one history/Undo operation. Each drag uses the committed image as its baseline; the slider returns to 0 after committing. Numeric input commits on Enter or focus loss. Brush radius uses integer pixels. A black/white brush circle follows the pointer over the material preview, scaled to its zoom. Eyedropper samples the active Thickness/M&S layer into the brush Value; select Paint again to draw.

**Histogram** below the preview opens a live histogram window for the selected Thickness, M&S, Height or Mask image. The horizontal axis is 0–1 (256 stored values); the vertical axis is pixel count, scaled automatically to the largest bin. Source alpha-zero pixels are excluded. Hover over a bar to see its value, count and percentage. Painting, Brightness preview, Undo and image switches update it immediately. The window also shows the visible pixel count and mean value.

**Fill**, next to Copy Height and Invert, replaces the selected Thickness/M&S layer with the current **Value** over the specified image scope. It ignores the brush blend mode, protects transparent source pixels, records history and supports Undo.

In Thickness/M&S mode, drag the black/white triangular handles below the histogram to adjust input levels: `output = clamp((input - black) / (white - black), 0, 1)`. The selected image and histogram preview live. Release to commit to the Material paint Scope as one Undo/history operation; handles reset to 0/1 for the next adjustment. Transparent pixels are protected. Escape (or Ctrl+Z during a drag) cancels the preview; Ctrl+Z after committing undoes it. Height/Mask histograms remain read-only.

Hover over a gallery image to see its zero-based list index and bitmap name at the upper right of the pointer. Hover over the main preview to see the source pixel coordinates and value: Thickness/M&S/Height/Mask show 0–1 and the stored 0–255 value, while Source/Normal show RGBA. Alpha is included, and material readouts follow live Brightness/Levels previews. Readouts disappear outside the image.

`DebugSSSLightingMode` (OFF by default) displays only the actual colored SSS contribution on PBR sprites; zero contribution is black. Thickness, M&S, sunlight, strength, received shadows, environment-blend attenuation and fog remain effective. Enhanced point and segment lights also contribute to SSS, using the existing per-sprite light mask, limits and squared range falloff. Local SSS works without a skybox, uses the SSS strength control, and is added after environment blending. Sun shadows do not attenuate these unrelated lights; local-light occlusion is not implemented. No extra texture reads or draw passes are added. This mode and specular debug are mutually exclusive and apply on next VR entry.

**Contrast** uses a live-preview slider (0–4, neutral 1). Dragging previews the selected material and histogram without accumulating changes. Releasing commits the selected scope as one history/Undo operation and resets the slider to 1. Numeric entry commits on Enter or focus loss. Transparent source pixels remain protected.

SSS transmission tint retains the source RGB proportions but divides linear albedo by max(0.25, its largest channel). This reduces baked-shadow attenuation, with at most 4× amplification and no added light for pure black. Sunlight and local SSS use the same tint; Thickness=1 still produces exactly zero transmission. This is an artistic approximation, not reconstructed physical leaf color.

SSS uses an angular lobe that peaks when view-to-surface aligns with surface-to-light and reaches its floor at 30 degrees. The launcher’s `Minimum Amount of SSS` slider controls that floor from 0.00 to 1.00 (default 0.35). Zero restores directional-only transmission; 0.35 restores the earlier 35% off-axis component. The factor is `minimum + (1 - minimum) * angularWeight`. This applies to sunlight, local lights, mesh/sprite materials and SSS debug, on next VR entry. It does not bypass thickness, backlighting, metallic attenuation, intensity or sunlight occlusion; Thickness=1 still transmits nothing.

### Model materials

When Use PBR-like Shader for Meshes is ON, model meshes opt into enhanced shadow receiving, local lighting and pseudo-PBR through the PBR checkbox on their source texture. Export for Quest and install the .mvrnm as usual. Unflagged meshes keep the lightweight shader; all meshes retain shadow casting. A texture containing both wood and cloth shares the enhanced path: use Thickness=1/M&S=0 on opaque wood. Mesh normals follow the rotating geometry with UV-based normal mapping and two-sided shading. Model pack versions MYTHMD03/MYTHAM02 preserve original image hashes before scene tint; re-convert older level packs for reliable matching. Legacy packs remain readable. Local lights still have no local shadow maps.

Sunlight occlusion is independent of the artistic enhanced-shadow darkness. Fully blocked sun produces zero direct SSS and specular even at 45% shadow darkness; PCF visibility controls partial coverage. Local lights and environment reflection remain independent. Light selection is shared across mesh and sprite shader variants for the same view/snapshot, avoiding repeated ranking; this adds no rendering pass.

The specular and SSS debug toggles are under **Setup & diagnostics**, both OFF by default. Existing saved material settings are preserved when updating the app.

**Use PBR-like Shader for Meshes** is in Sprite materials and defaults to OFF. OFF routes all static and animated model meshes, including water reflections, through the standard lightweight shader even when their textures have PBR checked. Mesh enhanced shadow receiving, local lighting and pseudo-PBR are bypassed, and the mesh PBR shader is not compiled. Shadow casting and sprite PBR remain unchanged. ON enables the enhanced path only for PBR-flagged mesh textures. The setting is saved and applies on next VR entry.

**Use PBR-like Shader for Units** and **Use PBR-like Shader for Scene objects** both default to OFF under Sprite materials. Classification uses the live unit ID and original object type, not collection names or texture colors: actor/unit sprites and corpses use Units; other eligible physical sprites (including trees, scenery and physical projectiles) use Scene objects. Linked visual overlays inherit the body’s category. Unowned translucent effects retain their existing non-PBR path. OFF selects the normal sprite shader while preserving normal maps and enhanced lighting/shadows. Meshes remain controlled independently by the mesh toggle. Settings apply on next VR entry, including water reflections; atlas batches with different permissions remain separate. Both sprite toggles OFF also skips compiling the sprite PBR variant. Debug modes do not override these switches.

In the right preview, **Shift + click** connects the last painted point to the clicked point using the current brush. Hold Shift to see a cyan dashed line guide before painting. This works for Thickness, Metallic & Smoothness and mask painting. Switching images or painting modes resets the starting point. Material lines preserve transparent-source masking and can be undone with Ctrl+Z.


### Distance LOD and compatibility

Enable pseudo-PBR separately for units, scene objects and meshes. LOD start ranges from 20 to 1000 m and smoothing from 4 to 100 m, defaulting to 20 and 4 m. Mesh exclusion defaults off. Distant eligible meshes retain simplified self-shadowing with enhanced shadows enabled. Export for Quest writes MVRMAT04 even with every PBR checkbox cleared. The main 0.5.0-beta APK cannot read this format. The PBR APK also reads legacy normals.
