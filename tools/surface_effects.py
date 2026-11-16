"""Reference for Build434 0x4bae80's 8-bit terrain shade composition.

Coordinates are original raw units (512/game unit), with 64 raw units/pixel.
These are shade bytes, NOT RGB values or alpha-blended final terrain colors.
Kept offline until source/lifetime synchronization is validated.
"""
def stamp_shade(dst, size, src, width, height, stride, x, y, kind, strength=65535):
    if kind not in (0, 1, 2):
        raise ValueError('unknown original composition kind')
    if len(dst) != size * size or stride < width or len(src) < stride * height:
        raise ValueError('invalid surface extent')
    if not 0 <= strength <= 65535:
        raise ValueError('invalid strength')
    left, top = x >> 6, y >> 6
    if width <= 0 or height <= 0 or left >= size or top >= size or left+width <= 0 or top+height <= 0:
        return
    sx, sy = max(0, -left), max(0, -top)
    width, height = min(width-sx, size-max(left, 0)), min(height-sy, size-max(top, 0))
    left, top = max(left, 0), max(top, 0)
    carry = [0] * width
    fx, fy = x & 63, y & 63
    for row in range(height):
        horizontal = 0
        for col in range(width):
            at = (top+row)*size+left+col
            source = src[(sy+row)*stride+sx+col]
            old = dst[at]
            if kind == 2:
                dst[at] = min(old, source)
                continue
            value = source * strength >> 16
            part = value * (63-fx)
            horizontal += part
            vertical = horizontal * (63-fy)
            amount = (carry[col]+vertical) >> 12
            carry[col] = horizontal*63-vertical
            horizontal = value*63-part
            if kind == 0:
                if value:
                    dst[at] = min(old, max(64, (255-amount)*old >> 8))
            else:
                dst[at] = min(255, old+amount)
