# Controller remapping / コントローラーの入れ替え

The launcher Controls section provides independent swaps for triggers, grips, A/X, Y/B, sticks (including clicks) and whole controllers. All default to Off; re-enter VR after changing settings. The controller swap changes only the pointer/UI hands (and their feedback), independently of button and stick assignments. Individual button/stick swaps are independent.

Keyboard input always uses physical hands: right trigger/A, left trigger/X, Y Backspace, B Enter. Keyboard-consumed buttons do not leak into gameplay after remapping. Whole-controller swap attaches the wrist UI to the right hand and uses the left gameplay pointer. Info-panel buttons, portrait and inventory move to mirrored positions, with readable text and icon artwork.

Quest system/Meta controls and OS-managed 2D window pointers remain controlled by the OS.

起動画面の操作セクションに、トリガー、グリップ、A/X、Y/B、スティック（押し込みを含む）、コントローラー全体の入れ替えを追加しました。初期値はすべてOffです。変更後にVRへ入り直すと反映されます。コントローラーの入れ替えはポインターとUI追従側（およびそのフィードバック）のみを交換します。ボタン・スティックの各設定は独立しており、影響を受けません。

キーボードは物理側の割り当て（右トリガー/A、左トリガー/X、Yで削除、BでEnter）を維持します。キーボードが消費した入力はゲーム操作へ渡しません。全体入れ替えでは右手にUI、左手にゲーム用ポインターを配置し、インフォパネルのボタン、肖像、所持品も左右反転配置します。文字とアイコン自体の向きは維持します。

Questのシステム/Meta操作とOS管理の2DウィンドウのポインターはOS側の動作を維持します。

Validation: Android ARM64 test passed all 64 setting combinations for action/pose/stick routing, involution, physical keyboard exclusion and consumed-button routing. Visual/controller comfort requires headset verification.
