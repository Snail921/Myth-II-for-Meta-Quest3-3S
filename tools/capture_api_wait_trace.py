"""Wait for the next VR scene, then record 60 seconds of scheduler/API evidence."""
import argparse, pathlib, subprocess, datetime
p=argparse.ArgumentParser();p.add_argument('--adb',required=True);p.add_argument('--serial',required=True);p.add_argument('--output',default='build');a=p.parse_args()
base=[a.adb,'-s',a.serial]
config=pathlib.Path(__file__).with_name('perfetto-api-waits.pbtxt')
remote_config='/data/misc/perfetto-configs/myth-api-waits.pbtxt'
subprocess.run(base+['push',str(config),remote_config],check=True)
print('Waiting for next Benchmark initial camera event...',flush=True)
watch=subprocess.Popen(base+['logcat','-v','threadtime','-T','1','MythVR:I','*:S'],stdout=subprocess.PIPE,encoding='utf-8',errors='replace')
try:
 for line in watch.stdout:
  if 'Benchmark initial camera=' in line:
   print(line.strip(),flush=True);break
 else:raise RuntimeError('logcat ended before scene start')
finally:
 watch.terminate();watch.wait()
stamp=datetime.datetime.now().strftime('%Y%m%d-%H%M%S')
remote='/data/misc/perfetto-traces/myth-api-'+stamp+'.pftrace'
out=pathlib.Path(a.output);out.mkdir(parents=True,exist_ok=True)
print('Recording scheduler/API trace for 60 seconds...',flush=True)
subprocess.run(base+['shell','perfetto','--txt','-c',remote_config,'-o',remote],check=True)
local=out/pathlib.PurePosixPath(remote).name
subprocess.run(base+['pull',remote,str(local)],check=True)
if local.stat().st_size<1024:raise RuntimeError('Trace unexpectedly small')
print('Trace saved: '+str(local.resolve()),flush=True)
