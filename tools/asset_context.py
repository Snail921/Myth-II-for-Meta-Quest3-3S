"""Optional per-level build context; legacy single-map commands keep their defaults."""
import os,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
WORK=Path(os.environ['MYTH_LEVEL_BUILD']) if os.environ.get('MYTH_LEVEL_BUILD') else None
BASE=WORK/'scene' if WORK else ROOT/'build/scene-bridge'
OUT=WORK/'assets' if WORK else ROOT/'build/vr/assets'
FIXTURE=WORK/'decoded-map.json' if WORK else ROOT/'research/fixtures/maps/willow-creek/decoded-map.json'
REPORT=WORK/'reports' if WORK else ROOT/'research/reports'
def dimensions():
 d=json.loads(FIXTURE.read_text(encoding='utf8'));return d['width'],d['height']
