#!/usr/bin/env python3
"""Inspect a Basic Master Jr. tape image (the byte-stream format bm2 loads).

The image is the demodulated serial byte stream: what the CPU reads one byte at
a time out of $EE80, before any Kansas City modulation. Block layout is decoded
from ref/emulators/bmtape2/bmtape2/bm.c (raw2s_300 / raw2s_1200).

    ./bmtape_info.py inufuto/mazy.s.bin
    ./bmtape_info.py inufuto/*.s.bin        # summary line per file

Exit status is non-zero if any file fails to parse or has a bad checksum, so it
doubles as a download integrity check.
"""

import sys
import os

KIND = {0x00: "end", 0x01: "binary", 0x10: "ascii"}


def parse_300(data):
    """Yield (kind, name, block, addr, size, header_ok, data_ok) per block."""
    p = 0
    n = len(data)
    while p < n:
        # leader: a run of $FF
        if data[p] == 0xFF:
            while p < n and data[p] == 0xFF:
                p += 1
        if p >= n:
            return
        if data[p] != 0x01:          # block start
            p += 1
            continue
        p += 1
        if p + 13 > n:
            return

        header = data[p:p + 12]
        kind = header[0]
        asc_block = header[1]
        name = bytes(header[2:10]).decode("ascii", "replace").rstrip()
        bin_block = header[10]
        # A size byte of 0 means 256 -- except on the end-of-file block (kind
        # $00), which bmtape2's writer emits with a genuinely empty payload.
        # bm.c's reader inflates that to 256 too and then ignores the block;
        # here we stop early so the checksum result stays meaningful.
        size = header[11] or (0 if kind == 0x00 else 0x100)
        p += 12
        if p + 3 > n:
            return
        addr = (data[p] << 8) | data[p + 1]
        p += 2
        hsum_want = data[p]
        p += 1

        hsum = (-sum(header) - (addr >> 8) - (addr & 0xFF)) & 0xFF
        header_ok = hsum == hsum_want

        if p + size + 1 > n:
            # Ran out of file mid-block. For tapes decoded from analog audio
            # this is normally just the trailing end-of-file marker clipped
            # where the recording stops, so flag it separately from a real
            # checksum failure.
            yield (kind, name, bin_block, asc_block, addr, size, header_ok, None)
            return
        payload = data[p:p + size]
        p += size
        dsum_want = data[p]
        p += 1
        data_ok = ((-sum(payload)) & 0xFF) == dsum_want
        p += 1                        # trailing byte after each block

        yield (kind, name, bin_block, asc_block, addr, size, header_ok, data_ok)


def report(path, verbose):
    with open(path, "rb") as f:
        data = f.read()

    blocks = list(parse_300(data))
    if not blocks:
        print(f"{os.path.basename(path):20} NOT A 300-BAUD TAPE IMAGE")
        return False

    bad = [b for b in blocks if b[7] is False or not b[6]]
    clipped = [b for b in blocks if b[7] is None]
    names = sorted({b[1] for b in blocks})
    lo = min(b[4] for b in blocks if b[0] != 0x00)
    hi = max(b[4] + b[5] for b in blocks if b[0] != 0x00)

    if verbose:
        print(f"{path}  ({len(data)} bytes)")
        for kind, name, bblk, ablk, addr, size, hok, dok in blocks:
            flag = ("" if (dok and hok) else
                    "  <-- TRUNCATED (end of recording)" if dok is None else
                    "  <-- BAD SUM")
            print(f"  {KIND.get(kind, hex(kind)):6} {name:9} blk={bblk:3}/{ablk:<3} "
                  f"${addr:04X}-${addr + size - 1:04X} {size:4}B{flag}")
        print()
    else:
        print(f"{os.path.basename(path):20} {len(blocks):3} blocks  "
              f"{','.join(names):12} ${lo:04X}-${hi - 1:04X}  "
              f"{'OK' if not bad else str(len(bad)) + ' BAD CHECKSUMS'}"
              f"{' (+1 truncated tail)' if clipped else ''}")

    return not bad


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 1
    verbose = len(argv) == 2
    ok = True
    for path in argv[1:]:
        ok &= report(path, verbose)
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
