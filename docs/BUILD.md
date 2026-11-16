# Build the APK and runtime

**English** | [日本語](BUILD.ja.md)

The public build uses the upstream Winlator 11.2 APK and this repository. It does not need a development APK, original game files, or a headset export.

## Tools (Windows)

Install Python 3.11 and `pip install -r requirements-build.txt`. Place these tools under the repository:

| Tool | Path |
| --- | --- |
| JDK 21.0.13+9 | `tools/runtime/jdk-21.0.13+9` |
| Android NDK r27d | `tools/runtime/android-ndk-r27d` |
| OpenXR loader AAR, extracted | `tools/runtime/openxr/loader` |
| Apktool 2.11.1 | `tools/downloads/apktool_2.11.1.jar` |

Install Android SDK platform 29 and build-tools 30.0.2. Set `ANDROID_HOME` to the SDK folder. The default is `%LOCALAPPDATA%/Android/Sdk`.

Download [Winlator 11.2](https://github.com/brunodev85/winlator/releases/tag/v11.2.0) to `tools/downloads/Winlator_11.2.apk`. The scripts check SHA-256 `ea5c81bf0e5b90a6b9ccb8007121ab6738c8646a5a60b341ce0ac2b33f1d100a`.

## Build

Create your own Android signing keystore. Keep it outside the repository and use the same key for subsequent updates.

```powershell
$env:PYTHONUTF8 = '1'
python tools/build_public_runtime.py --winlator-apk tools/downloads/Winlator_11.2.apk --out build/runtime-seed.tar
$env:MYTH_SIGNING_PASSWORD = 'your-keystore-password'
python tools/build_public_apk.py --winlator-apk tools/downloads/Winlator_11.2.apk --runtime build/runtime-seed.tar --work build/public-apk --out build/MythIIVR.apk --keystore C:/path/to/your.keystore
```

Use a new output/work directory for each build. The APK includes the matching runtime checksum. Both outputs have adjacent JSON reports with hashes.

The runtime builder uses an upstream fresh container, Wine builtin DLLs, and no local game state. The APK builder compiles the Windows helpers, ARM64 renderer and bridge, and Android UI, then applies the Winlator integration, aligns and signs the APK. It omits original game assets and optional native Windows component installers.

## Manager checks

```powershell
python -m unittest discover -s tools -p test_plugin_manager.py
python -m unittest discover -s tools -p test_setup_deploy.py
```

Historical device probes and conversion tests may need original game fixtures. The supported packaging entry points are the two `build_public_*` scripts above.

See [third-party notices](../THIRD_PARTY_NOTICES.md) for upstream sources and license terms.
