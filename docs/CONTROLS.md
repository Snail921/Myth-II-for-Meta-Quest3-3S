# Controls

**English** | [日本語](CONTROLS.ja.md) · [README](../README.md) · [Setup](INSTALL.md)

These are the default VR controls in **0.5.0-beta**. The right controller aims the pointer; the **left trigger** confirms battlefield selections and orders. Aiming and clicking use separate hands by default because double-clicking with the pointer hand can make it difficult to keep pointing at distant units. You can change these assignments in the launcher to suit your preference. A grip is the side button, not the trigger under your index finger.

## Select and command units

| Action | Input |
| --- | --- |
| Select a friendly unit | Point at it and click the left trigger |
| Select several units | Hold the left trigger and drag a selection area |
| Add to the selection | Hold the right trigger while selecting |
| Select units of the same type | Double-click a friendly unit with the left trigger |
| Move | Select units, point at the ground and click the left trigger |
| Attack | Select units, point at an enemy and click the left trigger |
| Special ability | With units selected, press the left grip; available abilities depend on the unit |
| Attack a ground position | With units selected, hold the right grip and click the ground with the left trigger |
| Orders menu | Hold A, move the controller toward an item, then release A |
| Formation menu | With units selected, hold X, move toward an item, then release X |

### Turn without moving

Point at a friendly unit and **hold the left trigger**. When the direction guide appears, point in the desired direction and release the trigger.

### Registered groups

| Action | Input |
| --- | --- |
| Register the selection | Hold the right trigger and X, choose a group slot, then release X |
| Choose a registered group from the menu | Hold the right grip and X, choose a slot, then release X |
| Previous / next group | Y / B |
| Clear selection | Right grip + B |
| Clear selection using a gesture | With gestures enabled, move the controllers toward each other as if lightly tapping them together (physical contact is unnecessary) |

In the launcher's controls settings, you can switch back to gesture-based group cycling. The default mode uses Y/B for cycling and both gestures for deselection.

## Camera and panels

| Action | Input |
| --- | --- |
| Move the viewpoint | Left stick |
| Orbit / turn | Right stick left or right |
| Raise / lower the viewpoint | Right stick up or down |
| Turn in place | Right grip + right stick left or right |
| Jump to selected units | Left stick click, without the right grip |
| Follow selected units | Right stick click, without the right grip |
| Leave follow mode | Move the left stick sideways |
| Show / hide minimap and information panel | Left controller menu button |
| Jump using the minimap | Point at a map position and press the left trigger |

## Game speed

- **Slower:** right grip + left stick click.
- **Faster:** right grip + right stick click.
- The current speed appears in white at the minimap's upper-right corner for about five seconds.

## Co-op: transfer or return units

1. Select the units you want to hand over.
2. Hold the **right grip and press Y**.
3. With one eligible teammate, transfer is immediate. With several, a name list appears in front of your current gaze. Point at a name and click the **left trigger**.

Return units using the same operation. While the list is open, battlefield selection and movement orders are suspended. Press Y to close the list without transferring.

## Original menus and graphics settings

Use the right pointer and right trigger in the launcher and original game menus. Hold Y in the original menu to open the software keyboard. Click **Shift**, then select **New Game** to open the level selector with locked levels available.

Launcher settings are grouped into collapsible sections. The display section offers automatic resolution or 1680 × 1760, 1832 × 1920, 1920 × 2048 and 2064 × 2208 per eye; 72 / 80 / 90 / 120 Hz; and MSAA off / 2×. Unsupported refresh rates fall back to a supported runtime setting. A selected refresh rate is a target, not a guaranteed frame rate. Apply changes on the next VR start.

[Enhanced shadows, lighting, LOD and FPS settings](GRAPHICS.md)

## Swap gameplay controls

In **Controls**, independently swap triggers, grips, A/X, Y/B and sticks (including clicks). All default to OFF. The tables above describe unswapped controls.

**Swap left/right controllers** changes only the pointer/UI hands. When ON, the left hand controls the pointer and the UI follows the right hand. Information-panel buttons, portraits and icons also exchange sides.

## Draw on the minimap

In multiplayer, click the information-panel pen to enable it. Aim at the map and use the selection trigger (left by default) to draw. This draws lines in your team color instead of moving the camera, and shares them with your team.

Click the eraser to clear drawings from the minimap.

**Allow Mini Map Rotation** defaults to OFF. When ON, the map follows HMD heading, but rotation temporarily stops while pen mode is enabled.

## Keyboard and chat

See [Keyboard and chat](KEYBOARD.md) for original-screen B/Y holds, two-hand typing, Yell/Whisper and fixed/follow placement.

## Diagnostic recorded and fixed poses (Preview 3)

Enable diagnostic pose recording in the launcher. Clicking the **physical left stick** records the viewpoint and both controllers' positions and orientations without freezing them. The saved pose persists across app restarts and APK updates.

Enable the diagnostic fixed-pose mode to restore and lock the recorded viewpoint/controllers on VR entry and start logging. If no valid recording exists for the level, the initial pose is locked instead and logging still starts. Click the **physical left stick** to end logging and release the lock. These diagnostic clicks are independent of gameplay control swaps.

A pose does not rewind the film: compare matching film ticks as well as the same view and settings. Keep diagnostic modes OFF for ordinary play.
