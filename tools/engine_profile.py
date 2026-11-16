"""Exact-build keyed profiles. Diagnostic tool only; unknown binaries fail closed."""
import hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def load_profile(exe):
 digest=hashlib.sha256(Path(exe).read_bytes()).hexdigest()
 for p in json.loads((ROOT/'src/engine/profiles.json').read_text())['profiles']:
  if p['sha256']==digest:return p
 raise ValueError('Unsupported executable SHA-256: '+digest)
def parse_pool(raw,layout):
 import struct
 return dict(name=raw[:16].split(b'\0')[0].decode('ascii',errors='replace'),stride=struct.unpack_from('<H',raw,layout['stride'])[0],count=struct.unpack_from('<H',raw,layout['count'])[0],data=struct.unpack_from('<I',raw,layout['data'])[0])
