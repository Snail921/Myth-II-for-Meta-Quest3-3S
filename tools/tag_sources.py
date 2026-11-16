"""Installed official foundation tags plus PATCH archives (not optional plugins)."""
import json,re,os
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]

def inventory():
    selected=os.environ.get('MYTH_TAG_PROFILE')
    if selected:
        profile=json.loads(Path(selected).read_text(encoding='utf8'))
        if profile.get('version')!=1:raise ValueError('Unsupported tag source profile')
        return {(kind,tag):(path,record) for kind,tag,path,record in profile['tags']}

    archives=json.loads((ROOT/'research/reports/tag-inventory.json').read_text(encoding='utf8'))
    foundations=[a for a in archives if a['archive_type']=='FOUNDATION']
    patches=[a for a in archives if a['archive_type']=='PATCH']
    patches.sort(key=lambda a: tuple(int(x) for x in re.findall(r'\d+',a['path'].split('plugins')[-1])))
    tags={}
    for archive in foundations+patches:
        for tag in archive['tags']:tags[tag['type'],tag['id']]=(archive['path'],tag)
    return tags
