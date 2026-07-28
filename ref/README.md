# Reference material — Hitachi Basic Master Jr. (MB-6885)

Everything here is third-party documentation and emulator source collected for hardware
research. Nothing in this folder is compiled into the core.

> **Size warning:** `docs/` is ~54 MB, mostly the scanned manual and the two schematic
> scans. Decide deliberately whether you want those in git history — a `git lfs track` or a
> `ref/docs/*.pdf` + `ref/docs/*.jpg` entry in `.gitignore` are both reasonable. The notes
> and the emulator sources are small and worth committing as-is.

## Start here

* **[`notes/hardware-reference.md`](notes/hardware-reference.md)** — consolidated I/O map,
  cassette format, DAC, video modes, keyboard. Every fact tagged with its source.
* **[`notes/mame-vs-bm2-verdicts.md`](notes/mame-vs-bm2-verdicts.md)** — where MAME and bm2
  disagree, and which one the schematic/ROM backs up.

## `docs/`

| File | What it is | Origin |
|---|---|---|
| `MB-6885_schematic_a1.jpg` | Factory schematic sheet 1/2 (4565×3286) | 旧コン研, <http://kyucom.sblo.jp/article/190792009.html> |
| `MB-6885_schematic_a2.jpg` | Factory schematic sheet 2/2 — CPU, keyboard, **cassette LM565 demodulator**, **5-bit R-2R DAC**, motor relay | same |
| `MB-6885_manual.pdf` | Scanned Japanese user manual (42 MB) | <https://archive.org/details/Hitachi_MB-6885_Basic_Master_Jr> |
| `MB-6885_manual_ocr.txt` | OCR text of the above — poor quality, but grep-able | same |
| `kyucom_MB-6885_system_list.webarchive` | 旧コン研 Jr. system list: model numbers, options, prices, spec summary | <http://kyucom.sblo.jp/article/181850126.html> |
| `mb6885info_ver0.html` | BASIC statement + monitor command reference | <https://ver0.sakura.ne.jp/doc/mb6885info.html> |

Schematic reading tips: the tape/audio analogue section is at the bottom-right of sheet 2/2
(`sips -c 900 1700 --cropOffset 2400 3050` isolates it); the `IC40` output latch and R-2R
ladder are around `--cropOffset 1850 2550`.

## Original I/O magazine articles

Internet Archive has complete, searchable scans of the three 1982 issues most useful for
the Jr. video implementation:

| Issue | Relevant pages | What it establishes |
|---|---|---|
| [I/O 1982-03](https://archive.org/details/io-198203) | 198, 291-299 | Jr. overview, block diagrams, power supply, and expansion-interface documentation. |
| [I/O 1982-04](https://archive.org/details/io-198204/page/n309/mode/1up) | 304-306 | Complete MP-1710 colour-adapter circuit and specifications. |
| [I/O 1982-05](https://archive.org/details/io-198205/page/n340/mode/1up) | 337 onward | MP-1710 usage and the source of Hitachi's supplied colour-graphics utility. |

The May utility is the best primary source for `$EFE0`. Its `SCREEN` command names display
codes 2 and 4 as text/graphics **superimpose**, and its `PAGMOD` assembly table maps the five
choices exactly to `$00`, `$C0`, `$40`, `$CC`, `$4C`. The ordinary user manual documents
only `$00`, `$C0`, and `$CC`; the mixed modes were therefore real but not part of its basic
graphics tutorial.

## `emulators/`

| Path | What | Licence |
|---|---|---|
| `bm2_official/` | **bm2** by maruhiro, `bm2src_20260125.tgz` — the reference Basic Master Jr. emulator. Read `bm2mem.c` (I/O map), `tape.c` (tape timing/format), `sound.c`, `depend.c` (video modes, blink). | BSD 2-clause |
| `bmtape2/` | **bmtape2** by maruhiro, `bmtape2src_20140321.tgz` — WAV ⟷ tape-image ⟷ S-record converter. `kansas.c` is the definitive KCS encoder/decoder; `bm.c` is the definitive block/file format. | BSD 2-clause |
| `bm2/` | `github.com/kwhr0/bm2` — a Mac/Linux fork of bm2 with an independent HD6303 core. Useful as a second opinion, but `bm2_official` is upstream. | see repo |

Upstream downloads: <https://ver0.sakura.ne.jp/pc/#bm2> (also links `bmtape2` and Windows
binaries). The author's page is the canonical source for newer releases.

## MAME

Not vendored here. The comparison used `mame/src/mame/hitachi/bmjr.cpp` from the working copy
of the MAME tree sitting untracked at the repo root, plus `mame/hash/bmjr_cass.xml` (its
software list ships a single 23 MB `.wav`, which is why MAME's model is audio-based).

## Not yet retrieved

* `basicmaster.fan-site.net` — linked from ver0's page as an I/O documentation source;
  the host was unreachable at time of writing. Worth another try.
* The 332-page 1982 I/O special 「ベーシックマスター活用研究」. The
  [National Diet Library record](https://ndlsearch.ndl.go.jp/books/R100000002-I000001618378)
  has a digitized copy, but it requires the library's registered-user individual
  transmission service. The March-May monthly articles above are publicly readable.
* `s-sasaji.ddo.jp/bml3mk5/` — Basic Master Level 3 emulator; different machine, but the
  author publishes hardware notes that may cover shared subsystems.
