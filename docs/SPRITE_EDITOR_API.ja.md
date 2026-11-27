# Sprite Editor 画像別PBRフラグAPI

既存の材質編集・保存・接続手順は[API仕様（英語）](SPRITE_EDITOR_API.md)を参照してください。

`GET /state`の各画像に`pbr_enabled`（boolean）を返します。revisionはフラグも含みます。`POST /pbr-set`は次のbodyで画像を一括指定します（番号は0始まり）。

```json
{"project_session":"from state","revision":"from state","indices":[0,1,2],"enabled":true,"dry_run":false}
```

`enabled`はboolean、`indices`は空でない重複のない有効画像番号一覧です。全件検証してから変更し、M&S・Thickness等の既存編集は保持します。`dry_run:true`で変更対象数を確認できます。応答は`frames`（変更数）、`enabled`、`dry_run`、更新後`revision`です。未保存状態のまま実行でき、`/material-undo`で直前のフラグ変更を取り消せます。途中の編集がある場合は409で保護します。CLIは`command --route /pbr-set --body-file flags.json`です。材質レイヤーの編集はPBRフラグを自動変更しません。

接続ファイルのトークンは公開しないでください。Editor更新後は保存して再起動し、「直接連携」を有効にし直してください。

### 材質編集履歴

`GET /history/{project_session}` で `{project_session, history}` を取得します。古いセッションは404です。`/state` に `history_count`、接続能力に `history` を追加しています。履歴にはUTC時刻、操作、チャンネル、操作元、対象画像番号・名前、変更画素数、前後のハッシュを記録します。ブラシでは値・半径・柔らかさ・ブレンド方式・画像座標の軌跡も含みます。手作業の材質描画、Heightコピー、反転、コントラスト調整、Undo、およびAPIの材質書き込みを記録します。一般的なクリックすべての記録ではありません。

CLI: `python tools/sprite_editor_client.py --connection sprite-editor-connection.json history --session SESSION`。履歴はプロジェクトに保存され、取得は読み取り専用です。


## アルベド種別（Preview 4）

変更リクエストには最新の `project_session` と `revision` が必要です。

- `POST /duplicate-unshadowed`：未使用の絶対 `.smproj` `path`。保存済みの元画像版が必要です。202応答後は状態をポーリングし、新しいセッション／revisionを取得します。
- `POST /replace-albedo`：0始まりの `index` とPNGの `png_base64`。影除去版のみで利用でき、元画像とサイズ・透明度が一致する必要があります。200と新しいrevisionを返します。
- `POST /copy-materials`：コピー元 `.smproj` の絶対 `path`。元コレクション識別情報の一致が必要です。評価済み高さ／ノーマルと材質レイヤーをコピーし、コピー先のアルベドとPBRフラグは維持します。202応答後は状態をポーリングします。

ノーマル反転は画像ごとのEditorスイッチで、プロジェクトv8に保存します。専用の反転APIルートはありません。状態にはアルベド種別と元画像識別情報も含まれます。接続トークンは公開ファイルに含めないでください。[noShadowの作業手順](NOSHADOW.ja.md)。
