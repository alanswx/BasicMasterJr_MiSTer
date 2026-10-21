#!/usr/bin/env python3
"""Load tape images in both the Verilator sim and the CSP reference emulator and compare.

  ./compare_tapes.py --csp BMJR_EXE --roms ROM_DIR --out NEW_DIR [-j N] IMAGE...

For each image: MON, L <name>, wait for the load, dump the loaded range, G <entry>,
screenshots at +300/+900 frames, Space, screenshot at +1600. Both runs are checked
byte-for-byte against the image. At each screenshot the text page ($100-$3FF) and
graphics page ($900-$20FF) are dumped from both and compared: identical screen RAM
means both CPUs did the same thing, whatever the renderers make of it. Screenshots go
in OUT/<title>/ and, if Pillow is importable, into OUT/montage.png (core left, CSP
right). Writes OUT/report.txt.

vsim runs with its default 16x fast tape, CSP at real 300 baud, so the load phase is
scheduled separately for each; everything after G is the same number of frames.
vsim also gets --key-hold 60: at the default 30 ms the monitor dropped the last S of
OSOTOS. CSP's bmjr VM needed two colour fixes in ~/dev2/common_source_code before its
colour screens were right: draw_screen read color_table[taddr] with taddr starting at
$100 (every cell took the colour 8 rows below, the last 8 rows read past the table),
and $E892 now follows bm2 (bit 0 gates colour writes, any write turns colour on).
"""
import argparse
import concurrent.futures as cf
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
VSIM = os.path.normpath(os.path.join(HERE, '..'))
SHOTS = (300, 900, 1600)    # frames after G; Space is pressed at +1000
SCREEN = ((0x100, 0x300), (0x900, 0x1800))   # text page, graphics page 0


def screen_dumps(start):
    return sum((['--dump-mem', f'{a:X}:{n:X}@{start + s}'] for s in SHOTS for a, n in SCREEN), [])


def screen_files(d, start):
    return [[os.path.join(d, f'memdump_{a:04X}_{a + n - 1:04X}_f{start + s:04d}.bin') for a, n in SCREEN]
            for s in SHOTS]


def parse(path):
    """Name, expected bytes {addr: value} and lowest address, per the block layout in software/README.md."""
    img = open(path, 'rb').read()
    p, exp, name = 0, {}, None
    while p + 16 < len(img):
        if img[p] != 0x01 or p == 0 or img[p - 1] != 0xFF:
            p += 1
            continue
        kind, size, addr = img[p + 1], img[p + 12] or 256, img[p + 13] << 8 | img[p + 14]
        if kind == 0x00:
            break
        if kind == 0x01:
            name = name or img[p + 3:p + 11].decode('ascii', 'replace').split('.')[0].strip()
            for i in range(size):
                exp[addr + i] = img[p + 16 + i]
        p += 16 + size + 1
    return name, exp, len(img)


def check(dump, exp, lo):
    got = open(dump, 'rb').read() if os.path.exists(dump) else b''
    want = bytes(exp.get(lo + i, 0) for i in range(max(exp) - lo + 1))
    if len(got) != len(want):
        return False, f'no dump ({len(got)} bytes)'
    bad = [i for i in range(len(want)) if lo + i in exp and got[i] != want[i]]
    if bad:
        return False, f'{len(exp) - len(bad)}/{len(exp)} bytes, first bad ${lo + bad[0]:04X}'
    return True, f'{len(exp)}/{len(exp)} bytes'


def keys(name, entry, load_done):
    g = load_done + 100
    return g, [(75, 'MON\\n'), (200, 'L'), (260, name), (340, ' '),
               (g, 'G'), (g + 100, f'{entry:04X}'), (g + 200, ' '), (g + 200 + 1000, ' ')]


def run_csp(a, image, name, exp, length, out):
    lo = min(exp)
    load_done = 340 + int(length * 11 / 300 * 60 * 1.1) + 300
    g, ks = keys(name, lo, load_done)
    start = g + 200
    stop = start + max(SHOTS)
    tape = image if image.lower().endswith(('.bin', '.wav')) else None
    cmd = [a.csp, '--headless', '--rom-dir', a.roms, '--out', os.path.join(out, 'csp'), '--tape', tape or image,
           '--dump-mem', f'{lo:04X}:{max(exp) - lo + 1:X}@{load_done}', '--stop-at-frame', str(stop)]
    for f, k in ks:
        cmd += ['--type', f'{f}:{k}']
    for s in SHOTS:
        cmd += ['--screenshot', str(start + s)]
    r = subprocess.run(cmd + screen_dumps(start), capture_output=True, text=True)
    d = os.path.join(out, 'csp')
    dump = os.path.join(d, f'memdump_{lo:04X}_{max(exp):04X}_f{load_done:04d}.bin')
    shots = [os.path.join(d, f'frame_{start + s:06d}.png') for s in SHOTS]
    return (r.returncode == 0, r.stderr.strip()[-200:]), check(dump, exp, lo), shots, screen_files(d, start)


