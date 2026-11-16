from build_progress import tracked
"""Original particle sprite library, matched by collection metadata (not runtime slots)."""
import json, struct
from pathlib import Path
from prepare_vr_assets import decode, resolved_instance, u
ROOT=Path(__file__).resolve().parents[1]
from asset_context import BASE,OUT,FIXTURE,REPORT,dimensions,WORK
def signature(values):
    h=2166136261
    for v in values:
        for byte in struct.pack('<I',v): h=((h^byte)*16777619)&0xffffffff
    return h
def main():
    scene=json.loads((BASE/'scene.json').read_text(encoding='utf8'))
    raws=[(a['name'],a['runtime_index'],(BASE/a['file']).read_bytes()) for a in scene['assets'] if not a.get('model_material',False)]
    # Runtime signature 0x240a1862 was received but missing during combat.
    # guts contains the original blood-puff and flying-fragment sequences.
    wanted={'fire','effe','buco','ebbf','mafi','guts','wate'}
    from tag_sources import inventory
    for (kind,tag),(path,t) in inventory().items():
        if not WORK and kind=='.256' and tag in wanted:
            with (ROOT/path).open('rb') as f:f.seek(t['offset']);raws.append((t['name'],0,f.read(t['size'])))
    textures=[];indices=[];quads=[];seen={};stats=[]
    for name,canonical,raw in tracked(raws,'パーティクル素材を変換'):
        sig=canonical if WORK else signature([u(raw,o) for o in (64,96,112,128,252)])
        if sig in seen:
            assert seen[sig]==raw, 'Ambiguous collection signature: '+name
            continue
        seen[sig]=raw;ims,instances,seq=decode(raw);start=len(textures);textures.extend(ims);indices.extend(decode(raw,palette_indices=True)[0])
        for si,(flags,scale,refs) in enumerate(seq):
            for fi,row in enumerate(refs):
                for vi in range(len(row)):
                    fl,rx,ry,bi=resolved_instance(ims,instances,flags,row,vi)
                    if bi<0:continue
                    w,h,_=ims[bi]
                    quads.append((sig,si,fi,vi,len(row),flags,fl,start+bi,float(rx),float(ry),scale,float(w),float(h)))
        stats.append({'name':name,'signature':sig,'textures':len(ims),'sequences':len(seq)})
    out=OUT/'particles.pack'
    with out.open('wb') as f:
        f.write((b'MYTHFX03' if WORK else b'MYTHFX02')+struct.pack('<II',len(textures),len(quads)))
        for (w,h,p),(_,_,ix) in zip(textures,indices):f.write(struct.pack('<II',w,h)+p+ix[0::4])
        for q in quads:f.write(struct.pack('<8I5f',*q))
    result={'bytes':out.stat().st_size,'textures':len(textures),'quads':len(quads),'collections':stats}
    (REPORT/'particle-assets.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
    print(json.dumps(result))
if __name__=='__main__':main()
