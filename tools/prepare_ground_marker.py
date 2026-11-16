"""Package the user-provided ground attack marker without changing its pixels."""
from pathlib import Path
from PIL import Image
import struct
root=Path(__file__).resolve().parents[1]
im=Image.open(root/'Assets/GroundAttackMarker.png').convert('RGBA')
assert 0<im.width<=1024 and 0<im.height<=1024
out=root/'build/vr/assets/ground-attack-marker.rgba'
out.parent.mkdir(parents=True,exist_ok=True)
out.write_bytes(struct.pack('<II',*im.size)+im.tobytes())
