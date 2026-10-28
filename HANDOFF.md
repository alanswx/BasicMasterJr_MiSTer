# Handoff

Updated: 2026-10-27. Branch: `tape-support` (not yet merged to `master`). The last
code commit is `1b100f6`. Quartus Prime 17.0.2 Lite runs on `misterubuntu`.

## Where things stand

The core works on a physical MiSTer. BASIC and the monitor boot, the keyboard works,
and tapes load byte-exact and run. Sound, the timer IRQ, BREAK (NMI), the `$EFE0`
graphics modes, blink, the MP-1710 colour adapter and its border, and MiSTer
`video_mixer` are all in. **Tape save and the printer are the main missing
features.** See `TODO.md`.

Verification:

- **Simulation:** `vsim/regression.sh` passes 7/7: 5 screenshot scenarios plus the
  MAZY and ASCEND tape-load RAM diffs.
- **CSP comparison:** all 22 inufuto titles load byte-exact in both the core
  (Verilator) and the CSP reference emulator. Title screens are pixel-identical
  (0.0%), and gameplay screens differ by at most 0.4%, which is expected timing
  drift (`vsim/csp/compare_tapes.py`).
- **Hardware (2026-07-27 build):** HDMI 320x240, keyboard, the MGL tape mount,
  a byte-exact MAZY load checked over JTAG, the border (`POKE 59537,7` gives
  white), and glyphs.
- **Tape saving (2026-07-28 hardware build, merged 2026-10-27):** BASIC `SAVE`
  on the 192.168.1.198 MiSTer produced a `record-test.sav` with four
  checksum-valid blocks and `$FF` padding to its 512-byte boundary. Not yet
  re-tested on hardware since the merge with Tape speed 32x.

## After the merge: .bin tapes and CALL $E37E (2026-10-27)

- **`.bin` tapes didn't load in the merged build (`3493c7c`).**
  - Main puts the file's position in the row's extension list in `ioctl_index[7:6]`
    (`menu.cpp:2673`). With `FS1,BMTBINSAV`, a `.bin` arrives as `$41`, and the core
    compared the whole byte with 1, so the download was ignored.
  - The recording commit was hardware-tested with a `.bmt`, which is first in the
    list and still arrives as 1.
  - Fix: `tape_ioctl = ioctl_index[5:0] == 1` in both tops. `vsim/sim_main.cpp` now
    sends the extension bits as Main does, so the sim would catch this.
- **CALL $E37E in the hint.**
  - BASIC keeps its program at `$0900`, on graphics page 1, and the call moves it to
    `$2100` (see `ref/notes/hardware-reference.md`, `$EFE0`). The monitor's `E`
    undoes it.
  - The hint parser scans BASIC text for `EFE0`/`61408`. When found, the BASIC load
    step shows two lines, `TYPE E ↵ CALL $E37E ↵` / `THEN LOAD GU ↵`. The rule
    matches the instructions of all 25 BASIC tapes we have: the 8 type-ins plus the
    17 zasshi games from mister12.
  - The overlay draws its own `$`, because the ROM's looks like `#`.
- **009 GUNDAM:** a tester got `SYNTAX ERROR` after `S`.
  - CSP gives the same error without the call, or with the call made before a
    `MON`/`E` round trip.
  - Following the new hint, the core in vsim reaches gameplay.


