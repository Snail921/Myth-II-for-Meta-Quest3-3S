"""Generate audited Build471b full-feature integration sources, isolated from 434.
See version-185-mesh-xrefs and command recovery reports for binary evidence.
"""
import runpy,re,shutil
from pathlib import Path
ctx=runpy.run_path('tools/prepare_185_integration.py');root=ctx['root'];mapping=ctx['mapping'];body=ctx['body'];region=ctx['region']
mapping.update({0x44c33a:0x48e795,0x67ec0e:0x6bee4e,0x529518:0x5304c4,0x52962c:0x5305c8,0x528b14:0x52d020,0x528ad0:0x52cfd0,0x528f9c:0x52ebf8,0x529638:0x5307fc,0x529678:0x530834,0x5297cc:0x5307f0,0x529cd8:0x5317b8,0x529bf0:0x5316cc,0x529cbc:0x5316c0,0x52ac38:0x5331d4,0x54f928:0x66c9ec,0x544348:0x6a61c4,0x67ec62:0x6beeb2,0x67ec04:0x6bee44,0x561870:0x68f6b8,0x561872:0x68f6ba,0x544768:0x540a34,0x657c14:0x638bf4,0x657c18:0x638bf8,0x657c1e:0x638bfe,0x657c28:0x638c08,0x657c2a:0x638c0a,0x466ec4:0x4ca522,0x44fda8:0x49d891,0x4a03c7:0x46b51c,0x4a042c:0x46b581,0x4321f1:0x45dbc1,0x4321f6:0x45dbc6,0x4a04e5:0x46b629,0x46a80a:0x4d0117,0x450f61:0x49edf7,0x4cc603:0x4d1597,0x4cc608:0x4d159c,0x46bfa2:0x4d8f96,0x467707:0x4cad8f,0x43b7a4:0x482b5e,0x43e3e7:0x4858c5,0x468e13:0x4cc617,0x4360e6:0x472c5f,0x43b3c8:0x482774,0x446948:0x47b614,0x446a17:0x47b6c5,0x466be9:0x4ca258,0x46835b:0x4cbb7f})
def reloc(s):return re.sub(r'0x[0-9a-fA-F]+',lambda m:hex(mapping.get(int(m[0],16),int(m[0],16))),s)
def pool(s):
 for name in ['ph','eh','effects','pool','header']:
  s=s.replace(name+'+46',name+'+42').replace(name+'+52',name+'+48')
 return s
for name in ['light_capture.h','particle_capture.h','shockwave_capture.h','weather_capture.h','lightning_capture.h']:
 (root/name).write_text(pool(reloc((Path('src/bridge')/name).read_text())),encoding='utf8')
