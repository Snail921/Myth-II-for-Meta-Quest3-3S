# Keyboard and multiplayer chat

**English** | [日本語](KEYBOARD.ja.md) · [Controls](CONTROLS.md)

## Original menus, lobby and text fields

Automatic keyboard display is disabled. In the original compatibility screen, menus or a window with a text field, hold **B or Y for one second**. It opens at the one-second threshold; releasing the button is unnecessary. Select the intended input field before typing. Near the bottom of the screen, the board can appear above the field and stays in position while typing.

**Enter** sends Enter to the original field and closes the keyboard. **Close** closes without sending another key; it does not undo characters already entered into the field. These controls differ from the floating VR chat keyboard below. The OS controls pointers in 2D windows.

For the original level selector, enable **Hold Shift** in the input keyboard, then click **New Game** while the holding indicator is visible.

## Open VR multiplayer chat

On the on-line list above the information panel:

- Click the **game-rule header** for **Yell** (everyone).
- Click **your own team row** for **Whisper** (your team). Other team rows do not address that team.

Type text and press **Enter** to send and close. Empty Enter or **Close** cancels the draft. ASCII letters and symbols, Shift, Backspace and cursor arrows are supported, up to 191 characters; this is not a Japanese IME. The input caret blinks.

Sent and received messages appear above the list with a team-colored pointed name banner and outlined white text. Messages stay for 10 seconds and fade over one second. New messages appear at the bottom and push earlier ones upward. Original notification sounds are retained.

## Two-hand typing in VR

| Action | Physical controller input |
| --- | --- |
| Press the key under the right pointer | Right trigger or A |
| Press the key under the left pointer | Left trigger or X |
| Backspace while either pointer is over the keyboard | Y |
| Enter while either pointer is over the keyboard | B |

The left typing ray appears only when it hits the keyboard. Keyboard presses do not also select units, cycle groups or open radial menus; they remain owned by the keyboard until released. Away from the keyboard, ordinary gameplay remains available. **Paused Game** and **Mission Objectives** are greyed out while chat input is open.

Keyboard input always uses the physical hands above, even when gameplay controls are swapped.

## Placement

The launcher's **Keyboard** section controls the floating chat board:

- **Fixed Chat Keyboard** defaults to ON: holds the opening location in the game world until closed.
- With fixed mode OFF, the board follows gaze. **Follow smoothing** adjusts rotational following; position/depth following remains immediate.
- Horizontal, vertical, distance and smoothing sliders show offsets from their defaults. **0 means center/default**, not zero distance or no smoothing.
- The baseline is horizontal 0 cm, vertical −40 cm, distance 175 cm and smoothing 493 ms. Smoothing runs from −493 to +493 ms relative to that baseline.

Values follow the handles. Changes apply on next VR entry. These placement settings affect VR chat, not the original-screen keyboard.
