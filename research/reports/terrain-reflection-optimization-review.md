# Terrain and reflection optimization review

Evidence: build/waterfall-batching-after.txt, 2026-11-20 16:12:53–16:13:33 device log. Read-only investigation; no runtime changes or APK deployment.

## Reflection sprites

19 sampled reporting windows: prepare CPU mean 2.269 ms/frame (1.396–2.644), upload 0.442 (0.329–0.720), draw submission 0.366 (0.248–0.422). Last window reports 120 normal builds, 240 reflected builds, 240 reflected reuses, and 480 reflected calls over 120 frames. Thus two water-plane preparations and four eye/plane render passes per frame, not a complete lack of stereo caching. Last visibility count: 60,308 accepted / 326,518 prepared candidates, about 18.5%.

src/vr/sprite_cache.h:19–65 scans objects, resolves frames/directions and HD textures, builds geometry/bounds per water-plane cache. Visibility rejection is later, in drawPreparedSprites. Most billboard geometry uses only center X/Z, which remain unchanged by horizontal water reflection. Good candidate: prepare shared per-object frame/texture/ordinary-billboard geometry once per frame; retain plane-specific height clipping and visibility. Axial projectiles use center Y and must remain plane-specific; hdSprite mutates Quad, so cached geometry must use its resulting dimensions. Selection overlays remain normal-view-only. Do not blindly reuse the normal *visible* set: reflections can contain objects outside the direct view.

Another candidate is conservative early visibility bounds before expensive HD preparation. Bounds must include animation/view-dependent dimensions, flips, HD scale and axial projectiles; overly small bounds cause disappearing objects.

Uploads can share a stereo-union visible set per plane, preserving both eyes, but this may increase GPU work. Lower priority because measured upload cost is smaller. Existing per-plane stereo reuse already skips upload when visible lists match.

## Terrain

20 sampled reporting windows: normal terrain GPU mean 2.933 ms per call, range 2.352–3.701. These are sampled per-eye/pass GPU timings, NOT CPU ms/frame; do not directly add to the CPU figures.

src/vr/main.cpp builds 16x16-cell bounds/ranges, but normal rendering submits terrainCount in one draw. Reflection rendering already rejects ranges against its clipped frustum. Candidate: conservative per-eye or stereo-union frustum culling for normal terrain, merging adjacent surviving ranges to contain draw-call overhead. Preserves geometry and textures. Benefit depends on vertex cost and how much terrain is off-screen: GPU already clips off-screen fragments, so this alone cannot promise a large pixel-cost reduction.

The shared fragment shader (color_pipeline.h) contains alpha discard and up to three detail texture samples per pixel. Candidate: coalesce identical detail material IDs/weights, skip zero-weight samples, or specialize terrain shaders once opaque terrain texture data is verified. Must preserve explicit gradients, mip selection, material blending, stains, shading and fog. Cannot simply remove discard without checking texture alpha.

Front-to-back terrain block order/depth prepass are experiments, not guaranteed wins on this device. Extra draws/vertex work can outweigh reduced overdraw. Existing backface culling, detail mipmaps and optional detail-distance fade should not be described as missing features. Distance fade / resolution reduction alter quality and are not the first proposed step.

## Priority / 日本語要約

1. 反射用スプライトの共通準備を水面間で共有。CPU準備平均約2.27ms/フレームが対象で、全部が削減できるわけではない。一般ビルボードは共有可能性が高いが、飛翔物などは個別処理が必要。
2. 通常地形に保守的な区画単位の視野判定を導入してA/B計測。見えている地形のポリゴン数・画質は維持。GPUのピクセル負荷には効かない可能性がある。
3. 地形シェーダーの重複詳細テクスチャ参照と不要分岐を調査。材質境界・濃淡・透明度の画像比較が必須。

同一視点・同一リフレッシュレート・描画設定で個別に検証する。CPUとGPUの削減量を足してFPS改善幅とみなさない。現時点では改善余地の調査結果であり、変更の実装・効果測定は未実施。


## Implementation follow-up (2026-11-20)

Implemented per-frame ordinary reflection billboard preparation shared across water planes. The shared source contains all eligible candidates, not just the first plane's visible set. Original pre-flip height clipping and small-object policy are applied separately for every plane. Axial projectiles remain plane-specific. Cache is cleared each frame, so live animation, palettes and HD geometry refresh. Existing per-plane stereo visibility/upload caches remain intact.

Normal terrain now uses a separate conservative frustum for each eye. Surviving contiguous vertex ranges merge without changing triangle order, textures, detail coordinates or geometry resolution. Near-plane intersections and eye-specific edge visibility are retained.

Added logs: Reflection geometry shared_builds/shared_reuses; Terrain view chunks=visible/tested and draw_calls. Device test passed 562,500 existing visibility cases plus new contiguous-range/gap, empty, per-eye and near-plane checks. Android build passed. In-game FPS improvement and final visual behavior await headset validation; no fixed FPS gain claimed.

反射の共通準備と通常地形の視野外除外を実装。反射候補は最初の水面の可視範囲に限定せず保持し、水面ごとの高さ・視野判定を維持。飛翔物は別計算とします。地形は眼ごとに保守的に判定し、連続する描画範囲のみを統合します。実機テストは通過。ゲーム中の改善幅と最終的な見え方は未確認です。
