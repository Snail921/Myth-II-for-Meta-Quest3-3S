# Scene color correction / シーンの色補正

Launcher: Color correction > Enable color correction; strength 0–100%, handle-following value. Default OFF, stored strength 100%. Settings apply on next VR entry.
起動画面「色補正」で有効／無効と強度を設定。初期値OFF・強度100%。次回VR起動時に反映。

The display-encoded artwork after detail/shadow modulation is transformed as:
`mix(c, clamp(c * (255 / 199), 0, 1), strength)`.
Black point 0, white point 199. Fog is composed afterward, followed by the existing sRGB decode and enhanced local-light contribution. This intentionally preserves authored fog/sky colors and the existing additive local lighting, rather than grading the final composited screenshot.
空・UIは対象外。地形・モデル・ユニット・パーティクル・天候を補正し、フォグと既存の追加照明は従来どおり合成。反射は描き込み時に一度だけ補正し、水面合成では二重補正しない。

OFF returns byte-identical original fragment source, without correction uniforms/calculations; host uniform updates are also skipped. ON adds multiply/clamp/mix and a uniform branch to existing scene/weather shaders, with strength zero during UI. No new render target, full-screen pass, texture sampling or histogram analysis. Actual ON frame-time overhead has not been benchmarked.
OFFは元のシェーダーソースをそのまま使用。全画面パスや輝度解析は追加しない。ON時の実シーンでのFPS差は未計測。

Validation: tools/test_scene_correction_gpu.cpp executed on connected Quest. Four shadow/light combinations, strengths 0/50/100%, clear/partial/full fog, normal and premultiplied reflection, RGB expected-value comparisons (2-byte tolerance), alpha preservation. Existing cutout/soft-alpha and batching comparisons also pass. OFF source equality asserted.
