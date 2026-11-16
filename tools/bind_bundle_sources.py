"""Bind converted bundles to portable original archive requirements."""
import argparse, hashlib, json, struct
from pathlib import Path, PureWindowsPath

def bind(bundle):
    bundle=Path(bundle)
    profile=json.loads((bundle/'source-profile.json').read_text(encoding='utf8'))
    if profile['version']!=1 or profile['engine']!='1.8.5':
        raise ValueError('Unsupported source profile')
    files=[]; seen=set(); selected=[]
    for archive in profile['archives']:
        kind=archive['kind']
        if kind not in (1,2,3):raise ValueError('Unsupported archive kind')
        name=PureWindowsPath(archive['path']).name
        if not name or name in ('.','..') or any(c in name for c in '/\\\0'):
            raise ValueError('Invalid archive name')
        path=('tags/' if kind==3 else 'plugins/')+name
        if path.casefold() in seen:raise ValueError('Ambiguous archive filename')
        seen.add(path.casefold())
        with Path(archive['path']).open('rb') as original: header=original.read(128)
        if len(header)!=128 or header[124:128]!=b'dng2':raise ValueError('Original archive header')
        raw_name=header[4:36].split(b'\0')[0].ljust(32,b'\0')
        identity=str(kind)+':'+raw_name.hex()+':'+str(struct.unpack_from('>I',header,104)[0])
        files.append(dict(path=path,size=archive['size'],sha256=archive['sha256'],runtime_identity=identity))
        if archive['path'] in profile['selected']:selected.append(identity)
    data=(json.dumps(dict(version=1,engine_sha256='aa68153d987bf8a17c30bf8c7711d3b5e0a291a6cd1f8630446496715539181c',
        scope='required files plus loaded archive identities and gameplay plugin order',files=files,active_plugins=selected),indent=2)+'\n').encode('utf8')
    (bundle/'source-files.json').write_bytes(data)
    digest=hashlib.sha256(data).hexdigest()
    for row in (bundle/'catalog.tsv').read_text(encoding='utf8').splitlines():
        key=row.split('\t')[0]
        if len(key)!=8 or any(c not in '0123456789abcdef' for c in key):raise ValueError('Invalid level ID')
        path=bundle/'levels'/key/'manifest.json';m=json.loads(path.read_text())
        m['source_files_sha256']=digest
        path.write_text(json.dumps(m,indent=2),encoding='utf8')
    return digest

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('bundle');a=p.parse_args();print(bind(a.bundle))
