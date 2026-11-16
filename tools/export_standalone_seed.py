"""One-time local development provisioning, not part of standalone play.
Run after stopping the original Winlator session. Keeps original files intact.
Base64 avoids binary corruption observed on this device's run-as output channel.
"""
import base64,subprocess,tarfile
from pathlib import Path

ADB='C:/Users/gen/AppData/Local/Android/Sdk/platform-tools/adb.exe'
root=Path(__file__).resolve().parents[1]
target=root/'research/fixtures/standalone/runtime-seed.tar'
target.parent.mkdir(parents=True,exist_ok=True)
subprocess.run([ADB,'shell','run-as com.winlator /system/bin/tar --exclude=files/rootfs/tmp -cf files/myth-seed.tar files/rootfs files/installed_components'],check=True)
p=subprocess.Popen([ADB,'exec-out','run-as','com.winlator','base64','files/myth-seed.tar'],stdout=subprocess.PIPE)
with target.open('wb') as out:
    for line in p.stdout:out.write(base64.b64decode(line,validate=False))
if p.wait()!=0:raise RuntimeError('Seed export failed')
with tarfile.open(target) as archive:
    members=archive.getmembers()
    if not any(m.name=='files/rootfs/home/xuser-1/.container' for m in members):raise RuntimeError('Incomplete seed')
    for m in members:
        if not m.name.startswith('files/') or '..' in Path(m.name).parts:raise RuntimeError('Unexpected archive path')
print(f'Validated {len(members)} entries; {target.stat().st_size} bytes')
subprocess.run([ADB,'push',str(target),'/sdcard/Download/MythVR-runtime-seed.tar'],check=True)
