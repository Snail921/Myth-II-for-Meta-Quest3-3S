"""Create a deterministic fresh runtime from the pinned upstream APK, not a device export.

The build deliberately selects Wine builtins rather than optional native Windows DLLs.
Requires zstandard. This builder does not itself establish upstream license compliance.
"""
import argparse, copy, hashlib, io, json, tarfile, zipfile
from pathlib import Path, PurePosixPath
import zstandard

UPSTREAM_SHA256 = 'ea5c81bf0e5b90a6b9ccb8007121ab6738c8646a5a60b341ce0ac2b33f1d100a'
PREFIX = 'files/rootfs/'

def build(apk, output):
    apk, output = Path(apk), Path(output)
    with apk.open('rb') as f:
        if hashlib.file_digest(f, 'sha256').hexdigest() != UPSTREAM_SHA256:
            raise ValueError('Expected the pinned upstream Winlator 11.2 APK')
    if output.exists():
        raise ValueError('Choose a new output file')
    entries = {}
    def put(name, data=b'', kind=tarfile.REGTYPE, link=''):
        info = tarfile.TarInfo(name)
        info.type, info.linkname = kind, link
        info.mode = 0o755 if kind == tarfile.DIRTYPE else 0o644
        info.size = len(data) if kind == tarfile.REGTYPE else 0
        entries[name] = (info, data)
    def overlay(z, asset, prefix):
        print('Reading ' + asset, flush=True)
        with z.open(asset) as source, zstandard.ZstdDecompressor().stream_reader(source) as stream:
            with tarfile.open(fileobj=stream, mode='r|') as archive:
                for member in archive:
                    name = member.name.removeprefix('./')
                    if name in ('', '.'):
                        continue
                    if PurePosixPath(name).is_absolute() or '..' in PurePosixPath(name).parts:
                        raise ValueError('Unsafe upstream path')
                    if not (member.isfile() or member.isdir() or member.issym() or member.islnk()):
                        raise ValueError('Unsupported upstream entry: ' + name)
                    info = copy.copy(member)
                    info.name = prefix + name
                    if info.islnk():
                        info.linkname = prefix + info.linkname.removeprefix('./')
                    entries[info.name] = (info, archive.extractfile(member).read() if member.isfile() else b'')
    with zipfile.ZipFile(apk) as z:
        overlay(z, 'assets/rootfs.tzst', PREFIX)
        overlay(z, 'assets/rootfs_patches.tzst', PREFIX)
        # Upstream home skeleton becomes container 1. No local registry or save is imported.
        home = PREFIX + 'home/xuser'
        for name in list(entries):
            if name == home or name.startswith(home + '/'):
                info, data = entries.pop(name)
                info = copy.copy(info)
                info.name = name.replace(home, home + '-1', 1)
                entries[info.name] = info, data
        overlay(z, 'assets/container_pattern.tzst', home + '-1/')
        # Use Wine's own fonts in the fresh prefix.
        font_dir = home + '-1/.wine/drive_c/windows/Fonts/'
        for name in list(entries):
            if name.startswith(font_dir):
                del entries[name]
        wine_fonts = PREFIX + 'opt/wine/share/wine/fonts/'
        for name, (info, data) in list(entries.items()):
            if name.startswith(wine_fonts) and info.isfile():
                clone = copy.copy(info); clone.name = font_dir + name.rsplit('/', 1)[1]
                entries[clone.name] = clone, data
        common = json.loads(z.read('assets/common_dlls.json'))
        for arch, directory in [('x86_64-windows', 'system32'), ('i386-windows', 'syswow64')]:
            for name in common[directory]:
                source = PREFIX + 'opt/wine/lib/wine/' + arch + '/' + name
                if source not in entries:
                    raise ValueError('Missing Wine builtin: ' + source)
                info, data = entries[source]
                target = home + '-1/.wine/drive_c/windows/' + directory + '/' + name
                clone = copy.copy(info); clone.name = target
                entries[target] = clone, data
    components = ','.join(n+'=0' for n in ('direct3d','directsound','directmusic','directshow','directplay','xaudio','vcrun2005','vcrun2010','wmdecoder'))
    config = dict(id=1, name='Myth II', screenSize='1280x720',
                  envVars='ZINK_DESCRIPTORS=lazy ZINK_DEBUG=compact MESA_SHADER_CACHE_DISABLE=false MESA_SHADER_CACHE_MAX_SIZE=512MB mesa_glthread=true WINEESYNC=1 TU_DEBUG=noconform',
                  cpuList='0,1,2,3,4,5', cpuListWoW64='0,1,2,3,4,5',
                  graphicsDriver='turnip,zink', dxwrapper='dxvk', audioDriver='alsa',
                  wincomponents=components, drives='D:/storage/emulated/0/Android/data/org.mythvr.standalone/files/GameE:/storage/emulated/0/Download',
                  hudMode=0, startupSelection=1, box64Preset='PERFORMANCE',
                  desktopTheme='LIGHT,IMAGE,#0277BD,0',
                  extraData=dict(appVersion='33', rfsVersion='23', wincomponents=components))
    put(home+'-1/.container', json.dumps(config, separators=(',',':')).encode())
    put(home, kind=tarfile.SYMTYPE, link='xuser-1')
    put(PREFIX+'.winlator/.rfs_version', b'23')
    put('files/installed_components', kind=tarfile.DIRTYPE)
    # Normalize archive metadata and add parent directories without touching host paths.
    for name in list(entries):
        for parent in PurePosixPath(name).parents:
            if str(parent) != '.' and str(parent) not in entries:
                put(str(parent), kind=tarfile.DIRTYPE)
    output.parent.mkdir(parents=True, exist_ok=True)
    try:
        with tarfile.open(output, 'w', format=tarfile.PAX_FORMAT) as archive:
            for name in sorted(entries):
                info, data = entries[name]
                info.uid = info.gid = info.mtime = 0
                info.uname = info.gname = ''
                info.pax_headers = {}
                archive.addfile(info, io.BytesIO(data) if info.isfile() else None)
        with output.open('rb') as f:
            digest = hashlib.file_digest(f, 'sha256').hexdigest()
        report = dict(version=1, upstream_sha256=UPSTREAM_SHA256, runtime_sha256=digest,
                      entries=len(entries), size=output.stat().st_size,
                      native_windows_components=False, device_export_used=False,
                      validation='generated; fresh-device runtime verification pending')
        output.with_suffix('.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf8')
        print(json.dumps(report), flush=True)
        return report
    except BaseException:
        output.unlink(missing_ok=True)
        raise

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--winlator-apk', required=True)
    parser.add_argument('--out', required=True)
    args = parser.parse_args()
    build(args.winlator_apk, args.out)
