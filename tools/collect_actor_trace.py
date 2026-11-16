"""Collect opt-in Quest diagnostics without overwriting earlier captures."""
import argparse
import subprocess
from datetime import datetime
from pathlib import Path
parser = argparse.ArgumentParser()
parser.add_argument('--seconds', type=float, default=300)
parser.add_argument('--output', type=Path)
parser.add_argument('--serial', default='340YC10G8T047C')
args = parser.parse_args()
if args.seconds <= 0:
    parser.error('--seconds must be positive')
adb = 'C:/Users/gen/AppData/Local/Android/Sdk/platform-tools/adb.exe'
path = args.output or Path('build') / ('quest-diagnostic-' + datetime.now().strftime('%Y%m%d-%H%M%S') + '.log')
path.parent.mkdir(parents=True, exist_ok=True)
with path.open('xb') as f:
    print('Enable Diagnostic logging in the launcher. Saving: ' + str(path.resolve()), flush=True)
    try:
        result = subprocess.run([adb, '-s', args.serial, 'logcat', '-v', 'threadtime', '-T', '1', '-s', 'MythVR:I', 'MythStandalone:I', '*:S'], stdout=f, stderr=subprocess.STDOUT, timeout=args.seconds, check=False)
        if result.returncode:
            raise SystemExit('ADB collection failed; see ' + str(path))
    except (subprocess.TimeoutExpired, KeyboardInterrupt):
        pass
print('Diagnostic collection complete', flush=True)
