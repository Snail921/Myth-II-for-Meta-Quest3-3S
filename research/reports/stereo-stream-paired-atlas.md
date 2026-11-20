# Stereo upload sharing and paired atlas pages — 2026-11-19

Implemented in the existing renderer and atlas option. No data conversion or Manager changes are required.

- Normal opaque sprites use the union of both eye frusta. Geometry, reference sorting, packed buffers and uploads are produced once per frame; the second eye binds and draws those buffers. Eye-specific GPU clipping remains. Logs expose `Sprite stereo shared_upload_reuses`.
- Reflection retains per-eye water-region culling, rather than expanding its draws to the entire reflected view. Its upload construction benefits from the direct-reference path, but eye-specific visibility can still cause two uploads. Transparent effects retain their existing sorted path.
- `PreparedSprite` vertices are referenced directly by `SpriteStream::Input`, then copied once into the final stream. Intermediate texture/owner vertex vectors were removed. Reusable reference storage plus in-place sort replaces a new local vector and stable-sort scratch allocation on each upload. Original texture/owner/order tie-breaks retain deterministic ordering within material groups.
- Lighting bounds are computed once during sprite preparation and reused while packing. Main output arrays retain capacity with rounded reservation. Normal/atlas mapping, owner IDs and bounds remain associated with each sprite after grouping.
- Color atlas pages can share space across collections; ordinary and HD images use separate page pools to avoid splitting draws by edge treatment. Normal pages are assigned to their corresponding color page. This avoids independent collection-based color/normal layouts creating unrelated material combinations. Color and normal UV rectangles remain independent, preserving original normal resolution and HD crop/flip mappings. Reused normals can be copied into more than one companion page; the existing total 256 MiB cap and original-texture fallback still apply.

Quest GPU verification: actual production streams and shaders; direct-reference binocular union versus independently culled per-eye uploads (including sprites visible in only one eye), pixel-identical in both eyes; four shadow/light variants, HD edges, alpha order, normals/crops/flips, reflection, dirty palette copies, cross-collection page sharing, rollover/fallback and level cleanup passed. Legacy-versus-atlas maximum channel difference remains 1/255.

Synthetic full Willow Creek base-scene test: all 4,282 referenced images from 35 collections registered, fallback zero. Pages/ranges decreased from 71 to 46 and additional color atlas storage from 77 to 52 MiB. This all-image test excludes normal/HD payloads and is not a gameplay FPS measurement.

Remaining tradeoffs: binocular union submits some edge sprites to an eye that will clip them; normal companion pages can duplicate normals across color pages. Actual FPS, CPU upload time, draw count and atlas capacity must be checked in gameplay. Old per-eye source range counts and new union counts are not directly comparable as identical workloads.


## Correction: unique normal residency (2026-11-19)

This supersedes companion normal pages described above. The experiment reached the 256 MiB cap during gameplay (223 pages, 2,872 normal entries, 1,528 fallbacks). Entries could include the same normal once per color page.

Removed the color page from the normal residency key and pool assignment. Each immutable normal source GL texture now has one atlas slot shared across all color pages and collections within the level. Level cleanup clears residency. Color/normal UV rectangles remain independent. Stereo upload sharing, direct vertex references, sorting reuse and shared color pages are retained. Independent normal pages may produce more material groups, but avoid duplicated normal storage. The 256 MiB limit is unchanged.

Quest GPU regressions passed: distinct color pages share the same normal slot without an extra GPU copy; another collection and 100 repeated resolves also produce no normal copies. Crop/flip mappings remain independent. Existing production shader/stream comparisons passed (maximum channel difference 1/255), including four lighting/shadow variants, alpha order, reflection, stereo reuse and level cleanup. Gameplay memory and FPS after correction require another playback; these tests do not establish FPS improvement.