s=Path('src/bridge/collection_access.h').read_text().replace('memcpy(raw+192,raw+24,4);','uint32_t maps=u32(raw+148);if(maps>0x10000000u||blob>UINT32_MAX-maps)return 0;maps+=blob;memcpy(raw+156,&maps,4);\n memcpy(raw+192,raw+24,4);')
(root/'collection_access.h').write_text(s)
s=Path('src/bridge/surface_capture.h').read_text().replace('mesh+1000','mesh+654').replace('uint32_t rp=u32(head+108);','uint32_t rp=u32(head+8)+u32(head+100);').replace('ref*128+96','ref*128+104').replace('if(!optional_read(cp+108,verify,4)||u32(verify)!=rp)return;','if(!optional_read(cp+8,verify,4)||u32(verify)!=u32(head+8))return;\n if(!optional_read(cp+100,verify,4)||u32(verify)!=u32(head+100))return;')
(root/'surface_capture.h').write_text(reloc(s))
s=Path('src/bridge/selection_info.h').read_text().replace('u16(header+0x54)','u16(header+0x44)').replace('u32(header+0x48)','u32(header+0x54)')
s=body(s,'static void info_level_title','''
 unsigned char field[2];char name[192]={0},difficulty[64]={0};
 uint32_t base=word(0x531618);
 if(base&&optional_read(base+0x28a,field,2))info_string((int16_t)u16(field),0,name,sizeof(name));
 info_string(info_string_tag(0x696e646c),(int16_t)(word(0x6bee44)&65535),difficulty,sizeof(difficulty));
 if(name[0]&&difficulty[0])snprintf(out,capacity,"%s  (%s)",name,difficulty);else snprintf(out,capacity,"%s",name);
''')
(root/'selection_info.h').write_text(reloc(s))
s=Path('src/bridge/capture.c').read_text().replace('#include "../engine/profile_434.h"','#include "profile.h"\n#include "actor_wire.h"\n#define MVR_COLLECTION_RELATIVE 1')
s=pool(s).replace('definitions+index*64,definition,64','definitions+index*32,definition,32')
s=s.replace('read_exact(0x46bf56,anchor,8);','read_exact(0x47b31c,anchor,4);').replace('const unsigned char expected[8]={0x85,0xc0,0xa3,0x3c,0x38,0x67,0x00,0x75};','const unsigned char expected[4]={0x55,0x8b,0xec,0xb8};').replace('memcmp(anchor,expected,8)','memcmp(anchor,expected,4)')
s=s.replace('uint32_t ac=u16(ap+46),oc=u16(op+46);','uint32_t ac=u16(ap+42),oc=u16(op+42);').replace('u16(ap+24)!=216','u16(ap+24)!=220')
s=s.replace('read_exact(u32(ap+52),actors,ac*216);read_exact(u32(op+52),objects,oc*80);','unsigned char rawActors[400*220];\n   if(ac){read_exact(u32(ap+48),rawActors,ac*220);if(!myth_actor_wire(actors,400*216,rawActors,ac*220,ac,471))return 3;}if(oc)read_exact(u32(op+48),objects,oc*80);').replace('u16(ap+48),u16(op+48)','u16(ap+44),u16(op+44)')
(root/'capture.c').write_text(reloc(s))
for name in ['sound_range.h','camera_bridge.h']:(root/name).write_text(reloc((Path('src/input')/name).read_text()))
s=Path('src/input/game_hook.c').read_text();s=s.replace('*(U*)0x46bf56!=0x3ca3c085','*(U*)0x47b31c!=0xb8ec8b55')
s=s.replace('((void(*)(void*,I,U,I,I))0x467c3b)((void*)0x553fd8,slot,flags,0,0);','attack185(slot,flags);')
s=s.replace('static U lastSeq;','extern void attack185(I slot,U flags);\nstatic U lastSeq;')
s=re.sub(r'\*216\b','*220',s);(root/'game_hook.c').write_text(reloc(s))
asm=Path('src/input/game_calls.S').read_text();sections=re.split(r'(?=\.globl )',asm);drop=['move_selected','route_selected','calculate_formation','movement_options','register_group','move_actor']
asm='.text\n'+''.join(p for p in sections if p.startswith('.globl ') and not any(p.startswith('.globl _'+n+'\n') for n in drop))
asm=reloc(asm)+'\n'+Path('src/input/engine185/game_calls.S').read_text()+'''
.globl _register_group
_register_group:
 movl $0x4cba6f,%eax
 jmp *%eax
.globl _attack185
_attack185:
 pushl %ebp
 movl %esp,%ebp
 pushl $0
 pushl $0
 pushl 12(%ebp)
 pushl 8(%ebp)
 movl $0x67ddb0,%eax
 movl $0x4cb3f4,%ecx
 call *%ecx
 leave
 ret
'''
(root/'game_calls.S').write_text(asm)
# No preview command suppression; unsupported addresses are absent from executable paths.
shutil.copyfile('src/vr/touch_input.h',root/'vr/touch_input.h')
s=(root/'java/StandaloneActivity.java').read_text(encoding='utf8').replace('VR integration preview','VR integration candidate').replace('選択・移動・隊形・経路の検証版。戦闘命令、FX、情報欄、音声範囲・シナリオカメラ同期は未対応。','全機能接続の検証版。実機での動作確認をお願いします。')
(root/'java/StandaloneActivity.java').write_text(s,encoding='utf8')
print('Full 471b connection sources generated; runtime verification still required.')

s=(root/"java/DetailTextures.java").read_text(encoding="utf8").replace("/MythII-Standalone/plugins", "/MythII-Standalone185/plugins")
(root/"java/DetailTextures.java").write_text(s,encoding="utf8")

# External-only game directory shared by engine and HD loaders.
for p in (root/'java').glob('*.java'):
 text=p.read_text(encoding='utf-8-sig').replace('/storage/emulated/0/Download/MythII-Standalone185','/storage/emulated/0/Android/data/org.mythvr.standalone/files/Game')
 p.write_text(text,encoding='utf8')

# Co-op roster/DETACH use independently verified Build 471b layouts.
for name,source in [('coop_transfer.h','src/input/coop_transfer.h'),('coop_roster.h','src/bridge/coop_roster.h')]:
 shutil.copyfile(source,root/name)
for name in ['game_hook.c','capture.c']:
 p=root/name;p.write_text('#define MYTH_COOP_471 1\n'+p.read_text(),encoding='utf8')