def run_vsim(a, image, name, exp, length, out):
    lo = min(exp)
    load_done = 340 + int(length * 11 / 300 * 60 * 1.3 / 16) + 400
    g, ks = keys(name, lo, load_done)
    start = g + 200
    stop = start + max(SHOTS)
    wd = os.path.join(out, 'core')
    os.makedirs(wd)
    for m in ('rom0.mem', 'romb.mem', 'romdf.mem', 'rome.mem'):
        os.symlink(os.path.join(VSIM, m), os.path.join(wd, m))
    cmd = [os.path.join(VSIM, 'obj_dir', 'Vemu'), '--headless', '--key-hold', '60', '--tape', os.path.abspath(image),
           '--dump-mem', f'{lo:04X}:{max(exp) - lo + 1:X}@{load_done}', '--stop-at-frame', str(stop),
           '--screenshot', ','.join(str(start + s) for s in SHOTS)]
    for f, k in ks:
        cmd += ['--send-keys', f'{f}:{k}']
    r = subprocess.run(cmd + screen_dumps(start), cwd=wd, capture_output=True, text=True)
    dump = os.path.join(wd, f'memdump_{lo:04X}_{max(exp):04X}_f{load_done:04d}.bin')
    shots = [os.path.join(wd, f'screenshot_frame_{start + s:04d}.png') for s in SHOTS]
    return (r.returncode == 0, r.stdout.strip()[-200:]), check(dump, exp, lo), shots, screen_files(wd, start)


def screen_ram(v, c, j):
    """'same', or how many screen-RAM bytes differ at SHOTS[j]."""
    n = 0
    for p, q in zip(v[3][j], c[3][j]):
        if not (os.path.exists(p) and os.path.exists(q)):
            return 'n/a'
        n += sum(x != y for x, y in zip(open(p, 'rb').read(), open(q, 'rb').read()))
    return 'same' if n == 0 else f'{n}B'


def one(a, image):
    name, exp, length = parse(image)
    title = os.path.basename(image).split('.')[0]
    out = os.path.join(a.out, title)
    os.makedirs(out)
    if not exp:
        return title, name, None, None
    with cf.ThreadPoolExecutor(2) as ex:
        c = ex.submit(run_csp, a, image, name, exp, length, out)
        v = ex.submit(run_vsim, a, image, name, exp, length, out)
        return title, name, v.result(), c.result()


# Offset of a vsim screenshot against the true picture. Was (2, -1) until rtl/video.v
# put RGB back in line with blanking and SimVideo stopped dropping picture line 0;
# kept so a regression shows up as a shift rather than noise.
CORE_DX, CORE_DY = 0, 0


def pixel_diff(p, q):
    from PIL import Image
    if not (os.path.exists(p) and os.path.exists(q)):
        return None
    a, b = Image.open(p).convert('RGB'), Image.open(q).convert('RGB')
    if a.size != b.size:
        return None
    pa, pb = a.load(), b.load()
    w, h = a.size
    xs = range(max(0, CORE_DX), w + min(0, CORE_DX))
    ys = range(max(0, -CORE_DY), h - max(0, CORE_DY) - 1)
    n = sum(pa[x, y] != pb[x - CORE_DX, y - CORE_DY] for y in ys for x in xs)
    return n * 100.0 / (len(xs) * len(ys))


def montage(results, path):
    from PIL import Image, ImageDraw
    W, H, label = 256, 192, 14
    rows = [r for r in results if r[2]]
    sheet = Image.new('RGB', (110 + len(SHOTS) * 2 * (W + 4), len(rows) * (H + label + 4)), (40, 40, 40))
    d = ImageDraw.Draw(sheet)
    for i, (title, name, v, c) in enumerate(rows):
        y = i * (H + label + 4)
        d.text((4, y + label + H // 2), title, fill=(255, 255, 255))
        for j in range(len(SHOTS)):
            for k, (who, res) in enumerate((('core', v), ('csp', c))):
                x = 110 + (j * 2 + k) * (W + 4)
                d.text((x, y), f'{who} G+{SHOTS[j]}', fill=(200, 200, 120))
                if os.path.exists(res[2][j]):
                    sheet.paste(Image.open(res[2][j]).convert('RGB').resize((W, H)), (x, y + label))
    sheet.save(path)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--csp', required=True)
    ap.add_argument('--roms', required=True)
    ap.add_argument('--out', required=True)
    ap.add_argument('-j', type=int, default=max(1, (os.cpu_count() or 2) - 2))
    ap.add_argument('images', nargs='+')
    a = ap.parse_args()
    a.csp, a.roms = os.path.abspath(a.csp), os.path.abspath(a.roms)
    os.makedirs(a.out)
    with cf.ThreadPoolExecutor(a.j) as ex:
        results = sorted(ex.map(lambda i: one(a, i), a.images), key=lambda r: r[0].lower())
    try:
        import PIL  # noqa: F401
        have_pil = True
    except ImportError:
        have_pil = False
    lines, fails = [], 0
    for title, name, v, c in results:
        if not v:
            lines.append(f'{title:10} no binary blocks')
            fails += 1
            continue
        diffs = [pixel_diff(v[2][j], c[2][j]) if have_pil else None for j in range(len(SHOTS))]
        ok = v[1][0] and c[1][0]
        fails += not ok
        lines.append(f'{title:10} L {name:7} core: {"ok  " if v[1][0] else "FAIL"} {v[1][1]:20} '
                     f'csp: {"ok  " if c[1][0] else "FAIL"} {c[1][1]:20} screen RAM: '
                     + ' '.join(f'{screen_ram(v, c, j):>6}' for j in range(len(SHOTS))) + '  pixels: '
                     + ' '.join('  -  ' if d is None else f'{d:4.1f}%' for d in diffs))
        for who, res in (('core', v), ('csp', c)):
            if not res[0][0]:
                lines.append(f'{"":10} {who} exited non-zero: {res[0][1]}')
    report = '\n'.join(lines) + f'\n\n{len(results) - fails}/{len(results)} loaded byte-exact in both\n'
    open(os.path.join(a.out, 'report.txt'), 'w').write(report)
    print(report)
    if have_pil:
        montage(results, os.path.join(a.out, 'montage.png'))
    sys.exit(1 if fails else 0)


if __name__ == '__main__':
    main()
