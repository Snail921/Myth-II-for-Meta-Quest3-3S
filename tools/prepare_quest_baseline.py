"""Stage an unchanged original engine for a separate Wine/Box device experiment.

Does not install software, connect to a device, or claim native VR support.
The output contains private game assets and must not be published.
"""
import hashlib
import json
from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'research/runtime/myth-1.8.1'
OUTPUT = ROOT / 'build/quest-baseline'
EXPECTED_EXE = 'b89a2737d6bdfd5bdd8a2ad668b2b0d02976225a95ee7469a5c257144233b16c'
EXPECTED_FILM = 'a86059d6771859d15c5cf64346f3761154d81f294d56c35d0748458e41a71b3b'


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    exe = SOURCE / 'Myth II.exe'
    film = ROOT / 'research/fixtures/films/capture-01.film'
    if digest(exe) != EXPECTED_EXE or digest(film) != EXPECTED_FILM:
        raise ValueError('Original engine or baseline Film differs from reviewed fixture')
    # Do not silently overwrite a prior device experiment or its save files.
    if OUTPUT.exists():
        raise FileExistsError(f'Existing baseline retained: {OUTPUT}')
    game = OUTPUT / 'MythII'
    game.mkdir(parents=True)
    rows = []
    # Retain the complete tag/plugin sets, including their load-order metadata.
    # Start with fresh hardware settings instead of the desktop NVIDIA settings.
    names = ['Myth II.exe', 'SMACKW32.DLL', 'tags', 'plugins', 'local', 'cutscenes']
    for name in names:
        origin = SOURCE / name
        if not origin.exists():
            raise FileNotFoundError(origin)
        files = sorted(origin.rglob('*')) if origin.is_dir() else [origin]
        for source in files:
            if not source.is_file():
                continue
            if source.is_symlink():
                raise ValueError(f'Unexpected symlink: {source}')
            relative = source.relative_to(SOURCE)
            target = game / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source, target)
            source_hash = digest(source)
            if digest(target) != source_hash:
                raise ValueError(f'Copy verification failed: {relative}')
            rows.append({'path': relative.as_posix(), 'size': target.stat().st_size,
                         'sha256': source_hash})
    for name in ['preferences', 'recordings', 'saved games', 'plugins', 'local', 'cutscenes']:
        (game / name).mkdir(exist_ok=True)
    target = game / 'recordings/Codex Willow Creek baseline'
    shutil.copy2(film, target)
    if digest(target) != EXPECTED_FILM:
        raise ValueError('Film copy mismatch')
    rows.append({'path': target.relative_to(game).as_posix(),
                 'size': target.stat().st_size, 'sha256': EXPECTED_FILM})
    (game / 'Start Myth II.bat').write_bytes(
        b'@echo off\r\ncd /d "%~dp0"\r\n"Myth II.exe"\r\n')
    manifest = {
        'purpose': 'Original x86 engine compatibility baseline; not native ARM64 or VR',
        'source': str(SOURCE), 'files': rows,
        'total_copied_bytes': sum(row['size'] for row in rows),
        'hardware_preferences': 'Not copied; choose initial graphics settings in game',
        'film': {'map': 'l1  ', 'name': 'Willow Creek', 'difficulty': 2,
                 'seed': 178824227, 'gameplay': 2181, 'end_tick': 263},
        'device_execution': 'NOT RUN', 'public_distribution': False,
    }
    (OUTPUT / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'directory': str(OUTPUT), 'files': len(rows),
                      'bytes': manifest['total_copied_bytes'], 'verified': True}))


if __name__ == '__main__':
    main()
