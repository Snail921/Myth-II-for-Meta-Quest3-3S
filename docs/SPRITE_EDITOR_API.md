# Sprite Editor live editing API

Enable **Direct connection** once per Editor session. Use the connection JSON with
`tools/sprite_editor_client.py`; keep its bearer token private. Restart an older
running Editor once to load this implementation, then enable the connection again.

## Workflow

1. GET `/state`: read `project_session`, `revision`, `project_path`, references and
   source identities. The revision covers source, settings and all editable layers.
2. GET `/source/{session}/{index}/image.png` and inspect the images. Reference
   colors guide identification of parts, including nearby shades, shape, location
   and animation context. They are **not** exact RGB selection criteria. The API
   does not itself perform semantic segmentation.
3. Build binary grayscale PNG masks at each source image's dimensions. White
   selects the identified part, black preserves it. Transparent source pixels are
   always excluded. GET `/metallic_smoothness/{session}/{index}/image.png` or
   `/thickness/{session}/{index}/image.png` to inspect existing layers.
4. POST `/material-edit` with `dry_run: true` and optional `preview_index` to inspect
   change counts and a base64 PNG highlighting the proposed selection. Inspect
   representative poses and uncertain frames; refine masks as necessary.
5. Submit the same masks without dry-run. Existing unsaved work is preserved,
   the selected material view refreshes immediately, and the project becomes dirty.
6. Save through `/save-project` when wanted. Manual save/reload is unnecessary.

## Commands

Every command below requires the current `project_session` and `revision`.
Stale state returns 409 with no edit; fetch state and recompute, rather than
blindly retrying an old mask. Processing is serialized on the UI thread. Busy
Editor requests return 503; wait for the operation to finish.

`POST /material-edit` body:

```json
{
  "project_session": "from state",
  "revision": "from state",
  "channel": "metallic_smoothness",
  "value": 0.6,
  "dry_run": true,
  "preview_index": 0,
  "masks": [
    {"index": 0, "image_sha256": "from frame identity", "png_base64": "binary L-mode PNG"}
  ]
}
```

Channels are `metallic_smoothness` and `thickness`; values are 0–1, quantized to
8 bits. A batch validates completely before mutation. Duplicate indices, invalid
dimensions, nonbinary masks or wrong source hashes are rejected. Unselected
pixels and other layers remain intact. Responses include changed `frames`,
`pixels`, `stored_value`, new `revision`, and optional `preview_png_base64`.
Requests are limited to 16 MiB; split large batches and fetch the new revision.

`POST /material-undo`: revert the last material-edit batch in this connection.
Any intervening content edit or project switch prevents undo from overwriting
new work. Undo restores the previous dirty state. It is one-level undo, not a
persistent history; a multi-batch operation needs a saved checkpoint for full rollback.

`POST /save-project`: pass an absolute `.smproj` `path`, or omit it to use the known
project path. Existing files require explicit `overwrite: true`. Saving includes
live unsaved layers/references and is atomic. New projects need a path once.

`POST /load-project`: pass an absolute `.smproj` `path`. Save dirty work first;
the API never discards it. Both save/load return 202; poll `/state` after busy
responses and check `last_operation.status` (`complete` or `error`). A completed
load changes the session. GUI save/load and normal session recovery remember the
project path. Material undo history is not restored across restarts.

CLI for any command above:

```powershell
python tools/sprite_editor_client.py --connection sprite-editor-connection.json command --route /material-edit --body-file edit.json
```

`/collections` and `/open-collection` remain available. To switch with unsaved
work, save via API, confirm completion, then open the selected collection ID.

## Per-image PBR flags

`GET /state` includes boolean `pbr_enabled` for every frame; revision covers these flags. `POST /pbr-set` takes:

```json
{"project_session":"from state","revision":"from state","indices":[0,1,2],"enabled":true,"dry_run":false}
```

Indices are zero-based, nonempty, unique and validated as a complete batch. `enabled` must be boolean. Existing unsaved layers are preserved. Dry-run returns the number of frames that would change. Responses include `frames`, `enabled`, `dry_run` and the new `revision`. `/material-undo` also reverts the last flag edit, with the same protection against intervening edits. CLI supports `command --route /pbr-set --body-file flags.json`. Layer edits never implicitly enable PBR.

### Material edit history

`GET /history/{project_session}` returns `{project_session, history}`. A stale session returns 404. `/state` includes `history_count`; connection capabilities include `history`. Entries contain UTC time, operation, channel, origin, changed image indices/names, pixel counts and before/after hashes. Brush entries additionally include value, radius, softness, blend and image-space points. UI material strokes, height copies, inversions, contrast adjustments and undo are recorded; API material writes are marked `origin: api`. This is material edit history, not a general UI event recorder.

CLI: `python tools/sprite_editor_client.py --connection sprite-editor-connection.json history --session SESSION`. History is saved in the project and can be read without changing editor state.


## Albedo variants (Preview 4)

All mutation requests require current `project_session` and `revision`.

- `POST /duplicate-unshadowed`: new absolute `.smproj` `path`; requires a saved shadowed project. Returns 202; poll state for completion and obtain the new session/revision.
- `POST /replace-albedo`: zero-based `index` and PNG `png_base64`; unshadowed projects only. Dimensions and alpha must match the original. Returns 200 and new revision.
- `POST /copy-materials`: absolute source `.smproj` `path`; source and target must share the original collection identity. Copies evaluated height/normal and material layers, keeping destination albedo and PBR flags. Returns 202; poll state.

Normal inversion is an image-specific Editor switch stored in project v8; there is no dedicated normal-inversion API route. Variant selection and source identities are exposed by state. Do not share connection tokens in published files. [noShadow workflow](NOSHADOW.md).
