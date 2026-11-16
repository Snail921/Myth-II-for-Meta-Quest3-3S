"""Read-only audit of the installed PC data, not just the extraction fixtures.

Absence of dmap/dtex proves there is no detail material to restore from this
installation; presence alone does NOT resolve plugin precedence or activation.
"""
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'research/references/mythextract/scripts'))
import myth_headers

def main():
    base = ROOT/'research/runtime/myth-1.8.1'
    records = []
    for folder in ('tags', 'plugins'):
        for path in sorted((base/folder).iterdir()):
            if not path.is_file() or path.name == 'plugin cache':
                continue
            data = path.read_bytes()
            header = myth_headers.parse_mono_header(path.name, data)
            tags = myth_headers.get_mono_tags(data, header)
            detail = [dict(type=str(t.tag_type), id=str(t.tag_id), name=str(t.name))
                      for t in tags if str(t.tag_type) in ('dmap', 'dtex', 'd256')]
            records.append(dict(path=str(path.relative_to(ROOT)),
                                sha256=hashlib.sha256(data).hexdigest(),
                                tag_count=len(tags), detail_tags=detail))
    loose = [str(p.relative_to(ROOT)) for p in (base/'local').rglob('*') if p.is_file()]
    log = (base/'myth_log.txt').read_text(errors='replace')
    settings = [line for line in log.splitlines()
                if line.startswith(('Running ', 'OpenGL: Initialized', 'Starting mesh'))]
    report = dict(archives=records, loose_files=loose, last_run_settings=settings,
                  detail_tag_count=sum(len(r['detail_tags']) for r in records),
                  interpretation='Presence requires load-order analysis; empty archive tags and local folder mean no supplied detail assets.')
    output = ROOT/'research/reports/pc-detail-tag-audit.json'
    output.write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps(dict(archives=len(records), detail_tags=report['detail_tag_count'],
                          loose_files=len(loose), last_run_settings=settings)))

if __name__ == '__main__':
    main()
