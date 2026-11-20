from pathlib import Path
import re,json,shutil
root=Path('build/integration-185');root.mkdir(parents=True,exist_ok=True)
# This is explicitly a reduced integration candidate. No guessed command entrypoints.
mapping={0x529bd4:0x531618,0x67ec00:0x6bee40,0x67ec24:0x6bee64,0x67383c:0x6a7b2c,0x529f20:0x532158,0x529f4e:0x532182,0x529f54:0x532188,0x52a9c0:0x532e3c,0x52a9ee:0x532e66,0x52a9f4:0x532e6c,0x553fd8:0x67ddb0,0x553fda:0x67ddb2,0x553fdc:0x67ddb4,0x52ae68:0x5332dc,0x52ae6c:0x5332e0,0x52b4d4:0x533e20,0x52a6f4:0x532118,0x554410:0x67e1e8,0x561860:0x68f650,0x67ec06:0x6bee46,0x67ec08:0x6bee48,0x67ec10:0x6bee50,0x553f20:0x67dcf8,0x553f22:0x67dcfa,0x54e43c:0x66b364,0x54e440:0x66b368,0x54e444:0x66b36c,0x529d18:0x5319a4,0x529ec4:0x531960,0x465f3c:0x4c9530,0x465107:0x4c86d7,0x468ae7:0x4cc2e2}
def relocate(s):return re.sub(r'0x[0-9a-fA-F]+',lambda m:hex(mapping.get(int(m[0],16),int(m[0],16))),s)
def body(s,signature,replacement):
 start=s.index(signature);b=s.index('{',start);depth=1;e=b+1
 while depth:
  depth+=(s[e]=='{')-(s[e]=='}');e+=1
 return s[:b]+'{'+replacement+'}'+s[e:]
def region(s,start,end,new=''):
 a=s.index(start);b=s.index(end,a);return s[:a]+new+s[b:]
p=Path('src/engine/profile_434.h').read_text(encoding='utf-8-sig');p=region(p,'#if MYTH_ENGINE_BUILD != 434','#define MVR_MESH');p=p.replace('#define MYTH_ENGINE_BUILD 434','#define MYTH_ENGINE_BUILD 471');(root/'profile.h').write_text(relocate(p))
for name in ['multiplayer_hud.h','collection_access.h','selection_eligibility.h','surface_capture.h','light_capture.h','particle_capture.h','shockwave_capture.h','weather_capture.h']:
 shutil.copyfile(Path('src/bridge')/name,root/name)
