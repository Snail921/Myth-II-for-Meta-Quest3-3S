# Terrain shader optimization investigation — 2026-11-20

Scope: investigation only; no renderer edits or APK update.

## Evidence

The renderer uses spriteShaderSource(false,false,false) for terrain, so atlas, enhanced shadow and light paths already have compile-time disabling. Do not attribute all the shared source code to actual runtime cost. color_pipeline.h still has a detail loop over three interpolated material weights, with textureGrad for each positive material ID. detail_terrain.h supplies three IDs and barycentric weights, including duplicate IDs.

Audited local converted Beyond the Cloudspine detail pack build/detail-test/30377274.pack (224x224, 12 materials, 1024px detail textures), matching map ID in build/vr/assets/levels/catalog.tsv. Applied the same alternating diagonal and triangle override indexing as buildDetailTerrain. Results in terrain-detail-sample-audit.json:

- 99,458 triangles total.
- 77,737 have one distinct positive material (78.16%).
- 21,484 have two (21.60%).
- 237 have three (0.24%).
- Source-level detail sampling opportunities: 298,374 -> 121,416 across triangle counts, 59.3% fewer; average 3 -> 1.221 detail samples.

This is a triangle-count-weighted opportunity, not screen-pixel-weighted, not a GPU benchmark, and not a predicted 59.3% FPS gain. Driver optimization and texture caching may already mitigate repeated fetches. Current device pack was not verified against this local pack. Newly captured terrain-shader-review-log.txt contains no MythVR samples; other VrApi processes must not be treated as game performance. Prior saved game logs remain the performance baseline.

## Preferred implementation candidate

Coalesce equal material IDs in CPU-generated detail vertex data. For every triangle, assign each unique ID a slot; each vertex contributes its existing barycentric weight to the matching slot. Mark unused slots ID 0 with weight 0. This preserves interpolated material mixing mathematically, avoids per-pixel duplicate-ID comparisons, and uses the existing id>0 branch to skip unused samples. Do not threshold small weights (would change transitions). Preserve textureGrad derivatives and scales. Floating-point summation changes can create tiny rounding differences, so validate with image comparisons, allowing only a specified small tolerance.

## Secondary candidates

A dedicated opaque terrain shader could remove generic alpha/softOnly/cutout/tint/particle-alpha handling, and guarantee unnecessary lighting-related work is absent. Priority lower: existing compile-time specialization already removes much of this, and drivers may fold uniform branches. Before removing discard, inspect both initial terrain texture alpha and every live surface update path. Preserve original stains/shading, fog, sRGB decode, reflection clip and premultiplied reflection output. Fog/LOD distance reuse can also be inspected, but compiler common-subexpression elimination may already handle it.

Changing detail fade distances, texture resolution, color conversion approximation or geometric detail changes appearance and is outside the first optimization step.

## Validation plan

1. CPU tests for [A,A,A], [A,B,A], [A,A,B], [A,B,C], zero IDs and override triangles: compare old/new weighted factors over barycentric samples.
2. Quest GPU comparisons at texture/material boundaries and several distances/mip levels, detail LOD on/off, normal/reflection passes, fog and live stains. Check stereo edges.
3. Fixed view with identical refresh rate, resolution and other settings. Measure terrain GPU time using the existing timer; compare both visually uniform terrain and mixed-material boundary scenes. Report changes separately from total FPS.

## 日本語要約

最優先候補は、同じ詳細材質IDの重複参照を、地形データ作成時にまとめることです。手元のBeyond the Cloudspineデータでは約78%の三角形が単一材質で、現行の3参照を1参照に集約できる構成です。三角形数ベースの参照削減余地は約59%ですが、画面内の面積・GPUキャッシュ・ドライバー最適化で実際の効果は変わります。画質維持の数学的根拠はありますが、浮動小数点の丸めとミップ境界の画像比較が必要です。次点は地形専用シェーダー化。ただし影・照明などは既にコンパイル時に無効化されており、大きな効果を事前には保証できません。今回は調査のみです。


## Implementation follow-up

Implemented detail_materials::compact in terrain vertex preparation. Each triangle retains its original material order and interpolated contribution, while repeated positive IDs share one slot; zero IDs remain neutral and are not renormalized. Existing shader, texture gradients/scales, triangle overrides, fog and color conversion are unchanged. Added Terrain detail compact startup log with triangles and before/after sample opportunity counts (not GPU-measured fetch counts).

Quest validation: 14,784 CPU barycentric comparisons; 768 GPU old/new image comparisons across all IDs 0–3 permutations, three UV/mip scales, normal/reflection output, fog and detail-distance fade. Maximum observed byte difference was 0. Test uses the real scene shaders and shared compaction helper. Android build passed. Actual game FPS gain is unmeasured and must be compared at the same viewpoint/settings.

実装済み。地形データ作成時に同じ材質IDの重みを集約し、材質0への境界を再正規化しないため、元の混合を維持します。QuestでCPU 14,784ケースとGPU画像比較768ケースが通過し、観測した画素差は0でした。実ゲームでのFPS改善幅は未測定です。