`fork/tape-support` had a separate commit, `f603e6f` "Add native tape recording and
MiSTer save support" (2026-07-28, built on `07d4afe`), that never reached this
tree. It is now merged:
- `rtl/cassette_recorder.v` decodes the ROM's `$EE80` bit-0 KCS output in CPU
  enables (so turbo doesn't affect it) and writes bytes into the tape RAM. It
  follows bm2's overwrite/append behaviour.
- The OSD `Tape` row is `FS1,BMTBINSAV`: it also mounts a growable `.sav` sidecar.
  When the motor stops, every occupied 512-byte sector is written to it. **New
  tape recording** (bit 119) clears the buffer.
- `.bmt` is the native extension (the same byte stream as `.bin`);
  `software/blank.bmt` is an empty one.
- The hint parser reads the tape RAM's write port, so it follows recordings as
  well as downloads.
- `make recorder-test` (iverilog) runs `vsim/recorder_test.sv`.
- **Changes made in the merge:**
  - `rtl/tape_speed.v`: turbo now runs whenever the motor runs, so saves are fast
    again. Only the 250 ms hold still needs tape left to play. In the 16x sim, a
    `SAVE TEST` ends at frame 347 instead of 1,293, and the 348-byte result is
    identical at both speeds and passes `bmtape_info.py`.
  - The hint skips the start hint after a recording (the position sits at the end
    of what was saved). After Rewind tape it shows `TYPE LOAD TEST ↵`.

## Type-next hint and the reset row (2026-10-27)

- **Hint line** (`tape_hint` in `rtl/tape_overlay.sv`): while the deck is stopped,
  the overlay box shows what to type next. It is read from the block headers as
  the image downloads.
  - Before loading: `TYPE MON ↵ L MAZY ␣`, or `TYPE LOAD GORILA ↵` for a BASIC tape.
  - At the end of the tape: `TYPE G 1000 ␣` or `TYPE RUN ↵`, until that last Space
    or Return is pressed.
  - On a tape with a machine-code file and then a BASIC one: `TYPE E ↵ LOAD GALAXY ↵`
    between the two.
  - The start address is the first block's load address. The overlay now draws
    with the machine's character ROM (a second copy on `clk_vid`).
- **Reset row:** renamed `Reset and close OSD`. It did nothing when selected,
  because Main's Enter handler (`menu.cpp`) counts the undrawn `I,` line as a row
  and the `I,` line sat just above it. The `I,` line now comes last.
- **Checks:**
  - Sim: MAZY all the way to the game, plus Gorilla and Tiny Galaxy hints.
    Regression 7/7.
  - Hardware: RBF `49c39b45…4356` (worst hold +0.249 ns). On mister14 the load
    hint shows in the bottom border after an MGL mount.
  - Installed on mister12 (10.0.2.12) as `_Computer/BasicMasterJr_20261027b.rbf`
    with all 34 tapes in `games/BasicMasterJr/`. Older RBFs are in
    `/media/fat/bmjr_old/`.

## Tester build (2026-10-26)

`~/dev2/BasicMasterJr_tester_20261026.zip` (folder of the same name) replaces the
2026-10-21 one:
- **RBF:** built from commit `2975917`, SHA-256 `1a4f2a5d…6ace`.
- **Software:** the same 34 tapes as before.
- **README:** a "new since" section, the new OSD rows, and test items for 32x,
  the status bar and Rewind tape.

## OSD tidy-up (2026-10-26, commit `2975917`)

- **Menu:** follows the OSD and tape standards where sensible.
  - The rows are now `Tape`, `Tape speed`, `Rewind tape`, `Scandoubler Fx`,
    `Aspect ratio`, `Tape overlay`, `Swap Alt/Win keys` (new, bit 127) and `Reset`.
  - The menu stays flat, with no Apply and reset. The Readme's OSD section says why.
- **`rtl/keyboard.v`:** with the swap on, Win (E0 1F / E0 27) is カナ記号 and Alt
  is ignored.
- **`rtl/tape_speed.v`:** Tape speed is latched when the motor starts.
- **Hardware (mister14, RBF `1a4f2a5d…6ace`, worst setup +0.290 ns, worst hold
  +0.248 ns):** checked:
  - The menu layout.
  - The mount message from the file browser.
  - Alt+Z gives ｯ with the swap off; Win+Z gives ｯ with it on.
  - MAZY loads at 32x and runs.
- **Regression:** 7/7.

## Tape speed 32x, tape overlay, Rewind tape (2026-10-25, commit `2975917`)

Follows `~/dev2/tape-standard.md` (reference: the Sharp X1 core).

- **`rtl/tape_speed.v`:** OSD **Tape speed** `32x` (default) / `Normal`, on new
  status bit 124. The CPU enable and tape bit rate both go 32x while the motor
  runs, and for 250 ms after it stops. BASIC `LOAD` stops the motor after every
  block, and that used to run at 750 kHz, half of every BASIC load.
- **`rtl/tape_overlay.sv`:** port of `x1_tape_overlay.sv`. **Tape overlay** On/Off
  (bit 125) draws `TAPE ▶ m:ss/ m:ss x32` and a 64-step bar in the bottom border.
  Times are exact: 11 bit-times per byte at 300 baud.
- **Rewind tape** button (bit 126), and an `I,Tape ready (rewound)` message on mount.
- **`rtl/dpram.v`:** CPU port A is now unregistered (one-clock reads).
  - The first 32x build crashed every load on hardware. Port A had a registered
    output, so reads took two clocks, and an enable 2 ticks apart read stale data.
  - The sim passed because `vsim/rtl/dpram.v` modelled one clock on both ports.
    The sim model now matches the hardware: port A one clock, port B two.
  - Reproduced in sim: the old latency at 32x gave 269 of 5,560 bytes right.
    With the fix, 5,560 of 5,560.
- **Sim:** `make SIM_HZ=50000000` builds the hardware timebase and 32x. The default
  25 MHz build can only do 16x. New flags: `--tape-overlay`, `--rewind-at`.
  `--trace-tape` shows `turbo=`.
- **Results:**
  - Regression 7/7.
  - 50 MHz sim: MAZY loads byte-exact in 503 frames (was 1007 at 16x).
  - Gorilla loads in about 17 s (was about 60 s) and runs.
- **mister14, RBF `2221b82d…12b53`:**
  - Worst hold slack +0.199 ns (HDMI PLL). The system clock has +5.8 ns setup.
  - MAZY loads in about 11 s and runs.
  - Gorilla loads in about 20 s and runs in colour.
  - The overlay shows in the bottom border.
  - Rewind tape reloads from block 01.
  - "Tape ready (rewound)" was not seen after an MGL mount.
  - Files are in `/media/fat/bmjr_test/` and `/media/fat/games/BasicMasterJr/`.
- **mister14 keyboard input:**
  - `misterkvms-cli type` lost short inputs, apparently a new virtual keyboard per call.
  - SharpMZ's `tools/mister_keys.py` (copied to `/tmp`) works, but the first keys
    after a `load_core` are dropped.
  - KVMS screenshots work (`--native` for pixel-exact). A wrapper with the right
    `--mister`/`--stream` is in the session scratchpad.

## Tester build (2026-10-21)

`~/dev2/BasicMasterJr_tester_20261021.zip` is the folder of the same name, zipped.
It holds:

- the RBF built from `1b100f6`: SHA-256 `2c6c889a1e2d777f49f6160f24f65b74ecc5f23cdd6d8fd821eca4cfaa233c16`,
  0 errors, 53 benign warnings, worst setup slack +0.764 ns, worst hold slack +0.249 ns
- an example MAZY MGL
- 34 tapes: 23 inufuto, 8 type-ins, and 3 zu2 programs (Tiny Galaxy, Dendai Tiny
  BASIC, LIFE2)
- the type-in docs
- a README with the key map, the load commands for every title and an 8-point test
  list

No BIOS file is needed because the ROMs are built into the RBF.

**This RBF has not been run on hardware yet.** It is the first hardware build with
the video RGB/blanking fix below. The MiSTer at `192.168.1.198` still has the
2026-07-27 glyph-fix build as `/media/fat/BasicMasterJr.rbf`. Backups there use
`.rbf.bak`, so the MGL's name-only core lookup can't pick them.

To rebuild:

```bash
git archive HEAD -- BasicMasterJr.qpf BasicMasterJr.qsf BasicMasterJr.sdc BasicMasterJr.srf \
    BasicMasterJr.sv files.qip rtl sys | ssh misterubuntu 'mkdir ~/bmjr_build_X && tar -x -C ~/bmjr_build_X'
ssh misterubuntu 'cd ~/bmjr_build_X && ~/intelFPGA_lite/quartus/bin/quartus_sh --flow compile BasicMasterJr'
```

A build takes about 4.5 minutes. `~/bmjr_build_1b100f6` on misterubuntu is the tree
for the tester RBF.

## Work since the 2026-07-27 hardware build

### RTL / sim (commit `346b8f0`)

- **Video RGB lagged blanking.** RGB arrived one `ce_pix` step after `hb`/`vb`/`hs`/`vs`:
  1 px on hardware (BORDER=1) and 2 px in vsim (BORDER=0). `rtl/video.v` now samples
  `vram_data` on the same `ce_pix` step that writes RGB. The counter drives internal
  `hs_i`/`vs_i`/`hb_i`/`vb_i`, and the outputs are delayed one step so they line up.
- **The sim dropped picture line 0.** In `vsim/sim/sim_video.cpp`, the vblank-falling
  reset now runs before the hblank-falling line increment.
- The golden images were re-blessed. Each change was exactly a 2 px left and 1 line
  up shift, with no other differing pixels.

### Reference emulator: CSP (commit `be21941`; CSP-side changes NOT committed)

- `~/dev2/common_source_code` has a headless `bmjr` CLI (`MAC_BMJR_CLI.md`) that runs
  about 300x faster than vsim. `vsim/csp/` holds `make_roms.py`, `compare_tapes.py`
  (core against CSP for any list of tapes) and `typein.py`.
- CSP's bmjr VM had two colour bugs, and both are fixed in the CSP working tree:
  - `draw_screen` read `color_table[taddr]` with `taddr` starting at `$100`, so every
    cell took the colour 8 rows below it. This was the main bug.
  - `$E892` now follows bm2: bit 0 gates colour writes, and any write turns colour on.
  - The CLI also accepts `.cmt` tapes.
- **Open question for the user:** the CSP repo has many other people's uncommitted
  changes besides these: `src/vm/bmjr/memory.*`, `src/mac/bmjr_cli.mm` and
  `MAC_BMJR_CLI.md`. Should only the bmjr pieces be committed, or everything? No
  answer yet.

### Software (commits `9998085`, `da36daa`, `aa79764`, `1b100f6`)

- `software/found/`: the zu2/BASICMASTER mirror (Tiny Galaxy, Dendai Tiny BASIC,
  L2BASCOM, LIFE2, GAME68, KUMAJIRI, …) and inufuto UCHOTOS.
- `software/found2/`: the type-in source survey. It includes an index of every
  BM program in マイコンBASICマガジン with archive.org page hits
  (`micombasic_archive_org/bemaga_bm_index.tsv`).
- `software/typein/`: six programs transcribed from ベーマガ scans and built into
  tapes with `vsim/csp/typein.py`: Graphic Helmet, Gorilla, MISSILE FIGHT, Wine
  Cellar, DRINK CAN and T.KUROKAWA. Graphic OPA and WILWESP are also there. All of
  them run in CSP. The page scans are git-ignored.
- Tiny Galaxy is two separate tapes in zu2's repo. The tester zip joins them into
  one (`cat galaxy.bin.cmt galaxy.bas.cmt`), and it works in CSP:
  `MON`, `L GALAXY` Space, `E`, `LOAD GALAXY`, `RUN`. Dendai Tiny BASIC: `L DENDAI`,
  `G 1000`. LIFE2's "black screen" is normal: the program waits for input.

### State of the type-ins on the core (vsim)

- **Graphic Helmet:** plays.
- **DRINK CAN, WINE CELLAR, T.KUROKAWA:** reach gameplay in colour.
- **Gorilla:** reaches its title screen only. A 120 ms injected key hold is too
  short for its roughly 1.5 s INKEY$ poll, so this is a test problem, not a known
  core bug.
- **MISSILE FIGHT:** not yet tried on the core.

## Related: Level 3 / MB-S1

A tester sent Level 3 and S1 software. None of it runs on the Jr: those machines
have a 6809, a different BASIC and a different tape format. A separate project was
started at `~/dev2/BasicMasterL3_MiSTer/`, with a full `TODO.md` handoff (sources,
MAME/CSP/EmuB references, reusable RTL, step 0 = a document/software download
subagent).

## Earlier history (2026-07, still relevant)

- **Hardware CPU clock:** `clk_cpu` is an enable, not a clock. The CPU runs on
  `clk_sys` with `cen_in`. Feeding the enable into clock ports glitched on hardware.
- **Tape download:** the external-SDRAM path lost bytes. Tapes now go into a
  dedicated 64 KiB dual-port BRAM: `hps_io` writes port A, the cassette reads port B,
  and `ioctl_wait` is tied low.
- **First-column glyph bug:** the renderer now waits for both RAM read stages before
  the character-ROM lookup.
- **Fast tape:** 16x CPU and tape rate only while the motor runs, which keeps the
  ROM's 2500 CPU cycles per bit. Replaced 2026-10-25 by Tape speed 32x (above).
- **The `* ERROR` seen on hardware** was a mistyped follow-up command: monitor
  commands end with Space, not Return.

## Verification commands

```bash
cd vsim && make && ./regression.sh            # 7 tests
./tape_test.sh full ../software/inufuto/mazy.s.bin MAZY
cd cputest && make && ./obj_dir/Vcputb --pad   # 6800 cycle accuracy
```

See `vsim/csp/README.md` for the CSP comparison and the type-in tools.
