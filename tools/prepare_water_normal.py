"""Package artist normal-map RG channels as linear data, without color correction."""
from pathlib import Path
from PIL import Image
import struct
root=Path(__file__).resolve().parents[1]
source=root/'Assets/Water/WaterNormal.png'
with Image.open(source) as image:
 image=image.convert('RGB');w,h=image.size
 if not (1<=w<=2048 and 1<=h<=2048):raise ValueError('normal-map dimensions')
 pixels=bytes(v for rgb in image.getdata() for v in rgb[:2])
(root/'build/vr/assets/water-normal.pack').write_bytes(b'MYTHWN01'+struct.pack('<II',w,h)+pixels)
print('Water normal:',source,w,h,'linear RG8')
