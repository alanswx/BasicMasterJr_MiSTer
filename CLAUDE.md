# CLAUDE.md

Guidance for Claude Code (claude.ai/code) when working in this repository.

## What this is

A MiSTer FPGA core for the **Hitachi Basic Master Jr. (MB-6885)**, a 1981 Japanese
home computer: HD46800 (6800-compatible) at 750 kHz, 32×24 text, 256×192 graphics,
Kansas City Standard cassette at 300 baud.

BASIC and the monitor boot, and **tape loading works end to end** — a real tape image
loads byte-exact and the program runs. Sound, interrupts, the `$EFE0` graphics modes
and blink attributes are in. See **TODO.md** for what is still open (graphics scanline
interleave, CPU cycle accuracy) and **ref/notes/** for the hardware research.

Target is real hardware. Keep everything FPGA-friendly: no unsynthesisable constructs,
prefer clock enables over gated clocks, and remember that some signals here named
`clk_*` are actually enables.

## Layout

```
BasicMasterJr.sv      MiSTer top level (hps_io, PLL, SDRAM, video/audio out)
rtl/core.v            memory map, I/O decode, ROM/RAM banking, CPU + video glue
rtl/m6801.sv          CPU core (6801; the machine is a 6800, a strict subset)
rtl/video.v           character generator + framebuffer + video timing
rtl/keyboard.v        PS/2 -> 4x16 key matrix
rtl/cassette.v        300-baud tape playback from the on-chip image buffer
rtl/rom*.mem          BASIC / monitor / printer ROMs and the character generator
rtl/dpram.v, pll.v    Quartus megafunction wrappers (shadowed in sim, see below)
vsim/                 Verilator simulation -- build, run, screenshot, regress
ref/                  hardware docs, schematics, reference emulator sources, notes
software/             tape images for testing + bmtape_info.py (decode/verify a tape)
wav2cas.py            WAV -> tape image converter (rate constants disagree with the RTL)
mame/                 untracked local MAME checkout used for cross-referencing
```

## Simulation (vsim/)

**Always run from `vsim/`** — the ROM `.mem` files are opened by `$readmemh` relative
to the working directory. `make` symlinks them in.

```bash
cd vsim/
make                                    # build (Verilator + SDL2 + ImGui)
make clean
./obj_dir/Vemu                          # windowed, with a debug panel
./obj_dir/Vemu --headless --screenshot 120 --stop-at-frame 130
./obj_dir/Vemu --headless --send-keys '150:PRINT 123\n' --screenshot 500 --stop-at-frame 510
./obj_dir/Vemu --tape ../software/inufuto/mazy.s.bin --trace-tape --stop-at-frame 900
./obj_dir/Vemu --help
```

Useful flags: `--trace-cpu` (every bus cycle), `--trace-io` (`$E800-$EFFF` accesses,
timestamped), `--trace-tape`, `--trace-key`, `--key-hold MS`, `--tape-baud N`,
`--screenshot-name <path>`, `--fast-tape on|off`, and three that make
debugging much cheaper:

* `--watch-pc F762,F766` — count bus accesses to given addresses. The fastest way to
  tell which branch of a ROM routine is actually being taken.
* `--dump-mem 1000:100@1300` — snapshot RAM to a `.bin` at a frame.
* `--poke 2100=FFx32@600` — write RAM directly (`FFx32` repeats), so a test can set
  up state without depending on typing a BASIC line correctly.

Speed is roughly **8 frames/second**, so a 500-frame scenario takes about a minute.
Screenshots are 256×192 PNGs of the active picture.

### Keyboard injection gotcha

`--send-keys` holds each key for 30 ms by default. The BASIC keyboard scan only
advances a few matrix columns per millisecond, so a shorter hold is simply missed —
this is the same "eats inputs if typed fast" behaviour MAME notes for this machine.
If a key does not register, raise `--key-hold` before suspecting the RTL.

### Regression

```bash
cd vsim/
./regression.sh              # screenshots + tape load checks (~10 min)
./regression.sh --no-tape    # screenshots only (~5 min)
./regression.sh boot         # one screenshot scenario
./regression.sh --bless      # accept current screenshots as the new golden

./tape_test.sh full ../software/inufuto/mazy.s.bin MAZY   # whole tape + run it (~30 min)
./tape_test.sh fast ../software/inufuto/mazy.s.bin MAZY   # first block at default 16x
```

The tape stage is functional, not visual: it loads a real image through the monitor
and diffs RAM against what the image says should be there — a screenshot cannot tell
a good load from a bad one. It reads the load address out of the image (titles differ:
MAZY at `$1000`, most others at `$2200`) and **fails rather than passes if the region
it dumped has no data in the image**, so it cannot succeed by comparing zeros.

Only the full form is slow (300 baud is 300 baud, and the sim runs ~8 fps).

**Run this after every RTL change.** If anything differs, stop and tell the user —
look at `regression_images/current/*.png` next to `regression_images/golden/*.png`
and describe what changed before blessing anything.

### How vsim differs from the real top

`vsim/sim.v` stands in for `BasicMasterJr.sv`. Keep them in sync. Deliberate
differences, all commented in the file:

* `clk_vid` comes from a phase accumulator instead of the Altera PLL (same
  10.638 MHz, ~59 Hz frames, up to one `clk_sys` of jitter).
* `vsim/rtl/dpram.v` shadows the Quartus dual-port RAM wrapper. **Do not add
  `-I../rtl` to the Verilator command**; it can bypass the explicit simulation
  source list and select the synthesis wrapper.
* Tape images use the same dedicated 64 KiB dual-port RAM in both tops.
  `ioctl_wait` is therefore tied low; external SDRAM is not part of playback.

Debug taps (`dbg_*` ports) are hierarchical reads out of the core, so adding one
needs no RTL change — just another `assign dbg_x = core.something;` in `sim.v`.

## Test software

`software/inufuto/` holds 22 homebrew titles as byte-stream tape images, all
checksum-verified by `software/bmtape_info.py`. They load from the monitor with
`MON <Return>`, `L <NAME> <Space>`, `G <addr> <Space>` — note the Space, not Return.
Entry is `$2200` for everything except MAZY (`$1000`). Details and provenance in
`software/README.md`.

Loading works: `./tape_test.sh` checks a block byte-for-byte, `./tape_test.sh full`
loads all of MAZY and runs it.

Tape images must fit in the 64 KiB on-chip buffer. **Fast tape loading** defaults
to On and multiplies both CPU and tape rates by 16 only while the cassette motor
is active, preserving the ROM loader's 2500 CPU cycles per bit. Set it Off for
the real 300-baud rate (`.bit_period` = 166667). That only works because
`rtl/m6800_pad.v` makes the CPU keep 6800 time — see below. If tapes stop loading,
check CPU timing first.

## CPU cycle accuracy

`rtl/m6801.sv` is a 6801 core; the machine has a 6800, which is slower per opcode.
The monitor times cassette bits with software delay loops, so this is load-bearing.

```bash
cd vsim/cputest && make && ./obj_dir/Vcputb --pad   # 199 exact, 1 too slow
./obj_dir/Vcputb --trace 7E                        # cycle-by-cycle for one opcode
./obj_dir/Vcputb --table && cp m6800_pad_table.vh ../../rtl/   # regenerate pads
```

The reference is MAME's `cycles_6800[]`, extracted by `extract_cycles.py`.
`rtl/m6800_pad.v` stalls the core's `cen` per opcode to make up the deficit;
`rtl/m6800_pad_table.vh` is **generated**, so re-run `--table` after any change to
the CPU core. `JMP` extended is still 1 clock long — padding cannot remove cycles.

## Hardware reference — read this before changing behaviour

Do not copy MAME blindly. `mame/src/mame/hitachi/bmjr.cpp` has several TODOs and at
least two behaviours that other sources contradict. The research is written up in:

* **`ref/notes/hardware-reference.md`** — I/O map, cassette format, DAC, video modes,
  keyboard, interrupts. Every fact tagged with where it came from ([SCH] schematic,
  [ROM] disassembly, [BM2]/[TAPE2] reference emulator, [MAME], [DOC]).
* **`ref/notes/mame-vs-bm2-verdicts.md`** — every point where MAME and the `bm2`
  emulator disagree, and which one the schematic or the ROM backs up.

The short version:

| Question | Answer | Who was right |
|---|---|---|
| `$EE80` read | demodulated NRZ serial bit (hardware LM565 PLL), **not** raw audio | MAME and bm2 both; this core is wrong |
| `$EE80` write | bit 0 = tape out, bits 5:1 = 5-bit R-2R DAC | MAME |
| CPU clock | **750 kHz** | bm2 + docs; MAME's 754,560 is a guess |
| `$EFD0` bit 4 | timer IRQ enable | bm2; MAME leaves it unused |
| `$EFE0` `$40-$4F` | mixed text+graphics mode | bm2; MAME only handles `$C0` |
| attribute bits 3/7 | blink foreground / blink background | bm2; MAME ignores them |
| `$EF80` | timer/interrupt source flag | ROM; MAME maps it to the BREAK key |
| `$EF40` read | interrupt acknowledge | ROM; neither emulator has it |

Primary sources are in `ref/`: the factory schematics (`ref/docs/MB-6885_schematic_a*.jpg`),
the `bm2` and `bmtape2` sources (`ref/emulators/`), and the scanned manual.

## Conventions

* Match the surrounding style: this RTL is terse, lower-case, two-space indent in
  `rtl/`, tabs in `BasicMasterJr.sv` and `vsim/`.
* When you fix something the notes describe, update the relevant line in `TODO.md`
  rather than leaving it done-but-listed.
* New hardware findings belong in `ref/notes/hardware-reference.md` with a provenance
  tag, not scattered in code comments.
