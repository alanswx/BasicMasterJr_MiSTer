# Software — tape images for testing the core

## What is here

`inufuto/` — 22 homebrew games by **inufuto**, downloaded from the author's own
site, <http://inufuto.web.fc2.com/8bit/>. Each game is published for dozens of
retro machines; the `bm/` (ベーシックマスター / Basic Master) builds are the ones
here. Basic Master Level 2 software runs on the Jr. — the Jr. is the last L2-series
model and is documented as L1/L2/L2II software compatible.

`bmtape_info.py` — decodes and checksum-verifies a tape image. Also a download
integrity check; it exits non-zero if anything fails.

```bash
./bmtape_info.py inufuto/*.s.bin     # one summary line per file
./bmtape_info.py inufuto/mazy.s.bin  # per-block detail
```

## Why the Internet Archive did not have these

Worth recording so nobody re-treads it: archive.org has the **scanned MB-6885
manual** (already saved as `ref/docs/MB-6885_manual.pdf`) but **no Basic Master Jr.
tape collection**. The `mame-sl` item (95 GB of MAME software-list ROMs) carries
`bml3_flop.zip` for the Level 3, but no `bmjr_cass`. MAME's own `bmjr_cass` software
list has exactly one title in it, and it ships as a 23 MB WAV.

inufuto's site is a better source anyway: 22 titles, actively maintained, and
already in the byte-stream tape format.

## Format

The `.s.bin` files are **tape images in the byte-stream format**, not audio: the
demodulated serial bytes the CPU reads one at a time out of `$EE80`. That is the
format `bm2` loads and the format this core should grow into — see
`ref/notes/mame-vs-bm2-verdicts.md` §1. Each file starts with a run of `$FF`
(the 300-baud leader) followed by `$01` block-start markers.

Block layout, per `ref/emulators/bmtape2/bmtape2/bm.c`:

```
65 x $FF          leader
$01               block start
kind              $01 binary, $10 ascii, $00 end-of-file
asc_block         ASCII block number
8 bytes           file name, space padded (ext .B = binary, .S = ASCII/BASIC)
bin_block         binary block number
size              block length, 0 means 256
addr_hi, addr_lo  load address
hsum              -(sum of the 12 header bytes) & $FF
size x data
dsum              -(sum of data) & $FF
```

Two titles (MAZY, NEURAS) only ship as Motorola S-records upstream; they were
converted here with `bmtape2` built from `ref/emulators/bmtape2/`:

```bash
cd ref/emulators/bmtape2/bmtape2
make CFLAGS="-Os -Wall -Wno-implicit-function-declaration -Wno-deprecated-non-prototype" LDFLAGS=""
./bmtape2 -300 mazy.s          # writes mazy.s.bin (tape image) and mazy.s.wav
```

`bmtape2` also converts the other direction (WAV capture → tape image → S-record),
which is how you would ingest a real tape recording.

## Titles

All verified: every block's header and data checksum is correct.

| file | tape name | entry | occupies | bytes |
|---|---|---|---|---|
| `aerial.s.bin` | `AERIAL` | `$2200` | $2200-$4F37 | 15,430 |
| `antiair.s.bin` | `ANTIAI` | `$2200` | $2200-$3CF9 | 9,202 |
| `ascend.s.bin` | `ASCEND` | `$2200` | $2200-$4586 | 12,129 |
| `awass.s.bin` | `AWASS` | `$2200` | $2200-$4836 | 13,063 |
| `battlot.s.bin` | `BATTLO` | `$2200` | $2200-$4E20 | 15,069 |
| `bootskell.s.bin` | `BOOTSK` | `$2200` | $2200-$4C54 | 14,445 |
| `cacorm.s.bin` | `CACORM` | `$2200` | $2200-$466F | 12,444 |
| `cavit.s.bin` | `CAVIT` | `$2200` | $2200-$5245 | 16,458 |
| `cracky.s.bin` | `CRACKY` | `$2200` | $2200-$43EF | 11,558 |
| `guntus.s.bin` | `GUNTUS` | `$2200` | $2200-$5176 | 16,169 |
| `hopman.s.bin` | `HOPMAN` | `$2200` | $2200-$45B0 | 12,171 |
| `impetus.s.bin` | `IMPETU` | `$2200` | $2200-$60C4 | 21,317 |
| `lift.s.bin` | `LIFT` | `$2200` | $2200-$448D | 11,798 |
| `mazy.s.bin` | `MAZY` | `$1000` | $1000-$25B7 | 7,446 |
| `mazy2.s.bin` | `MAZY2` | `$2200` | $2200-$4AB6 | 13,867 |
| `mieyen.s.bin` | `MIEYEN` | `$2200` | $2200-$4676 | 12,451 |
| `Neuras.s.bin` | `NEURAS` | `$2200` | $2200-$4B4A | 14,097 |
| `osotos.s.bin` | `OSOTOS` | `$2200` | $2200-$4948 | 13,419 |
| `Ruptus.s.bin` | `RUPTUS` | `$2200` | $2200-$57DB | 18,298 |
| `svellas.s.bin` | `SVELLA` | `$2200` | $2200-$483F | 13,072 |
| `sword.s.bin` | `SWORD` | `$2200` | $2200-$4435 | 11,710 |
| `yewdow.s.bin` | `YEWDOW` | `$2200` | $2200-$4772 | 12,785 |

`MAZY` is the odd one out — it loads and runs at `$1000`; every other title uses
`$2200`. All of them are 6800 machine code, loaded and started from the monitor.

## How to load one

From the author's instructions for the Basic Master build, and matching the
monitor's own syntax:

```
MON <Return>          enter the machine-language monitor
L MAZY <Space>        load; the command is terminated by Space, not Return
G 1000 <Space>        run from the entry address
```

The Space-not-Return terminator is real — MAME's driver notes it too
(`bmjr.cpp` header comment).

In the simulator, once the tape path works:

```bash
cd vsim/
./obj_dir/Vemu --tape ../software/inufuto/mazy.s.bin \
               --send-keys '150:MON\n' --send-keys '260:L MAZY ' \
               --screenshot 900 --stop-at-frame 950
```

## Current status

Tape loading works end to end in both Verilator and on a physical MiSTer. The
hardware test mounted `mazy.s.bin`, transferred all 7,446 image bytes into the
64 KiB on-chip tape buffer, loaded all 5,560 program bytes byte-for-byte into
`$1000-$25B7`, and ran MAZY at `$1000`.

The simulator uses the same dual-port buffer architecture. `vsim/tape_test.sh`
checks loaded RAM against the tape image; its `full` mode also executes the
program. The shared `sim/sim_bus.cpp` completion message reports the final byte
address, so its displayed count is one less than the number of bytes transferred.

## Provenance and redistribution

These are inufuto's own programs, published for free download on the author's
site. No explicit licence is stated there, so treat them as "freely downloadable,
author retains rights" — fine for local testing, but check with the author before
redistributing them as part of a release. The upstream pages also carry source
archives (`*_src.zip`) and real-hardware WAVs, which were deliberately not mirrored
here to keep the repo small; `bmtape2` regenerates the WAVs from these images.
