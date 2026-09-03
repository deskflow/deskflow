#!/usr/bin/env python3
"""Genererar Orderkolls ikoner (egen ikon – ingen Amazon-logotyp, se arbetsorder 6).

Motiv: ett kvitto/checklista i tilläggets marinblå identitet, med en orange
bock som enda accentfärg. Ritas matematiskt med 4x supersampling och skrivs
som PNG utan tredjepartsberoenden.
"""
import struct
import zlib
from pathlib import Path

NAVY_TOP = (61, 75, 122)      # #3D4B7A
NAVY_BOTTOM = (35, 47, 62)    # #232F3E  "Squid Ink"
PAPER = (248, 250, 252)
LINE = (150, 160, 178)
ACCENT = (255, 153, 0)        # #FF9900

SS = 4  # supersampling


def rounded_rect(x, y, w, h, r, px, py):
    """True om punkten (px, py) ligger i en rundad rektangel."""
    cx = min(max(px, x + r), x + w - r)
    cy = min(max(py, y + r), y + h - r)
    dx, dy = px - cx, py - cy
    if x <= px <= x + w and y + r <= py <= y + h - r:
        return True
    if x + r <= px <= x + w - r and y <= py <= y + h:
        return True
    return dx * dx + dy * dy <= r * r


def seg_distance(px, py, x1, y1, x2, y2):
    vx, vy = x2 - x1, y2 - y1
    wx, wy = px - x1, py - y1
    denom = vx * vx + vy * vy
    t = 0.0 if denom == 0 else max(0.0, min(1.0, (wx * vx + wy * vy) / denom))
    dx, dy = wx - t * vx, wy - t * vy
    return (dx * dx + dy * dy) ** 0.5


def sample(u, v, size):
    """Färg för en punkt i enhetskvadraten, eller None för transparent."""
    x, y = u * size, v * size
    radius = size * 0.22
    if not rounded_rect(0, 0, size, size, radius, x, y):
        return None

    t = y / size
    bg = tuple(int(NAVY_TOP[i] + (NAVY_BOTTOM[i] - NAVY_TOP[i]) * t) for i in range(3))

    # Orange bock ritas överst så att den syns hel mot både kvitto och botten.
    c = size
    check = [
        (0.50 * c, 0.72 * c, 0.62 * c, 0.84 * c),
        (0.62 * c, 0.84 * c, 0.88 * c, 0.46 * c),
    ]
    width = max(size * 0.070, 0.9)
    for (x1, y1, x2, y2) in check:
        if seg_distance(x, y, x1, y1, x2, y2) <= width:
            return ACCENT

    # Kvittot.
    pw, ph = size * 0.50, size * 0.58
    px0, py0 = size * 0.20, size * 0.17
    if rounded_rect(px0, py0, pw, ph, size * 0.06, x, y):
        for i in range(3):
            ly = py0 + ph * (0.18 + i * 0.18)
            lx0 = px0 + pw * 0.16
            lx1 = px0 + pw * (0.84 if i < 2 else 0.58)
            if seg_distance(x, y, lx0, ly, lx1, ly) <= max(size * 0.030, 0.6):
                return LINE
        return PAPER

    return bg


def render(size):
    rows = []
    for py in range(size):
        row = bytearray()
        for px in range(size):
            r = g = b = a = 0
            for sy in range(SS):
                for sx in range(SS):
                    u = (px + (sx + 0.5) / SS) / size
                    v = (py + (sy + 0.5) / SS) / size
                    color = sample(u, v, size)
                    if color is not None:
                        r += color[0]
                        g += color[1]
                        b += color[2]
                        a += 255
            n = SS * SS
            if a == 0:
                row += bytes((0, 0, 0, 0))
            else:
                hits = a / 255
                row += bytes((int(r / hits), int(g / hits), int(b / hits), int(a / n)))
        rows.append(bytes(row))
    return rows


def write_png(path, size, rows):
    raw = b''.join(b'\x00' + row for row in rows)
    def chunk(tag, data):
        payload = tag + data
        return struct.pack('>I', len(data)) + payload + struct.pack('>I', zlib.crc32(payload) & 0xFFFFFFFF)
    png = b'\x89PNG\r\n\x1a\n'
    png += chunk(b'IHDR', struct.pack('>IIBBBBB', size, size, 8, 6, 0, 0, 0))
    png += chunk(b'IDAT', zlib.compress(raw, 9))
    png += chunk(b'IEND', b'')
    path.write_bytes(png)


def main():
    out = Path(__file__).resolve().parent.parent / 'icons'
    out.mkdir(exist_ok=True)
    for size in (16, 32, 48, 128):
        write_png(out / f'icon-{size}.png', size, render(size))
        print('skrev', out / f'icon-{size}.png')


if __name__ == '__main__':
    main()
