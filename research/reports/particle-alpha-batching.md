# Particle alpha batching / 粒子の透明度を維持した描画集約

The Beyond Cloudspine capture reached 33 FPS with GPU utilization at 99%. Normal effects CPU time was 4.728 ms/frame and reflected effects 4.658 ms/frame. Particle logs showed roughly 280–300 draws per pass. Terrain GPU cost also rose, so this change alone does not establish a complete fix.

ParticleStream already merged adjacent matching draws, but required identical alpha. Fade differences split otherwise compatible draws even with the texture atlas enabled. Alpha is now a flat per-vertex attribute (location 9, w component), retaining original sorted primitive order. Adjacent ranges merge only when texture, owner and resolved material/atlas keys match. Texture/material/owner transitions still split; no sorting by texture, opacity quantization or particle reduction is introduced. Normal and reflection paths use the same change. Disabled attribute defaults to w=1; the particle pass restores this state afterward.

`legacy_draws` records how many draws the former alpha-sensitive grouping would have required for the same input. Compare it with `draws` in Particle stages logs. Extra opacity stream costs 16 bytes per vertex (96 bytes per billboard).

Validation: full Android build; Quest GPU test covering four shadow/lighting shader variants, normal and premultiplied reflection blending. Three overlapping particles with distinct alpha produced pixel-identical output between separate uniform-alpha draws and one vertex-alpha draw. Existing cutout/soft-alpha checks also pass. Actual Beyond Cloudspine FPS improvement remains to be measured at the same viewpoint.

Beyond Cloudspineの記録では最低33FPS、GPU使用率99%、通常エフェクトCPU約4.7ms/フレーム、反射側約4.7msを確認。地形GPU負荷も増えているため、この修正だけで解消するとは断定できません。

既存の連続描画の集約が粒子ごとの透明度差で分割されていました。透明度を頂点属性へ移し、描画順、透明度、粒子数を保ったまま、同じテクスチャ・材質・所有者の連続部分を集約します。通常表示と反射の両方に適用します。ログのlegacy_drawsとdrawsから、同じ入力に対する描画命令の削減数を確認できます。

Quest上のGPU比較で、通常表示と反射表示の画素値一致を確認済み。実際の滝でのFPS改善幅は未測定です。
