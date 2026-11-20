# Terrain options and sprite CPU reuse investigation — 2026-11-19

## Implemented

Startup → テクスチャ・影 → **遠方の地面の詳細テクスチャを簡略化** / Simplify distant terrain detail. Default OFF, applies on the next VR entry. ON retains full detail through distance 20, smoothly fades it through 20–40, then skips all three detail-array samples beyond 40. Distances use VR world units and the shared eye-center position, mirrored for reflection passes. Original terrain color, stains, shade and lighting in texture unit 0 remain. Explicit gradients retain valid mip selection inside the distance branch. Without a loaded detail pack, this option has no effect.

Terrain backface culling was disabled. It is now enabled only around terrain draws, with CCW front winding for normal views and CW for reflected views. The heightfield's X-reversed coordinates produce upward-facing triangles; the reflection matrix reverses Y. Culling is disabled again before models, sprites and other two-sided artwork. Shadow-map caster behavior is unchanged. This rejects back-facing triangles, not arbitrary terrain hidden behind other terrain; it is not general occlusion culling.

Also corrected the sprite upload byte counter: it previously omitted the atlas attributes. With enhanced shading and atlas enabled, the stream uploads 88 bytes per vertex (20 geometry/UV + 4 owner + 16 light bounds + 48 atlas rectangles), not the 40 bytes previously reported. Timing measurements already included the work; this correction changes only the byte accounting.

## Sprite investigation — optimization candidates, not yet implemented

`prepareSprites` already caches prepared geometry across the two eyes each frame. Actual logs include normal `builds=120 reuse=120` in a 120-frame interval. Claiming that both eyes repeat all geometry preparation would be incorrect.

`drawPreparedSprites` has a narrower reuse rule for batching/uploads: the per-eye visible index vector must exactly equal the previous eye's vector. Logs show `uploads=240` in 120 frames without reflection, and up to 480 with reflection. `Sprite batch CPU ... builds=240,240` confirms the separate normal/reflection per-eye rebuilds. This is a concrete remaining duplication.

Candidate approaches:

1. Compute the union of both eyes' visible sprites once per frame and share the packed buffers. Each eye's clipper rejects the extra edge geometry. Test the CPU saving against the increased submission/GPU work, and retain eye-specific reflection bounds. Transparent sorting cannot be changed indiscriminately.
2. Replace `PreparedSprite.vertices → batches[key] vector → SpriteStream.packed` with sorting lightweight references followed by one final copy. Keep owners, light bounds, material mappings and opaque draw grouping intact.
3. Retain the upload's local `ordered` vector and sorting workspace across calls. Existing `packed`, owner and atlas arrays already retain capacity after clear; avoid attributing allocations to all of them. The local vector plus `stable_sort` temporary storage are specific allocation candidates.
4. Resolve atlas/material information once for repeated source textures, subject to palette dirty state and material binding revisions. Cache light bounds and owner metadata alongside prepared geometry rather than reconstructing bounds from six copied vertices after each upload.
5. Share common source selection/HD lookup work between normal and reflection preparations. The ordinary upright billboard orientation uses center X/Z, which reflection preserves. Axial/projectile billboard orientation can depend on center Y; reflection filtering, water-plane clipping and overlays differ. Sharing the entire prepared result unconditionally would be incorrect.

No percentage saving is established by this investigation. Last observed CPU phases included about 1.8 ms/frame for normal preparation and 1.2–2.3 ms/frame for all stream uploads; these are upper bounds on their respective work, not guaranteed removable time.

## Validation

`tools/test_terrain_options_gpu.cpp` on Quest: near/OFF pixel equality; three-material blend; gradual transition; far/base equality; disabled detail; upward/downward terrain visibility; reflected winding; restored two-sided state. Passed.

`tools/test_sprite_atlas_gpu.cpp` on Quest with the updated shared shader: actual sprite/particle streams, four shadow/light variants, OFF atlas elimination, HD edges, normals/crops/flips, alpha order, reflection and atlas lifecycle. Passed (maximum pixel difference 1/255).

Java/D8 and ARM64 renderer builds passed. Full gameplay visual/performance comparison of the new terrain settings remains pending.
