import json
from pathlib import Path
x=json.loads(Path('build/plugin-skybox-inventory.json').read_text())
scenes={
'The Healer':'Quiet temperate countryside at early morning, pale blue sky, elegant thin cirrus, gentle warm sun. A scholarly healer departs east after disturbing visions. Olive grassland and soft earthy haze. Hopeful with restrained unease.',
'The Warrior':'Ruewood woodland country under a fresh blue daytime sky, broad white cumulus building at the horizon, distant soft forest-green plains. A rescue mission as the Hive threatens peaceful forest inhabitants; calm light with gathering tension.',
'The Mage':'A haunted valley near Kyrand’s tomb, muted blue-grey daylight and high broken clouds with soft silvery light. Low olive-brown plains, still atmospheric haze, solemn mourning mood. Tavern meeting at sunset is a past event, not proof this mission occurs at sunset.',
'The Archer':'Exit from mountains into a shadowy marsh and Ruewood; cool overcast sky with a few muted blue breaks, diffuse pale light. Dark moss-green wet lowland fading into layered mist, no large mountains or enclosed tomb: the playable route goes into the valley beyond.',
'The Messenger':'Journey to the Cavanaugh beachhead, damp coastal evening with grey rain clouds and a narrow pale golden western gap. Low flat olive grass and slate-blue distant water dissolve into sea mist. Political intrigue and anticipation, no ships or buildings.',
'The Baron ':'Secret rendezvous at Cavanaugh in the evening, darkening blue-grey rainy coastal sky, faint amber twilight band under layered clouds, subdued wet brown-green plains and grey water in haze. No bright daytime sun.',
'The Chimera':'Approach to a sorceress stronghold in a cold hostile landscape, pale snow haze over muted grey ground, heavy steel-blue clouds, one soft cold break of light. Demonic invocation and final assault lend ominous scale without literal monsters or magical symbols.',
'The Fiend':'Final confrontation with the Fiend, vast brooding charcoal-blue storm canopy with restrained muted warm light below the clouds. Dark brown desolate plain and smoky atmospheric haze, dramatic tragic climax, no giant mountains, fireballs or monsters.',
"Crow's Bridge":'August morning in northern rural country, clear rich blue with loosely scattered white summer clouds and warm sun. Flat muted olive meadows, warm earth and light river haze. A village vigil before the war reaches it.',
'Training Map':'Welcoming temperate training ground, clear blue midday sky with soft white cumulus and fine high cloud, quiet olive-brown grassy plains, subtle distant haze.',
"A Traitor's Grave":'August on King’s Highway, bright late-summer sky with high wispy clouds and gentle sun. Low dry olive meadows merging into bluish distant haze, a tense pursuit in otherwise peaceful countryside.',
'The Siege of Madrigal':'Diversionary assault one hour after midnight in August, deep navy night, sparse faint stars through moving grey cloud, restrained moonlight. Dark desaturated green-brown plains and low river mist. No sunrise or artificial fires.',
'Homecoming':'September ruins of Covenant approached from Tyr, subdued autumn daylight, muted blue sky and thin grey veil, pale warm light. Dusty brown-olive plains with low haze, bleak abandoned-city atmosphere without drawing buildings.',
'Flight From Covenant':'Dawn escape from ruined Covenant as the Watcher fills the sky with dust and smoke. Pale peach-grey horizon beneath a cold blue-grey smoky sky, diffuse first sunlight, dark earthy plains disappearing into dust haze. No large flames.',
'Force Ten from Stoneheim':'November Plain of Scales before winter, cool blue-grey sky with wind-stretched clouds, low pale sun, muted ochre and olive flat plains in distance. A desperate mission to destroy a World Knot.',
'Bagrada':'November Cloudspine pass in hard snowfall; dense soft silver-grey overcast, cold diffuse illumination, low desaturated snowy terrain dissolving into white mist. No dramatic peaks, no painted foreground snowflakes; runtime provides snow.',
'Ambush at Devil’s Overlook':'Two days of heavy November snow at Bagrada, seamless leaden white-grey sky with layered cold cloud, icy blue haze and a flat snow-muted distant floor. Bitter cold and hushed ambush tension, no tall mountains or foreground flakes.',
'The Five Champions':'Night insertion into the rough Barrier desert in November. Deep clear indigo night, restrained stars and a small veiled moon, dry tan-grey plains extremely subdued under moonlight, broad horizon haze. No mountains or structures.',
'Out of the Barrier':'Escape across the Barrier desert, cool early daylight after a night rescue. Pale blue sky with thin ivory clouds and restrained low sun, dry sand-brown flat desert fading into haze; cautious hope and exhaustion.',
'Silvermines':'Late November evening infiltration of abandoned Silvermines, dusk blue overhead fading to a restrained dusty mauve horizon, thin cold clouds, grey-brown barren plains and soft mist. Ominous but readable, no buildings.',
'Shadow of the Mountain':'November volcanic eruption thirty miles away, sky choked with broad ash-grey clouds tinged faintly rusty amber from distant heat. Dark earth and warm haze at flat horizon, tense last stand. No nearby volcano, lava or painted falling ash.',
'Seven Gates':'December flooded pass warmed by continuing distant volcanic activity. Pale winter daylight filtered through grey ash clouds, muted amber-grey haze along low wet dark brown terrain. No snowstorm, no towering volcano.',
'Forest Heart':'May spring on the edge of Forest Heart; fresh blue sky with beautiful thick white clouds and gentle sunlight, muted moss and olive plains dissolving into humid forest haze. Peace threatened by Soulblighter, no foreground treetops.',
'Sons of Myrgard':'Late May near occupied dwarven homelands. Broad blue sky under slowly gathering cumulus, dry brown and olive plains, warm sun and distant dusty haze. Brave bittersweet homecoming, no buildings or peaks.',
'A Long Awaited Party':'Late May at Myrgard after a desperate stand, clear blue with streaked high cloud, stronger warm afternoon light over desaturated ochre-brown distant plain. Defiant and solemn, no figures or architecture.',
'The Road North':'June at the edge of the Dire Marsh, humid rain-heavy green-grey atmosphere, layered dark rain clouds with a faint silver break. Flat dark olive wetland merging into low fog, no foreground rain strokes.',
'Across the Gjol':'Midnight June river crossing and ambush in the poisoned Dire Marsh, dark navy rainclouds with very faint cold moon-glow, barely visible dark green wet plains and slate water under dense low mist. No sunset.',
'The Watcher':'After the midnight Gjol battle, rain-soaked Dire Marsh under a deeply overcast blue-grey sky, weak early light through cloud, dark moss-green flat terrain fading into mist. Grim retribution, no figures or stone giant.',
'River of Blood':'July outside ruined Trow Rhi’anon; bleak cold-looking daytime despite summer, grey-blue clouds above desaturated barren brown plains, diffuse pale sun. Menacing empty atmosphere, no buildings.',
'Pools of Iron':'Night before the dawn assault at Rhi’anon, indigo sky with thin clouds, one small bright comet with a delicate short tail high above, muted grey-brown barren plains and silver haze. The comet is explicitly in the journal; no extra moons.',
'The Last Battle':'Dawn final assault at Balor’s fortress in July. Dramatic cold blue upper sky and pale amber first sunlight beneath layered cloud, small faint comet still visible, dark barren earth dissolving into smoky haze. Tragic heroic scale without fortress or troops.',
'The Great Devoid':'Morning after Balor falls, somber luminous pale blue sky with shredded grey clouds and restrained dawn warmth, a very faint small comet, dusty grey-brown plain fading into haze. End-of-war dread and release, no giant chasm painted into sky.',
"Mazzarin's Demise VI":'The last defense of Mazzarin in the Age of Splendor against an army of the dead. Slate-blue gathering storm, pale gold break in cloud above a muted olive-brown distant plain. Restrained epic sorcery mood without visible magic or buildings; time/weather artistic interpretation.',
"Mazzarin's Demise -  Secret Level":'Secret survival battlefield associated with Mazzarin, dramatic dark violet-grey cloud masses with a silver-blue opening, low earthy desaturated plains in fog. Ominous magical-war atmosphere, no literal spell effects; time/weather artistic interpretation.'}
mp={
'fosg':'Fosgarach Ruillick battlefield, cool temperate grey-blue daylight, fine high cloud and layered white cumulus, muted grassy plains fading into soft low mist.',
'40de':'Dry desert battlefield, cloudless deep blue sky and a single fierce high sun, ochre-tan low flat desert merging into pale heat haze, no dunes taller than the horizon.',
'53mm':'Mudpit battlefield, dark wet brown low plains, heavy slate-grey rainclouds, a little silver light at the horizon, damp mist. Rain is supplied by the game, do not paint rain streaks.',
'20gz':'Ground Zero battlefield, brooding storm-grey canopy, muted green-brown muddy plains fading into rain haze, subtle brighter opening in clouds. No explosion or mushroom cloud.',
'57gr':'Graveyard countryside battlefield, atmospheric cool dusk-blue sky, wispy pale clouds and dim moonlight, dark muted olive earth under low mist. No gravestones or other nearby objects.',
'207c':'Carnage battlefield, crisp cool daylight with strong blue sky and sculpted white clouds, muted olive grass and grey-brown flats under gentle haze. Broad readable horizon.',
'20cr':'Borderlands battlefield, temperate blue sky with elegant stretched white cloud and soft afternoon sun, muted green-brown plains fading into bluish haze. No walls or buildings.',
'56ri':'Rest in Pieces battlefield, pale cool grey-blue overcast with thin brighter gaps, desaturated brown-green plains and low ground fog, quietly ominous.',
'40di':'Dire Marsh battlefield, dense green-grey humid cloud cover, diffuse cool daylight, flat dark moss-green wetland blending into fog and grey distant water.',
'keme':'Keep Across the River battlefield, blue sky with tall soft white clouds and warm afternoon light, low olive grassland and a subdued blue-grey water horizon in haze. No castle.',
'apph':'Appalachian Hot Springs battlefield, clear cool blue sky with broad delicate white clouds, pale sun, dark olive grassy flatlands and faint silvery steam haze near distant ground. No foreground steam or mountain silhouettes.',
'vVcm':'A Vivid Struggle battlefield, brilliant but natural blue sky with sweeping cirrus and bright white clouds, muted earthy ochre and green flat ground, delicate warm sunlight. Keep terrain restrained not neon.',
'86mf':'Cloudspine battlefield, cold clear high-altitude blue sky and wispy ivory cloud, pale sunlight, cool grey-brown flat distant ground dissolving into mist. No high mountains.',
'89sn':'Winter battlefield, overcast snow-grey sky with soft blue breaks, white-grey snow-covered distant flat ground merging into frost haze. No falling flakes baked into texture.'}
base='Use case: photorealistic-natural. Production Myth II VR skybox. Exact 2:1 equirectangular 360x180 full-sphere texture, 2048x1024. Horizon precisely v=0.5, top hemisphere entirely sky; seamless left/right longitude wrap, naturally converging zenith/nadir. Lower hemisphere low-detail flat plains in atmospheric haze, subdued enough to blend into classic RTS ground. No mountains, sharp ridges, nearby objects, buildings, people, text, watermark, borders, UI, or lens flare. One restrained sun or moon at most, around u=.62 when appropriate. Beautiful natural cloud detail, cinematic restraint. Scene: '
jobs={};excluded=[]
for a in x:
 for e in a['entries']:
  n=e['name']
  if n in ['Heart of the Stone','The Smiths of Muirthemne']:
   excluded.append(dict(archive=a['name'],name=n,key=e['key'],reason='Journal explicitly places mission inside the Tain, vast underground caverns.'));continue
  if n in scenes:ident=e['key']+'-'+a['name'];theme=scenes[n]
  else:
   land=e['header']['landscape_collection_tag'];weather=e['header']['particle_system_tag'];ident='multiplayer-'+land+('-dry' if land=='53mm' and weather=='ÿ'*4 else '');theme=mp[land]
   if ident.endswith('-dry'):theme=theme.replace('rainclouds','overcast clouds').replace('Rain is supplied by the game, do not paint rain streaks.','No rain.')
  if ident not in jobs:jobs[ident]=dict(id=ident,theme=theme,prompt=base+theme,levels=[])
  jobs[ident]['levels'].append(dict(archive=a['name'],name=n,key=e['key'],journal=e['journal'],lighting=e['header']['mesh_lighting_tag'],weather=e['header']['particle_system_tag']))
plan=dict(jobs=list(jobs.values()),excluded=excluded,entries=sum(len(a['entries']) for a in x))
Path('build/plugin-skybox-plan.json').write_text(json.dumps(plan,ensure_ascii=False,indent=2),encoding='utf8')
print('Designs',len(jobs),'outdoor entries',sum(len(j['levels']) for j in jobs.values()),'excluded',len(excluded))
