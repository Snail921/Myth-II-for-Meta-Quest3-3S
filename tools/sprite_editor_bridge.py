"""Explicitly enabled, authenticated, localhost editor bridge with guarded live material editing."""
import hashlib
import time
import io
import json
import queue
import secrets
import threading
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer


class EditorBridge:
    def __init__(self, editor):
        self.editor = editor
        self.requests = queue.Queue(maxsize=16)
        self.token = secrets.token_urlsafe(32)
        bridge = self

        class Handler(BaseHTTPRequestHandler):
            def log_message(self, *args): pass
            def do_POST(self):
                self.do_GET()
            def do_GET(self):
                if self.headers.get('Origin') or not secrets.compare_digest(self.headers.get('Authorization', ''), 'Bearer '+bridge.token):
                    self.send_error(403); return
                body = None
                if self.command == "POST":
                    try:
                        size=int(self.headers.get("Content-Length", "0"))
                        if not 0<size<=16*1024*1024: raise ValueError()
                        self.connection.settimeout(2)
                        body=json.loads(self.rfile.read(size))
                        if not isinstance(body,dict):raise ValueError()
                    except (ValueError, OSError):
                        self.send_error(400);return
                reply = queue.Queue(maxsize=1)
                try:
                    bridge.requests.put_nowait((self.path, body, time.monotonic()+4, reply))
                    status, kind, data = reply.get(timeout=5)
                except (queue.Full, queue.Empty):
                    self.send_error(503); return
                try:
                    self.send_response(status); self.send_header('Content-Type', kind)
                    self.send_header('Cache-Control', 'no-store'); self.send_header('Content-Length', str(len(data)))
                    self.end_headers(); self.wfile.write(data)
                except OSError: pass

        self.server = ThreadingHTTPServer(('127.0.0.1', 0), Handler)
        self.server.daemon_threads = True
        threading.Thread(target=self.server.serve_forever, daemon=True).start()
        self.job = editor.root.after(100, self.poll)

    def connection(self):
        return dict(version=1, url='http://127.0.0.1:'+str(self.server.server_port), token=self.token,
                    capabilities=['state', 'source_png', 'collections', 'open_collection', 'material_edit', 'material_undo', 'save_project', 'load_project', 'layer_png', 'pbr_set', 'history', 'color_selection'], read_only=False)

    def poll(self):
        for _ in range(8):
            try: path, body, deadline, reply = self.requests.get_nowait()
            except queue.Empty: break
            if time.monotonic()>deadline:
                reply.put_nowait((408, "text/plain", b"Request expired"));continue
            e = self.editor; p = e.project
            if getattr(e, "material_paint", None):e.material_paint.finish()
            if e.busy:
                result = (503, 'text/plain', b'Editor busy')
            elif body is not None:
                if path in ('/material-edit', '/material-undo', '/save-project', '/load-project', '/pbr-set'):
                    from sprite_editor_commands import execute
                    result=execute(self, path, body)
                elif path != '/open-collection':
                    result=(404,'text/plain',b'Unknown command')
                elif e.dirty or body.get('project_session')!=e.project_session:
                    result=(409,'text/plain',b'Unsaved changes or stale project session; save in Editor first')
                else:
                    candidates=self.collections()
                    matches=[c for c in candidates if c['id']==body.get('collection_id')]
                    if len(matches)!=1:result=(404,'text/plain',b'Unknown collection ID')
                    else:
                        e.collection(matches[0]['path'])
                        result=(202,'application/json',json.dumps({'accepted':True,'collection_id':matches[0]['id']}).encode())
            elif path == '/collections':
                result=(200,'application/json',json.dumps(self.collections(),ensure_ascii=False).encode('utf-8'))
            elif path.startswith('/history/') and path.split('/')[-1]==e.project_session:
                result=(200,'application/json',json.dumps(dict(project_session=e.project_session,history=p.history),ensure_ascii=False).encode('utf-8'))
            elif path == '/state':
                from sprite_editor_commands import revision
                data = dict(version=2, revision=revision(p), project_path=getattr(e,'project_path',None), last_operation=getattr(self,'last_operation',None), project=p.name, project_session=e.project_session,
                    history_count=len(p.history), selected_index=e.index, dirty=e.dirty, busy=e.busy, material_hints=p.material_hints,
                    frames=[dict(index=i, name=f.name, size=f.image.size, identity=f.identity, pbr_enabled=f.pbr_enabled) for i,f in enumerate(p.frames)])
                if getattr(e,'selection',None):data['color_selection']=e.selection.api_state()
                result = (200, 'application/json', json.dumps(data, ensure_ascii=False).encode('utf-8'))
            else:
                # Session in URL prevents reading a different collection after a switch.
                parts = path.split('/')
                if len(parts)==5 and parts[1] in ('source','metallic_smoothness','metallic','thickness') and parts[2]==e.project_session and parts[3].isdigit() and parts[4]=='image.png' and int(parts[3])<len(p.frames):
                    out=io.BytesIO();f=p.frames[int(parts[3])];getattr(f,'image' if parts[1]=='source' else parts[1]).save(out,format='PNG')
                    result=(200,'image/png',out.getvalue())
                else: result=(404,'text/plain',b'Not found or stale project session')
            reply.put_nowait(result)
        self.job = self.editor.root.after(100, self.poll)

    def collections(self):
        from pathlib import Path
        from sprite_editor_files import collections
        paths=set()
        folders=set(self.editor.collection_browsers)
        previous=self.editor.paths.initial('collection_browser')
        if previous:folders.add(str(previous))
        for folder in folders:
            try:paths.update(str(p.resolve()) for p in collections(folder))
            except OSError:pass
        return [dict(id=hashlib.sha256(p.encode()).hexdigest(),name=Path(p).stem,path=p) for p in sorted(paths)]

    def close(self):
        self.editor.root.after_cancel(self.job)
        self.server.shutdown(); self.server.server_close()
