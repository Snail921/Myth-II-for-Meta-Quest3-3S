"""Provision and start a local development VR bridge for an already loaded Myth map.
The native capture is read-only; the game-thread hook invokes original selection/movement routines.
Uses the debug Winlator UID to resume its own paused guest processes.
"""
import argparse,secrets,subprocess,time
from pathlib import Path
ADB='C:/Users/gen/AppData/Local/Android/Sdk/platform-tools/adb.exe'
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser();p.add_argument('--serial',default='340YC10G8T047C');p.add_argument('--seconds',type=int,default=600);args=p.parse_args()
 if not 1<=args.seconds<=3600:raise ValueError('duration must be 1..3600 seconds')
 base=[ADB,'-s',args.serial]
 def shell(cmd):return subprocess.check_output(base+['shell',cmd],text=True)
 ps=shell('ps -A');matches=[line for line in ps.splitlines() if line.endswith('Myth II.exe')]
 if len(matches)!=1:raise RuntimeError('Load Willow Creek in Winlator first; expected one Myth II.exe')
 pid=int(matches[0].split()[1]);uid=matches[0].split()[0]
 if not any(line.endswith('mythinput.exe') for line in ps.splitlines()):
  shell('run-as com.winlator files/myth-bridge/win-exec "C:\\mythvr\\mythinput.exe"')
  time.sleep(1)
  if not any(line.endswith('mythinput.exe') for line in shell('ps -A').splitlines()):raise RuntimeError('Original input helper did not start; check installer.bin')

 services=shell('dumpsys activity services com.winlator')
 if 'isForeground=true' not in services:raise RuntimeError('Enable Winlator background protection and restart its session first')
 # Re-provision a fresh capability in each app-private directory without printing it.
 shell('am force-stop org.mythvr.viewer')
 token=secrets.token_hex(16).encode("ascii")
 for package,path in [('org.mythvr.viewer','files/bridge.token'),('com.winlator','files/myth-bridge/bridge.token')]:
  subprocess.run(base+['shell','-T',f'run-as {package} sh -c "cat > {path}; chmod 600 {path}"'],input=token,check=True)
 for package,path in [('org.mythvr.viewer','files/bridge.token'),('com.winlator','files/myth-bridge/bridge.token')]:
  if int(shell(f'run-as {package} wc -c {path}').split()[0])!=32:raise RuntimeError('Capability provisioning failed')
 shell('am start -n org.mythvr.viewer/android.app.NativeActivity')
 time.sleep(4)
 # Winlator's onPause stops Wine; resume only stopped processes belonging to that engine UID.
 ps=shell('ps -A');stopped=[int(line.split()[1]) for line in ps.splitlines() if line.startswith(uid+' ') and line.split()[7]=='T']
 if stopped:shell('run-as com.winlator kill -CONT '+' '.join(map(str,stopped)))
 out=ROOT/'research/reports/vr-bridge-launch.log'
 with out.open('w',encoding='utf-8') as log:
  result=subprocess.run(base+['shell',f'run-as com.winlator files/myth-bridge/capture {pid} @mythvr {args.seconds*30} 33'],stdout=log,stderr=subprocess.STDOUT,timeout=args.seconds*2+30)
 if result.returncode:raise RuntimeError(f'Bridge failed; see {out}')
 print(f'Bridge capture finished; log: {out}')
if __name__=='__main__':main()
