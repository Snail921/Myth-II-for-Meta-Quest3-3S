"""Validated, transactional deployment of external sprite normal packs."""
import json
import shlex
import struct
import uuid
from pathlib import Path
from build_progress import PREFIX
from plugin_manager_backend import PACKAGE, digest

REMOTE = '/sdcard/Android/data/' + PACKAGE + '/files/SpriteMaterials'
LIMIT = 64 * 1024 * 1024  # Per pack, not the combined installed library.


def validate(path):
    size = path.stat().st_size
    if not 12 <= size <= LIMIT:
        raise ValueError('Invalid material pack size: ' + path.name)
    with path.open('rb') as f:
        magic = f.read(8)
        if magic not in (b'MVRNORM1', b'MVRNORM2'):
            raise ValueError('Invalid material pack format: ' + path.name)
        count, = struct.unpack('<I', f.read(4))
        if not 1 <= count <= 10000:
            raise ValueError('Invalid material frame count: ' + path.name)
        for _ in range(count):
            header = f.read(8)
            if len(header) != 8:
                raise ValueError('Truncated material pack: ' + path.name)
            w, h = struct.unpack('<II', header)
            if not (1 <= w <= 2048 and 1 <= h <= 2048) or f.tell() + (32+w*h*4 if magic==b'MVRNORM2' else w*h*8) > size:
                raise ValueError('Invalid material image: ' + path.name)
            f.seek(32+w*h*4 if magic==b'MVRNORM2' else w*h*8, 1)
        if f.tell() != size:
            raise ValueError('Unexpected material data: ' + path.name)
    return size


def deploy(folder, device):
    folder = Path(folder).resolve()
    if not folder.is_dir():
        raise ValueError('ノーマルマップのフォルダを指定してください。')
    files = sorted((p for p in folder.iterdir() if p.is_file() and p.suffix.lower() == '.mvrnm'), key=lambda p: p.name.casefold())
    if not files:
        raise ValueError('指定フォルダ直下に.mvrnmファイルがありません。')
    names = [p.stem + '.mvrnm' for p in files]
    if len(set(n.casefold() for n in names)) != len(names) or any(any(ord(c) < 32 for c in n) for n in names):
        raise ValueError('Duplicate or invalid material file names.')
    sizes = {}
    hashes = {}

    def progress(stage, current, item=''):
        device.log(PREFIX + json.dumps(dict(stage=stage, current=current, total=len(files), item=item)))

    for i, (p, name) in enumerate(zip(files, names)):
        progress('ノーマルパックを検査', i, p.name)
        sizes[name] = validate(p)
        hashes[name] = digest(p)
    progress('ノーマルパックを検査', len(files))
    device.ready()

    def shell(cmd):
        return device.call('shell', cmd, timeout=120)

    q = shlex.quote
    shell('mkdir -p ' + q(REMOTE))
    # Keep existing files and staging together until verification succeeds.
    free = int(shell('df -k /sdcard').splitlines()[-1].split()[3]) * 1024
    if free < sum(sizes.values()) + 16 * 1024 * 1024:
        raise ValueError('Questの空き容量が不足しています（転送用の一時領域も必要です）。')

    token = uuid.uuid4().hex
    stage = REMOTE + '/.incoming-' + token
    backup = REMOTE + '/.backup-' + token
    shell('mkdir -p ' + q(stage) + ' ' + q(backup))
    published, saved = [], []
    try:
        for i, (p, name) in enumerate(zip(files, names)):
            progress('ノーマルパックを転送', i, name)
            device.call('push', str(p), stage + '/' + name, timeout=120)
            if shell('sha256sum ' + q(stage + '/' + name)).split()[0] != hashes[name]:
                raise ValueError('Transfer verification failed: ' + name)
            progress('ノーマルパックを転送', i+1, name)
        device.ready()  # Recheck after transfer; never terminate a game.
        for i, name in enumerate(names):
            dest = REMOTE + '/' + name
            if device.exists(dest):
                shell('mv ' + q(dest) + ' ' + q(backup + '/' + name))
                saved.append(name)
            shell('mv ' + q(stage + '/' + name) + ' ' + q(dest))
            published.append(name)
            progress('ノーマルパックを配置', i+1, name)
    except Exception:
        # Roll back only files owned by this operation. Keep backups if rollback fails.
        for name in reversed(published):
            shell('rm -f ' + q(REMOTE + '/' + name))
        for name in reversed(saved):
            shell('mv ' + q(backup + '/' + name) + ' ' + q(REMOTE + '/' + name))
        raise
    finally:
        for name in names:
            shell('rm -f ' + q(stage + '/' + name))
        shell('rmdir ' + q(stage))
    # Commit succeeded: delete only the previous files saved by this transfer.
    # Keep this outside the rollback block: cleanup failure must not undo new files.
    for name in saved:
        shell('rm -f ' + q(backup + '/' + name))
    shell('rmdir ' + q(backup))
    return 'ノーマルパックの配置が完了しました。レベルを読み直してください。'
