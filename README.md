# Myth II for Meta Quest 3 / 3S

**English** | [日本語](README.ja.md)

An unofficial VR frontend for playing Myth II: Soulblighter on Quest 3 / 3S. Step into an immersive battlefield and select and command your units with the controllers. Setup requires a PC, but gameplay runs entirely standalone on Meta Quest 3 / 3S.

**Download the APK, initial runtime and Plugin Manager below.** Setup requires your own original game ISO and the 1.8.5 updater.

[Downloads](https://github.com/Snail921/Myth-II-for-Meta-Quest3-3S/releases/tag/v0.5.0-beta) · [Installation guide](docs/INSTALL.md) · [Controls](docs/CONTROLS.md)

<p>
  <a href="https://raw.githubusercontent.com/Snail921/Myth-II-for-Meta-Quest3-3S/main/docs/images/screenshot-01.png"><img src="docs/images/screenshot-01.png" height="120" alt=""></a>
  <a href="https://raw.githubusercontent.com/Snail921/Myth-II-for-Meta-Quest3-3S/main/docs/images/screenshot-02.png"><img src="docs/images/screenshot-02.png" height="120" alt=""></a>
  <a href="https://raw.githubusercontent.com/Snail921/Myth-II-for-Meta-Quest3-3S/main/docs/images/screenshot-03.png"><img src="docs/images/screenshot-03.png" height="120" alt=""></a>
  <a href="https://raw.githubusercontent.com/Snail921/Myth-II-for-Meta-Quest3-3S/main/docs/images/screenshot-04.png"><img src="docs/images/screenshot-04.png" height="120" alt=""></a>
  <a href="https://raw.githubusercontent.com/Snail921/Myth-II-for-Meta-Quest3-3S/main/docs/images/screenshot-05.png"><img src="docs/images/screenshot-05.png" height="120" alt=""></a>
  <a href="https://raw.githubusercontent.com/Snail921/Myth-II-for-Meta-Quest3-3S/main/docs/images/screenshot-06.png"><img src="docs/images/screenshot-06.png" height="120" alt=""></a>
</p>

## From download to playing

1. **Prepare on PC** — Prepare an ISO of your own original Myth II CD and the 1.8.5 updater, then use Plugin Manager to convert the data for VR.
2. **Copy to Quest** — Install the APK, connect USB and use Manager to transfer the VR data converted in step 1.
3. **Start the game** — Open the Quest app library → **Unknown Sources** → **Myth II for Meta Quest 3 / 3S**. Run **Initial setup** once, wait for extraction to finish, then choose **Start Myth II**.
4. **Play** — Start a game from the original menu. The display switches to VR when a level starts.

The [installation guide](docs/INSTALL.md) lists the required software and what to select in each screen. Obtain any additional plugins separately.

The Myth II 1.8.5 patch, high-resolution map textures and sprites (Twice Born Edition), and plugins such as The Fallen Levels are available from [Project Magma](https://projectmagma.net/).

## Compatibility

| Game | Checked so far |
| --- | --- |
| Myth II: Soulblighter 1.8.5 | Tutorial completed; gameplay checked in Willow Creek and other levels |
| Chimera | VR startup checked in all 8 levels |
| The Fallen Levels | Basic gameplay checked; full playthrough testing continues. Keep TFL Gameplay OFF |

Not every plugin is supported. Plugins with a simple structure and moderate memory requirements are likely to work.

## Optional additions

- **Campaigns:** Convert and transfer a gameplay plugin with Manager.
- **HD images:** Select separately obtained HD addons during setup.
- **Skyboxes:** Extract the supplied skybox ZIP and select its folder in Manager. Custom skyboxes using the same format can also be used.

---

For developers: [Build notes](docs/BUILD.md) · [Release preparation](docs/RELEASE_STATUS.md) · [Licenses](THIRD_PARTY_NOTICES.md)

## Controls

[Open the controls guide](docs/CONTROLS.md) — Selection, movement, groups, co-op transfers and game speed.

## Updates

**0.5.0-beta** — Multiplayer chat, two-hand keyboard, minimap drawing, independent control swaps, rendering optimizations, color correction and Bloom (default OFF).

**0.3.2-beta** — Hash-only normal packs: direct transfer, smaller files and faster VR loading.

**0.2.0-beta** — Display settings, co-op transfers, improved facing/group controls, and shadow/sprite fixes.

[Full changelog](CHANGELOG.md)

[Graphics settings and FPS](docs/GRAPHICS.md) · [Sprite Material Editor](docs/MATERIALS.md)

[Install the normal-map library (206 collections)](docs/NORMAL_LIBRARY.md)

[Keyboard and chat](docs/KEYBOARD.md)

## Technical information

[How the VR frontend works: architecture and optimization notes](docs/TECHNICAL.md) — Component responsibilities, rendering techniques, validation results, gameplay observations and measurement guidance.
