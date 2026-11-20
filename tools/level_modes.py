"""Classify playable mesh entrypoints using Myth II mesh flags."""
def level_mode(flags, tag):
    single=bool(flags & 0x10000) or tag=='00tm'
    multi=bool(flags & 0xffff)
    return 'both' if single and multi else 'single' if single else 'multiplayer' if multi else 'unknown'

def mode_label(mode):
    return {'single':'シングル','multiplayer':'対戦','both':'シングル・対戦'}.get(mode,'不明')
