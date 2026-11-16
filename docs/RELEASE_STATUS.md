# Release contents

**English** | [日本語](RELEASE_STATUS.ja.md)

The release includes the APK, fresh initial runtime, Plugin Manager/source, skyboxes and upstream source archives. Original Myth II files and third-party gameplay/HD plugins remain user-supplied.

A separate Quest 3 installation was tested with the newly generated runtime. Initial setup, original menu and Willow Creek VR entry were confirmed. Rendering logs recorded approximately 72 FPS and acknowledged unit commands. This is a beta; it does not establish compatibility with every community plugin.

The initial test omitted the original patch archives from the test data copy. All eight required patches were restored and hash-checked. Manager's base conversion already includes them.

The new build scripts use the pinned upstream Winlator APK and project sources. They do not depend on the old development APK or a device-exported runtime. See [build instructions](BUILD.md).

Use the APK and runtime from the same release. Existing installations retain their game data and saves; do not repeat base setup over an existing Game folder.
