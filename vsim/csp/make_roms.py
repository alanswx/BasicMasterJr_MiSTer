#!/usr/bin/env python3
"""Write the ROM directory the Common Source Code Project bmjr VM expects, from rtl/*.mem.

  ./make_roms.py OUT_DIR

romb.mem is $B000-$CFFF, romdf.mem is $D000-$DFFF then $F000-$FFFF (see the rom
instances in rtl/core.v), rome.mem is the $E000 printer ROM, rom0.mem the font.
"""
import os
import sys

RTL = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'rtl')


def mem(name, size):
    with open(os.path.join(RTL, name)) as f:
        data = bytes(int(t, 16) for t in f.read().split())
    if len(data) != size:
        sys.exit(f'{name}: {len(data)} bytes, expected {size}')
    return data


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    out = sys.argv[1]
    os.makedirs(out, exist_ok=True)
    romdf = mem('romdf.mem', 0x2000)
    roms = {
        'BASIC.ROM': mem('romb.mem', 0x2000) + romdf[:0x1000],
        'MONITOR.ROM': romdf[0x1000:],
        'PRINTER.ROM': mem('rome.mem', 0x800),
        'FONT.ROM': mem('rom0.mem', 0x800),
    }
    for name, data in roms.items():
        with open(os.path.join(out, name), 'wb') as f:
            f.write(data)
        print(f'{name} {len(data)}')


if __name__ == '__main__':
    main()
