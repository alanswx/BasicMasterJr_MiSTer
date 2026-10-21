#!/usr/bin/env python3
"""Turn a transcribed BASIC listing into a tape image and try it in the CSP emulator.

  ./typein.py build LISTING.bas NAME [-o OUT.bin]
  ./typein.py run   TAPE.bin NAME --csp BMJR_EXE --roms ROM_DIR --out NEW_DIR
                    [--pre 'CALL $E37E'] [--keys FRAME:TEXT ...] [--shots N,N,...]
                    [--rate PRESS:RELEASE]

LISTING.bas is UTF-8 with half-width katakana (ｱ-ﾟ), one numbered line per line.
build converts it to the machine's JIS X 0201 bytes and runs bmtape2, which writes
the same ASCII (.S) blocks BASIC's own SAVE does, so `LOAD NAME` reads it back.
NAME is the tape file name, as typed after LOAD. BASIC's file names hold 6 characters
(bmtape2 would silently cut a longer one, and LOAD of the long name then fails).

run boots BASIC, types the --pre commands, LOADs the tape at 300 baud and RUNs it,
then takes screenshots. --keys frames and --shots are counted from the RUN.
--rate sets how long every key is held and released (default 4:4). BASIC games often
read INKEY$ only once per loop, so give them long holds, e.g. --rate 60:20.
The report lists every screenshot plus the text page so errors such as
`SYNTAX ERROR` with their line are easy to spot. A clean run is not proof the
transcription is right; compare against the magazine's screenshots too.
"""
import argparse
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, '..', '..'))
BMTAPE2 = os.path.join(ROOT, 'ref', 'emulators', 'bmtape2', 'bmtape2', 'bmtape2')


def build(listing, name, out):
    text = open(listing, encoding='utf-8').read()
    lines = [l.rstrip('\r') for l in text.split('\n') if l.strip()]
    for l in lines:
        if not l.split(' ', 1)[0].isdigit():
            sys.exit(f'not a numbered line: {l!r}')
    data = ('\r\n'.join(lines) + '\r\n').encode('shift_jis')   # kana are single bytes
    if any(b >= 0x80 and not 0xA1 <= b <= 0xDF for b in data):
        sys.exit('listing has characters outside ASCII + half-width katakana')
    if len(name) > 6 or not name.isalnum():
        sys.exit('NAME must be 1-6 letters/digits (BASIC file names hold 6)')
    with tempfile.TemporaryDirectory() as d:
        open(os.path.join(d, name.upper()), 'wb').write(data)
        subprocess.run([BMTAPE2, '-300', name.upper()], cwd=d, check=True, stdout=subprocess.DEVNULL)
        shutil.copy(os.path.join(d, name.upper() + '.bin'), out)
    print(f'{out}: {len(lines)} lines, {len(data)} bytes of text, {os.path.getsize(out)} bytes of tape')


def screen_text(dump):
    """$100-$3FF as 24 rows of 32, kana decoded."""
    rows = []
    for r in range(24):
        row = dump[r * 32:(r + 1) * 32]
        rows.append(''.join(chr(b) if 0x20 <= b < 0x7F else bytes([b]).decode('shift_jis', 'replace')
                            if 0xA1 <= b <= 0xDF else '.' for b in row).rstrip())
    return rows


def run(a):
    size = os.path.getsize(a.tape)
    press, release = (int(x) for x in a.rate.split(':'))
    keys, frame = [], 75
    for p in a.pre:
        keys.append((frame, p + '\\n'))
        frame += (press + release) * (len(p) + 1) + 30
    load_start = frame
    # 300 baud, 11 bits a byte, 60 frames/s, plus the gaps between blocks
    run_at = load_start + (press + release) * (len(a.name) + 6) + 200 + int(size * 11 / 300 * 60 * 1.25)
    keys += [(load_start, f'LOAD {a.name}\\n'), (run_at, 'RUN\\n')]
    for k in a.keys:
        f, t = k.split(':', 1)
        keys.append((run_at + int(f), t))
    shots = [run_at - 10] + [run_at + int(s) for s in a.shots.split(',')]
    cmd = [a.csp, '--headless', '--rom-dir', a.roms, '--out', a.out, '--tape', a.tape, '--type-rate', a.rate,
           '--stop-at-frame', str(max(shots) + 1)]
    for f, t in keys:
        cmd += ['--type', f'{f}:{t}']
    for s in shots:
        cmd += ['--screenshot', str(s), '--dump-mem', f'100:300@{s}']
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode:
        sys.exit(r.stderr)
    for s in shots:
        when = 'before RUN' if s < run_at else f'RUN+{s - run_at}'
        print(f'--- frame {s} ({when}): {a.out}/frame_{s:06d}.png')
        dump = open(os.path.join(a.out, f'memdump_0100_03FF_f{s:04d}.bin'), 'rb').read()
        for row in screen_text(dump):
            if row:
                print('  |' + row)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest='cmd', required=True)
    b = sub.add_parser('build')
    b.add_argument('listing')
    b.add_argument('name')
    b.add_argument('-o', dest='out')
    r = sub.add_parser('run')
    r.add_argument('tape')
    r.add_argument('name')
    r.add_argument('--csp', required=True)
    r.add_argument('--roms', required=True)
    r.add_argument('--out', required=True)
    r.add_argument('--pre', action='append', default=[], help="command typed before LOAD, e.g. 'CALL $E37E'")
    r.add_argument('--keys', action='append', default=[], help='FRAME:TEXT, FRAME counted from RUN')
    r.add_argument('--shots', default='300,1200,3000', help='frames after RUN')
    r.add_argument('--rate', default='4:4', help='PRESS:RELEASE frames for every typed key')
    a = ap.parse_args()
    if a.cmd == 'build':
        build(a.listing, a.name, a.out or os.path.splitext(a.listing)[0] + '.bin')
    else:
        run(a)


if __name__ == '__main__':
    main()
