"""Read selected original-engine globals from the authorized Quest debug runtime."""
import subprocess,json,re,datetime,sys
from pathlib import Path
base=['C:/Users/gen/AppData/Local/Android/Sdk/platform-tools/adb.exe','-s','340YC10G8T047C']
ps=subprocess.check_output(base+['shell','ps','-A'],text=True)
pids=[int(line.split()[1]) for line in ps.splitlines() if line.endswith('Myth II.exe')]
if len(pids)!=1: raise RuntimeError('Expected exactly one Myth II process')
pid=pids[0]
def words(address,count=1):
 cmd=f'run-as com.winlator sh -c "dd if=/proc/{pid}/mem bs=4 count={count} skip={address} iflag=skip_bytes status=none | od -An -v -tu4"'
 r=subprocess.run(base+['shell',cmd],capture_output=True,text=True,check=True)
 if r.stderr.strip(): raise RuntimeError(r.stderr)
 values=[int(x) for x in r.stdout.split()]
 if len(values)!=count: raise RuntimeError('Short process read')
 return values
if words(0x400000)[0]&0xffff!=0x5a4d: raise RuntimeError('Expected PE mapping absent')
data={'observed_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'pid':pid,'method':'read-only /proc/PID/mem through debug app UID','tick_address':'0x67ec24','tick':words(0x67ec24)[0],'primary_rng_address':'0x67383c','primary_rng':words(0x67383c)[0],'limitation':'Not an atomic snapshot or a cross-platform determinism proof'}
out=Path(sys.argv[1]) if len(sys.argv)>1 else Path('research/reports/quest-state-samples.jsonl')
with out.open('a',encoding='utf-8') as f: f.write(json.dumps(data)+'\n')
print(json.dumps(data))
