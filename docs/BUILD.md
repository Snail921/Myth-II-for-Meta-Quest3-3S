# Source and build notes

**English** | [日本語](BUILD.ja.md)

This is a development source snapshot, not yet a one-command clean APK build. The Plugin Manager can run from this tree after installing its documented dependencies. Runtime binaries and private game fixtures are deliberately absent.

## Layout and 1.8.5 generation

`src/standalone` and parts of `src/input`/`src/bridge` are shared/base implementation. The 1.8.5 build applies `tools/prepare_185_integration.py` and then `tools/prepare_185_full.py`, replacing provisional paths with full-feature hooks. Use the **full** generator, not the provisional generator alone:

```powershell
$env:PYTHONUTF8 = '1'
python tools/prepare_185_full.py
```

Generated source appears in `build/integration-185/`. It is not original Myth II source; the proprietary executable remains a separately supplied user input.

## Development toolchain

The existing build scripts use Android NDK r27d, JDK 21.0.13+9, Android API 29, Android Build Tools 30.0.2, OpenXR loader/headers, and Apktool 2.11.1. SDK paths in historical PowerShell scripts must be adjusted for the local machine. Do not run all scripts indiscriminately: some are device probes or development installers.

The current sequence generates source, builds x86 input helpers and the ARM64 bridge/renderer, compiles Java/Dex, patches a development runtime APK, then aligns and signs it. Relevant files:

- `tools/build_game_input.ps1`, `tools/build_bridge_arm64.ps1`, `tools/build_vr.ps1`
- `tools/prepare_185_full.py`
- `tools/build_185_integration_native.ps1`, `tools/build_185_integration_app.ps1`
- `tools/prepare_standalone_apk.py`, `tools/package_185_full.py`, `tools/patch_apk_fast.py`

**Packaging is not reproducible from this checkout alone.** `package_185_full.py` currently expects the private development candidate APK and local intermediate objects/import libraries. `prepare_standalone_apk.py` is the earlier binary-integration script and also expects generated assets. Do not distribute or request the old candidate APK: it contains original game data. The water-normal input is not included pending its provenance check. A clean source-build entry point is still required. No private signing key is published; generate your own key for a future clean build and understand update-signature compatibility.

## Manager checks

These tests use synthetic deployment data and do not require a headset or original game:

```powershell
python -m unittest discover -s tools -p test_plugin_manager.py
python -m unittest discover -s tools -p test_setup_deploy.py
```

Many other historical tests require original local fixtures, native toolchains, a running original game or a connected Quest. They are not all standalone CI tests. Base conversion is available through `tools/base_setup.py` and plugin conversion through `tools/convert_plugin_levels.py`; always pass your own original game input. Consult the installation manual before deploying.

## Upstream sources

The parser is vendored with its MIT notice. The Winlator reference is https://github.com/brunodev85/winlator-app/tree/2c33741d50c5daab0521a098be5c969f63b9e887 . It is a reference checkout, not a substitute for a complete versioned source/notice inventory of all runtime components. See [third-party notices](../THIRD_PARTY_NOTICES.md).
