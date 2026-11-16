# Release status

**English** | [日本語](RELEASE_STATUS.ja.md)

## Published in this release

- Original VR/launcher/bridge/input source, including the 1.8.5 adaptation generator.
- Python Plugin Manager, conversion/deployment source, required MIT parser sources and tests.
- Optional Logo/CD artwork, 160 skyboxes, six gameplay screenshots.
- English/Japanese README and installation instructions.

## Not yet published

The tested external-data APK and the pinned initial runtime seed are **not** part of this public release. Therefore a new user cannot yet finish a fresh headset installation using only this release. Existing development installations can use the manager/source and optional assets.

This is not because Wine or Winlator uses the LGPL: redistribution is allowed subject to its terms. The remaining work is concrete:

1. Account for the exact sources, modifications, notices and build inputs corresponding to the runtime binaries shipped in the APK/seed, including Wine, GLib, FluidSynth, PulseAudio and graphics components. The reference Winlator source is pinned, but that alone does not establish all bundled-library provenance.
2. Review the `assets/wincomponents/*.tzst` payloads (Direct3D, DirectMusic, DirectPlay, DirectShow, DirectSound, VC runtimes, WM decoder and XAudio) individually. Their redistribution conditions cannot be inferred from Wine's license.
3. Replace the build's dependency on the private `myth-standalone-185-candidate.apk` with a documented clean build input. That private candidate includes original game material and must not be published.
4. Build a reproducible, sanitized initial runtime rather than relying on an exported local Winlator environment. The existing seed hash is a compatibility requirement, not proof of redistribution permission.

The development APK has passed an original-game-asset exclusion check and was tested on a fresh Quest 3. Those facts establish neither a complete source-build recipe nor license compliance for all its dependencies. `docs/provenance.json` records its hash for identification; the hash is not a download link.

The source publication includes historical development scripts. Some reference local fixtures and toolchains that are intentionally absent. See [build notes](BUILD.md). Original game files, converted levels, logs and signing keys are excluded.
