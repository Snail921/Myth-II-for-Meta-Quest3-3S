"""Per-operation dialog locations and lightweight collection previews."""
import json
import os
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
SETTINGS = Path(os.environ.get('APPDATA', Path.home()))/'MythIIVR/sprite-editor-paths.json'


class DialogPaths:
    def __init__(self, settings=SETTINGS):
        self.settings = Path(settings)
        try:
            data = json.loads(self.settings.read_text(encoding='utf8'))
            self.paths = data if isinstance(data, dict) else {}
        except (OSError, ValueError):
            self.paths = {}

    def initial(self, operation):
        saved = self.paths.get(operation)
        if isinstance(saved, str) and Path(saved).is_dir():
            return saved
        return str(ROOT)

    def remember(self, operation, selected, directory=False):
        if not selected:
            return
        folder = Path(selected).resolve()
        if not directory:
            folder = folder.parent
        # Merge other editor windows' operations instead of overwriting them.
        latest = DialogPaths(self.settings).paths
        latest[operation] = str(folder)
        self.settings.parent.mkdir(parents=True, exist_ok=True)
        temp = self.settings.with_suffix('.tmp')
        temp.write_text(json.dumps(latest, ensure_ascii=False, indent=2), encoding='utf8')
        temp.replace(self.settings)
        self.paths = latest


def collections(folder):
    return sorted((p for p in Path(folder).iterdir() if p.is_file() and p.suffix.lower() in ('.collection', '.256', '.bin')), key=lambda p: p.name.casefold())


def thumbnail(path):
    from prepare_vr_assets import decode, resolved_instance
    images, instances, sequences = decode(Path(path).read_bytes())
    first = None
    for flags, scale, rows in sequences:
        for row in rows:
            for direction in range(len(row)):
                bitmap = resolved_instance(images, instances, flags, row, direction)[3]
                if bitmap >= 0:
                    first = bitmap if first is None else min(first, bitmap)
    if first is None:
        raise ValueError('Collection has no sprite frames')
    w, h, pixels = images[first]
    image = Image.frombytes('RGBA', (w, h), bytes(pixels))
    image.thumbnail((120, 110))
    return image


def exported_collection_names(folder):
    """Case-insensitive collection stems with exports; None means unavailable.

    This is an existence indicator, not a material completeness/validity check.
    Match the default Project.collection -> export_runtime basename exactly.
    """
    if not folder:
        return None
    try:
        return {p.stem.casefold() for p in Path(folder).iterdir()
                if p.is_file() and p.suffix.casefold() == '.mvrnm'}
    except OSError:
        return None
