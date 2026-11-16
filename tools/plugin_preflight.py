"""Reject collection identities the current runtime cannot distinguish, before conversion."""
from level_assets import dependencies,raw_tag
from build_level_assets import collection_fields

def inspect(tags,mapid):
 deps,missing=dependencies(tags,mapid)
 missing=[(kind,tag) for kind,tag in missing if tag!='\u02c7'*4 and kind!='dmap']
 collections={tag:data for (kind,tag),data in deps.items() if kind=='.256' and int.from_bytes(data[128:132],'big')}
 for tag in ('fire','effe','buco','ebbf','mafi','guts','wate'):
  if tag not in collections and ('.256',tag) in tags:collections[tag]=raw_tag(tags,'.256',tag)
 keys={};collisions=[]
 for tag,data in sorted(collections.items()):
  key=collection_fields(data)
  if key in keys and keys[key][1]!=data:collisions.append([keys[key][0],tag])
  else:keys[key]=(tag,data)
 return {'missing_dependencies':missing,'legacy_collection_collisions':collisions,'collection_count':len(collections),'binding':'MVRCL002 original tag IDs (updated 1.8.5 APK required)','supported':not missing and len(collections)<=223}
