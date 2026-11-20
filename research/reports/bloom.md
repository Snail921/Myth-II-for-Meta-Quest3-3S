# Bloom / ブルーム

## User settings / 設定
Launcher Bloom section: Enable Bloom (default OFF), processing resolution 1/32, 1/16, 1/8, 1/6, 1/4, 1/2 per dimension (default 1/8), separable blur iterations 1–4 (default 1), strength 0–200% (default 15%). Values follow slider handles. Changes persist and apply on next VR entry. Strength zero also bypasses processing.
起動画面のBloomで有効化。初期値はOFF、縦横1/8・ぼかし1回・強度15%。次回VR起動時に反映。強度0も完全迂回。

## Pipeline / 描画方式
Existing scene renders normally. Only with Bloom active, clear eye alpha to zero and prevent the sky pass writing alpha. Scene geometry and translucent effects supply coverage; reflections keep their existing alpha behavior. Capture scene color before controls/UI into a same-size, same-format texture using a color blit (also resolves MSAA). Sample linear RGB, suppress sky-only pixels using coverage alpha, soft-threshold peak RGB from 0.65 to 0.95, average 16 samples per low-resolution output pixel. Apply horizontal+vertical 5-fetch Gaussian blur per iteration, then add the glow in linear RGB to the original eye target. Restore eye alpha to 1 and render controls/UI afterward. No reflected scene Bloom pass; reflected highlights can glow in the main eye image.

OFF leaves original clear/sky behavior, allocates no Bloom resources and issues no Bloom rendering calls. Temporary images are reused sequentially across eyes and fully overwritten. All touched GL state is restored; targets/programs are released before EGL context destruction. Source capture is full eye resolution even at 1/8 blur resolution: this resolve/copy and final full-screen blend impose a bandwidth cost that slider reduction cannot eliminate.

空だけの画素を発光源から除外し、UI描画前に合成。空の上に光が広がること自体は許容。炎・魔法という種類で判定するのではなく、描画された明るいハイライトを抽出。半透明FXは背景と合成された後の明るさ／カバレッジで決まるため、薄いFXは光りにくい。アルファマスクの境界ではフィルタリングにより背景色も混ざりうる。水面に映った明るい色は主画面側で発光しうる。

## Validation / 検証
`tools/test_bloom_gpu.cpp`, compiled Android arm64 GLES3 executable, passed on Quest 2G0YC5ZF7P0623:
- RGBA8 and SRGB8_ALPHA8; single-sample and 2x MSAA.
- Every resolution, 1 and 4 blur iterations: sky-only source produces no halo, scene source produces a halo, final alpha is opaque.
- Caller framebuffer/texture unit/blend/depth/scissor/cull/color-write state preserved.
- Blank second eye after bright first eye contains no residual glow.
- OFF allocates no targets/programs.

Synthetic 1536x1536 single-sample SRGB benchmark (12 passes, includes source painting and glFinish; CPU wall-time, not isolated GPU timer, not game FPS):
| Divisor | Iterations | ms per eye pass |
|---|---|---|
| 8 | 1 | 0.503 |
| 8 | 4 | 0.544 |
| 4 | 1 | 0.591 |
| 4 | 4 | 0.731 |
| 2 | 1 | 0.734 |
| 2 | 4 | 1.117 |

In-game diagnostic Stage CPU/GPU id=14 is Bloom (CPU name=bloom, reflection=false). Existing nonblocking timer sampling includes capture, extraction, blur and composite. Actual gameplay appearance/frame-rate remains for headset playtesting. Build succeeded with pre-existing warnings.

## Strength range update / 発光強度の上限拡張

Raised the strength limit from 50% to 400% (8x the old maximum). Saved values and the 15% default retain their exact meaning. Threshold, resolution, iterations and draw-pass count are unchanged.
発光強度のみ上限を400%へ拡張。既存設定は維持。明るさの抽出条件や描画パス数は変更しない。

## Lower resolution range / 低解像度の追加

Added 1/16 and 1/32 per dimension. Persisted IDs 0–3 retain their original meanings; new IDs 4 and 5 are ordered at the low end of the launcher slider. Existing settings and the 1/8 default are preserved. GPU tests include the two new resolutions.

Strength maximum subsequently reduced to 200% at user request. Existing values above 200% clamp to 200%; the default remains 15%. Native loading also clamps previously written values up to 400%.
