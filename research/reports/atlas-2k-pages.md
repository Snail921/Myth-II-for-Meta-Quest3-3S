# 2K atlas page experiment — 2026-11-19

Changed the basic atlas page size from 512 to 2048 pixels. All new color and normal pages are 2048 x 2048 RGBA8 without mipmaps (16 MiB each). Existing shared normal residency, ordinary/HD color pools, cross-collection packing, binocular upload reuse and direct-reference packing are retained. The additional atlas budget remains 256 MiB, allowing at most 16 pages; original-texture fallback remains available. No data conversion or Manager changes are needed. The atlas enable toggle is unchanged.

Added layout diagnostics reporting page_side, color_pages and normal_pages alongside existing memory, copy, fallback and batch counts.

Quest GPU tests passed: production shader/stream comparisons across four shadow/light variants, crop/flip mappings, transparency order, reflection, stereo reuse, dirty copies, gutters, shared normal residency, explicit same-pool 2K rollover and level cleanup. Maximum channel difference from non-atlas rendering: 1/255.

Full Willow Creek base-scene fixture: 4,282 images from 35 collections, 4 color pages / 64 MiB, 4 grouped ranges, zero fallback. Previous 512-based packing of the same fixture used 46 pages / 52 MiB and 46 ranges. This fixture excludes HD and normal textures and does not measure gameplay FPS. Actual gameplay must verify page budget use, draw count and FPS; larger pages trade fewer switches for larger allocation increments and potentially more unused space.


## Capacity trial: 768 MiB

Following the user request, raised the atlas-only allocation ceiling to 768 MiB (48 RGBA8 2K pages). Pages remain allocated on demand, not upfront. Original textures, normal textures, eye buffers and other resources are additional memory. The 256 MiB trial reached 16 pages and fell back during extended gameplay. The larger ceiling needs gameplay verification for actual residency, fallback counts and device memory; it does not guarantee every level fits. Packing and shared normal residency are unchanged.