shutil.copyfile('src/engine/actor_wire.h',root/'actor_wire.h')
s=Path('src/bridge/capture.c').read_text(encoding='utf-8-sig');s=s.replace('#include "../engine/profile_434.h"','#include "profile.h"\n#include "actor_wire.h"\n#define MVR_COLLECTION_RELATIVE 1')
s=s.replace('#include "selection_info.h"','#define SELECTION_INFO_BYTES 1828\nstatic void collect_selection_info(const unsigned char*a,unsigned n,const unsigned char*s,unsigned char*out){(void)a;(void)n;memset(out,0,SELECTION_INFO_BYTES);unsigned count=u16(s);memcpy(out,&count,4);}')
s=body(s,'static uint32_t collect_models',s[s.index('static uint32_t collect_models'):].split('{',1)[1].split('\n}',1)[0].replace('header+46','header+42').replace('header+52','header+48').replace('index*64,definition,64','index*32,definition,32'))
s=s.replace('read_exact(0x46bf56,anchor,8);','read_exact(0x47b31c,anchor,4);').replace('const unsigned char expected[8]={0x85,0xc0,0xa3,0x3c,0x38,0x67,0x00,0x75};','const unsigned char expected[4]={0x55,0x8b,0xec,0xb8};').replace('memcmp(anchor,expected,8)','memcmp(anchor,expected,4)')
s=s.replace('uint32_t ac=u16(ap+46),oc=u16(op+46);','uint32_t ac=u16(ap+42),oc=u16(op+42);').replace('u16(ap+24)!=216','u16(ap+24)!=220')
s=s.replace('read_exact(u32(ap+52),actors,ac*216);read_exact(u32(op+52),objects,oc*80);','unsigned char rawActors[400*220];\n   if(ac){read_exact(u32(ap+48),rawActors,ac*220);if(!myth_actor_wire(actors,400*216,rawActors,ac*220,ac,471))return 3;}if(oc)read_exact(u32(op+48),objects,oc*80);')
s=s.replace('surface_collect(&surface,surface_pending,tick,ns());','memset(surface_pending,0,offsetof(SurfacePending,data)); /* not yet ported */')
s=region(s,'   unsigned light_bytes=collect_lights','   read_exact(MVR_ACTOR_POOL','   unsigned light_bytes=4,particle_count=0;memset(light_data,0,4); /* FX remain disabled in this candidate. */\n')
s=region(s,'   uint32_t sniff=word','   ok=ok&&fwrite(camera_info','   /* Script camera bridge remains disabled in this candidate. */\n')
s=s.replace('u16(ap+48),u16(op+48)','u16(ap+44),u16(op+44)')
s=relocate(s);(root/'capture.c').write_text(s,encoding='utf8')
# Generate dispatcher from existing route/selection logic; remove all unported paths entirely.
s=Path('src/input/game_hook.c').read_text(encoding='utf-8-sig').replace('#include "sound_range.h"','').replace('#include "camera_bridge.h"','')
s=region(s,'extern I can_pickup','static U execute')
s=region(s,' if(c->op==18)',' // Reject menus')
s=region(s,' if(c->op==16||c->op==17)',' if(c->op==14)', ' if(c->op==15)return preview_formation(c);\n')
s=region(s,'  if(c->op==4&&controlled_selection()','  // Original selection', '  if(c->op==4&&!can_control(slot))return 17;\n')
s=region(s,' if(c->op>=5&&c->op<=8)',' return 17;', ' if(c->op==8){if(!controlled_selection())return 18;if(c->id>9)return 16;((void(*)(I,I))0x468ae7)((I)c->id,1);return 1;}\n')
s=s.replace('*(U*)0x46bf56!=0x3ca3c085','*(U*)0x47b31c!=0xb8ec8b55')
s=s.replace(' // Reject menus,',' if(!(c->op==1||c->op==2||c->op==3||c->op==4||c->op==8||(c->op>=9&&c->op<=11)||(c->op>=13&&c->op<=15)))return 17;\n // Reject menus,')
s=re.sub(r'\*216\b','*220',s)
s=relocate(s);(root/'game_hook.c').write_text(s,encoding='utf8')
shutil.copyfile('src/input/win_min.h',root/'win_min.h');shutil.copyfile('src/input/install_hook.c',root/'install_hook.c')
# Keep only independently inspected unchanged ABI adapters; movement/formation use tested new ABI.
asm=Path('src/input/game_calls.S').read_text(encoding='utf-8-sig');sections=re.split(r'(?=\.globl )',asm);keep=['select_slot','can_control','voice_selected','current_formation','terrain_kind']
asm='.text\n'+''.join(p for p in sections if any(p.startswith('.globl _'+n+'\n') for n in keep))
asm=relocate(asm)
for a,b in {0x467707:0x4cad8f,0x43b7a4:0x482b5e,0x43e3e7:0x4858c5,0x468e13:0x4cc617,0x4360e6:0x472c5f}.items():asm=asm.replace(hex(a),hex(b))
asm+='\n'+Path('src/input/engine185/game_calls.S').read_text(encoding='utf-8-sig');(root/'game_calls.S').write_text(asm)
(root/'movement.c').write_text('''#include "win_min.h"
extern void movement185(const US*,U,const US*,U,const I*);
extern void formation185(const void*,I,const US*,I,const I*,I*,US*,I);
extern I current_formation(void);
void movement_options(const void*opts,I n,const I*points){movement185(opts,*(US*)0x67ddb0,(US*)0x67ddb4,n,points);}
void route_selected(I n,const I*p){I f=(short)current_formation();US opts[4]={f<0?1:8,(US)f,0,0};movement_options(opts,n,p);}
void move_selected(I x,I y){I p[2]={x,y};route_selected(1,p);}
void calculate_formation(const void*a,I b,const US*c,I d,const I*e,I*f,US*g,I h){formation185(a,b,c,d,e,f,g,h);}
''')
print('Generated isolated 471b integration sources; unsupported operations reject with status 17.')
# App source is generated into an isolated staging directory, never overwriting 434.
java=root/'java';java.mkdir(exist_ok=True)
for f in Path('src/standalone').glob('*.java'):shutil.copyfile(f,java/f.name)
shutil.copyfile('src/standalone185/Engine185Data.java',java/'Engine185Data.java')
s=(java/'EngineProfile.java').read_text(encoding='utf-8-sig');s=relocate(s).replace('b89a2737d6bdfd5bdd8a2ad668b2b0d02976225a95ee7469a5c257144233b16c','aa68153d987bf8a17c30bf8c7711d3b5e0a291a6cd1f8630446496715539181c');(java/'EngineProfile.java').write_text(s,encoding='utf8')
s=(java/'EngineProfile.java').read_text(encoding='utf8').replace('ACTIVE_ARCHIVES = 0L','ACTIVE_ARCHIVES = 0x6a1f50L').replace('COLLECTION_IDS = 0L','COLLECTION_IDS = 0x533e24L').replace('COLLECTION_COUNT = 0L','COLLECTION_COUNT = 0x533e14L');(java/'EngineProfile.java').write_text(s,encoding='utf8')
s=(java/'StandaloneBridge.java').read_text(encoding='utf-8-sig');s=relocate(s).replace('slot*216','slot*220')
s=s.replace('long refs=Integer.toUnsignedLong(intAt(f,pool+runtime*320+140));','long refs=Integer.toUnsignedLong(intAt(f,pool+runtime*320+8))+Integer.toUnsignedLong(intAt(f,pool+runtime*320+132));')
s=s.replace('base=c.getFilesDir().getParentFile();','base=c.getFilesDir().getParentFile();\n        Engine185Data.verify();')
(java/'StandaloneBridge.java').write_text(s,encoding='utf8')
s=(java/'StandaloneActivity.java').read_text(encoding='utf-8-sig');s=s.replace('private volatile boolean importing;','private volatile boolean importing,starting;')
s=s.replace('private void startEngine(){','private void startEngine(){\n        if(starting)return;starting=true;show(UiLanguage.english(this)?"Checking external game data…":"外部の原作データを確認中…");\n        new Thread(()->{try{Engine185Data.prepare(this);runOnUiThread(()->{starting=false;launch185();});}catch(Exception e){runOnUiThread(()->{starting=false;show("1.8.5 preparation failed: "+e);});}},"Prepare185").start();\n    }\n    private void launch185(){')
s=s.replace('/Download/MythII-Standalone/Myth II.exe','/Download/MythII-Standalone185/Myth II.exe')
(java/'StandaloneActivity.java').write_text(s,encoding='utf8')
print('Generated 1.8.5 sources; original game files are external.')
# Test renderer never queues unported background commands (would cancel queued moves on failure).
vr=root/'vr';vr.mkdir(exist_ok=True)
for f in Path('src/vr').iterdir():
 if f.is_file():shutil.copyfile(f,vr/f.name)
s=(vr/'touch_input.h').read_text(encoding='utf8')
s=s.replace('static bool command(unsigned op,unsigned id=0,unsigned flags=0,int x=0,int y=0,int z=0){','static bool command(unsigned op,unsigned id=0,unsigned flags=0,int x=0,int y=0,int z=0){\n if(!(op==1||op==2||op==3||op==4||op==8||(op>=9&&op<=11)||(op>=13&&op<=15)))return false;')
(vr/'touch_input.h').write_text(s,encoding='utf8')
for p in java.glob('*.java'):p.write_text(p.read_text(encoding='utf-8-sig'),encoding='utf8')
