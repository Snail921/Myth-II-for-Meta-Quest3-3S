# Release contents

**English** | [日本語](RELEASE_STATUS.ja.md)

The release includes the APK, fresh initial runtime, Plugin Manager/source, skyboxes and upstream source archives. Original Myth II files and third-party gameplay/HD plugins remain user-supplied.

A separate Quest 3 installation was tested with the newly generated runtime. Initial setup, original menu and Willow Creek VR entry were confirmed. Rendering logs recorded approximately 72 FPS and acknowledged unit commands. This is a beta; it does not establish compatibility with every community plugin.

The initial test omitted the original patch archives from the test data copy. All eight required patches were restored and hash-checked. Manager's base conversion already includes them.

The new build scripts use the pinned upstream Winlator APK and project sources. They do not depend on the old development APK or a device-exported runtime. See [build instructions](BUILD.md).

Use the runtime linked from the latest release; it is unchanged from 0.1.0-beta. Existing installations retain their game data and saves; do not repeat base setup over an existing Game folder.

See the [changelog](../CHANGELOG.md) and [graphics guide](GRAPHICS.md) for 0.5.0-beta. The initial runtime remains the same as 0.1.0-beta; use the link in the latest release. FPS depends on the scene and settings; enhanced features do not guarantee 90 FPS.

## 0.5.0 validation

Color correction and Bloom were user-confirmed on Quest. GPU tests cover every Bloom resolution, MSAA on/off, sky exclusion, state restoration and no residual glow between eyes. Seven Manager conversion/deployment tests pass. The APK retains the previous signing certificate; Java, rendering and capture entries match the headset-tested build. Rebuilt Windows helpers have identical executable sections, with changed timestamps only. The source ZIP includes the same source and bilingual controls/keyboard/graphics/setup guides as the repository.

## Preview 3 — 0.5.0-pbr-like.3 (2026-11-24)

This release updates only `pbr-like`; `main` is unchanged. It includes reflection preparation/culling improvements, the film-crash fix, configurable reflection sprite LOD up to 32 px, and recorded/fixed diagnostic poses. The APK and Manager/source ZIP correspond to the same source revision and bilingual guides. The Manager itself has no functional changes in this preview.

The release APK preserves the runtime from the headset-tested build and changes the release version metadata. It uses the existing signing certificate. This publication is not a new from-scratch runtime build. Existing data needs no reconversion: reuse the Preview 2 material pack and the linked initial runtime/skyboxes. See the README download links and the release checksums. Original game files are not included.


## Preview 4 — unified shadows and noShadow (2026-11-28)

Adds unified terrain/building/sprite shadows, cached static shadows and dynamic map resolutions through 8K; terrain base-color export/loading, interpolated terrain normals and shared mesh shader-state setup; independent shadowed/unshadowed projects, noShadow albedo priority over unedited HD textures, per-image normal inversion and the exit-recovery fix. Publishes a separate noShadow pack (10 collections / 97 images), updated APK and Manager/source ZIP. Existing normal/PBR packs, runtime and skyboxes remain available. Current performance is approximately 50–60 FPS as a guide due to rendering load and incomplete optimization, not a guaranteed rate. [noShadow setup and validation limits](NOSHADOW.md).
