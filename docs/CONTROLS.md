# Controls

**English** | [日本語](CONTROLS.ja.md) · [README](../README.md) · [Setup](INSTALL.md)

These are the default VR controls in **0.2.0-beta**. The right controller aims the pointer; the **left trigger** confirms battlefield selections and orders. A grip is the side button, not the trigger under your index finger.

## Select and command units

| Action | Input |
| --- | --- |
| Select a friendly unit | Point at it and click the left trigger |
| Select several units | Hold the left trigger and drag a selection area |
| Add to the selection | Hold the right trigger while selecting |
| Select units of the same type | Double-click a friendly unit with the left trigger |
| Move | Select units, point at the ground and click the left trigger |
| Attack | Select units, point at an enemy and click the left trigger |
| Special ability | Left grip; availability depends on the selected unit |
| Attack a ground position | Hold the right grip and click the ground with the left trigger |
| Orders menu | Hold A, move the controller toward an item, then release A |
| Formation menu | With units selected, hold X, move toward an item, then release X |

### Turn without moving

Point at a friendly unit and hold the left trigger for about **0.35 seconds**. When the direction guide appears, point in the desired direction and release the trigger. An unselected unit is selected first. If that unit is already part of your selection, the selection is retained and each selected unit turns in place.

### Registered groups

| Action | Input |
| --- | --- |
| Register the selection | Hold the right trigger and X, choose a group slot, then release X |
| Choose a registered group from the menu | Hold the right grip and X, choose a slot, then release X |
| Previous / next group | Y / B |
| Clear selection | Either controller's group gesture, with gestures enabled |

After clearing or changing selection manually, the **first Y or B press recalls the last group**. Further presses cycle through registered groups. Empty groups are skipped.

For the gesture, bring one controller toward the other and stop it nearby; physical contact is unnecessary. Separate them before repeating.

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
| Show / hide minimap | Left controller menu button |
| Show / hide information panel | Right trigger + left menu button |
| Jump using the minimap | Point at a map position and press the left trigger |

## Game speed

- **Slower:** right grip + left stick click.
- **Faster:** right grip + right stick click.
- The current speed appears in white at the minimap's upper-right corner for about five seconds.

Release the stick click before the next step. These controls send the original game's F1/F2 speed commands, so multiplayer permissions and original speed limits still apply. The left grip remains assigned to special abilities.

## Co-op: transfer or return units

1. Select the units you want to hand over.
2. Hold the **right grip and press Y**.
3. With one eligible teammate, transfer is immediate. With several, a name list appears in front of your current gaze. Point at a name and click the **left trigger**.

Return units using the same operation. While the list is open, battlefield selection and movement orders are suspended. Press Y to close the list without transferring.

## Original menus and graphics settings

Use the right pointer and right trigger in the launcher and original game menus. To open the original level selector, enable **Hold Shift** in the input keyboard, then click **New Game** while the holding indicator is visible.

Launcher settings are grouped into collapsible sections. The display section offers automatic resolution or 1680 × 1760, 1832 × 1920, 1920 × 2048 and 2064 × 2208 per eye; 72 / 80 / 90 / 120 Hz; and MSAA off / 2×. Unsupported refresh rates fall back to a supported runtime setting. A selected refresh rate is a target, not a guaranteed frame rate. Apply changes on the next VR start.
