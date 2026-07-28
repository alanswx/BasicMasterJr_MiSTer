# Handoff — timing-clean border/video-mixer build

Date: 2026-07-27. Branch: `tape-support`. Quartus Prime 17.0.2 Lite.

## Current result

The core now works on physical MiSTer hardware:

- HDMI locks and displays the native 256x192 picture.
- BASIC and MONITOR boot and accept keyboard input.
- An MGL launches the core and mounts `mazy.s.bin` through `hps_io` index 1.
- The complete 7,446-byte image reached the FPGA byte-for-byte.
- MONITOR loaded all 5,560 program bytes at `$1000-$25B7` byte-for-byte.
- `G 1000 <Space>` started MAZY and reached its `PUSH SPACE KEY` screen.
- **Fast tape loading** is On by default and completes MAZY in about 15 seconds;
  the user confirmed the 16x hardware load works perfectly. Off retains 300-baud timing.

The installed test MGL is `/media/fat/.LASTLAUNCH.mgl`; its tape is
`/media/fat/games/BasicMasterJr/mazy.s.bin`.

## Current development build

The 2026-07-27 build adds three completed roadmap items:

- Extended `JMP` (`$7E`) now takes exactly three 6800 clocks. The padded CPU suite reports all 200 measurable legal opcodes exact.
- `$E891` now colors a 32-pixel horizontal and 24-line vertical MP-1710 border around the unchanged 256x192 machine image, producing 320x240 active video. Screen reverse selects the opposite color nibble, matching bm2.
- Video now passes through MiSTer `video_mixer`, with forced scandoubling, HQ2x, CRT 25%/50%/75% scanlines, and gamma support. A 25 MHz video master provides four clocks per 6.25 MHz input pixel.

The border/video-mixer build was exercised on the MiSTer at `192.168.1.198`:
HDMI locked at 320x240 and `POKE 59537,7` changed the border to white. The
hardware screenshot endpoint also exposed a one-column character-rendering
regression. The corrected RBF described below is installed and hardware-tested:
A, M, and V match the ROM glyphs, HDMI remains locked at 320x240, and
`POKE 59537,7` produces a white border around the black 256x192 picture. The
pre-fix image remains backed up as `/media/fat/BasicMasterJr.pre-glyph-fix.rbf.bak`.

## Bugs fixed

### Character first-column rendering

The dual-port system RAM registers both its video-port address and output. The
pixel renderer originally waited only one cycle at each character boundary, so
the first column used the preceding character code. This removed the left stroke
from ROM glyphs whose bit 7 is set, most visibly A, M, and V. The renderer now
waits for both RAM stages before starting the character-ROM lookup, and the
framebuffer write enable follows the new final renderer state. All five screenshot
goldens were refreshed; their changes are confined to that formerly stale
character-cell column.

### Hardware CPU clock

`clk_cpu` is a one-cycle enable decoded from the 50 MHz divider, not a clock.
The old design drove it into CPU clock ports, creating an unconstrained fabric
clock that glitched on hardware and left the screen black. `rtl/core.v` now runs
`m6800_pad` and `m6801` on `clk_sys`, using `clk_cpu` only as `cen_in`.

### Tape download path

The original external-SDRAM path lost or misaligned bytes because the HPS write
strobe, address/data, and SDRAM handshake did not line up reliably. The final
path uses a dedicated 64 KiB dual-port block RAM:

- `hps_io` writes port A at full speed.
- The cassette reads port B at the selected authentic or fast timing.
- `ioctl_wait` is tied low.
- `sys/hps_io.sv` asserts `ioctl_wr` in the same cycle as `ioctl_addr` and
  `ioctl_dout`.
- Mounting a new image records its size and rewinds playback.

Temporary JTAG source/probe instances used to prove both byte streams were
removed from the release RTL after validation.

## Fast tape loading

The OSD exposes one compatibility control: **Fast tape loading: On / Off**. On is
the default. While the cassette motor runs, the CPU clock enable and tape bit rate
are both multiplied by 16, preserving the monitor timing ratio and all ROM filename,
block, and checksum handling. The CPU returns to 750 kHz immediately when the motor
stops. Off restores the original 300-baud tape timing.

## Hardware validation evidence

A diagnostic build exposed the tape RAM and CPU RAM over USB JTAG. The complete
tape-buffer dump matched `mazy.s.bin` exactly. After real-time playback, the CPU
RAM dump at `$1000-$25B7` also matched all 5,560 expected payload bytes with zero
differences. Live cassette state advanced to position 7,444, then stopped at the
monitor EOF boundary. The program was then started normally and rendered its
title screen.

The earlier apparent `* ERROR` was not a failed tape transfer. A clean replay
ended at the monitor prompt; the error came from an incorrect follow-up command.
Monitor commands are terminated with Space, not Return.

## Build and timing

Build command:

```bash
~/intelFPGA_lite/quartus/bin/quartus_sh --flow compile BasicMasterJr
```

The glyph-fixed border/mixer build completes with 0 errors and 53 warnings.
TimeQuest passes with +0.218 ns worst setup slack and +0.249 ns worst hold slack. The system and
video PLL outputs are declared asynchronous, matching the explicit two-flop
synchronizers and true dual-clock framebuffer at their boundaries. The RBF SHA-256
is `cee0eb4b1a7d10b6be1fcd1c325e60a7850b513fc853a30ea2dfdaa46492a1e9`.

The remaining warnings are benign MiSTer/Quartus artifacts: inferred-ROM write
pins, generated PLL nodes optimized away, deliberately idle SDRAM/ADC pins,
SD/USER_IO output enables, LogicLock Lite licensing, and broad fast-I/O assignments.

## Verification commands

```bash
cd vsim
make
./regression.sh --no-tape
./tape_test.sh full ../software/inufuto/mazy.s.bin MAZY
./tape_test.sh fast ../software/inufuto/mazy.s.bin MAZY

cd cputest
make
./obj_dir/Vcputb --pad
```

The Verilator top mirrors the hardware tape RAM rather than the obsolete
external-SDRAM tape path. Tape images are limited to 64 KiB.

## Remaining work

The glyph-fixed RBF is installed as `/media/fat/BasicMasterJr.rbf`; the pre-fix
release is recoverable as `/media/fat/BasicMasterJr.pre-glyph-fix.rbf.bak`. All
backup images use the `.rbf.bak` suffix so the MGL's name-only core lookup cannot
select them. mRExt
launched the exact RBF path, `/tmp/CORENAME` reports `BasicMasterJr`, and the
MiSTer process command line confirms `/media/fat/BasicMasterJr.rbf`.

The Verilator simulator now uses a timing-equivalent 25 MHz system timebase.
CPU, video, cassette, keyboard and IRQ-watchdog rates are scaled so emulated
time is unchanged, while a 120-frame headless run improved from 11.50 s to
6.37 s on the development host (10.4 to 18.8 fps, about 1.8x). Screenshot
goldens were refreshed because the asynchronous renderer phase changed; the
differences were deterministic and confined to scanline zero. The screenshot
suite, the 256-byte Fast Tape test and all 200 measurable padded CPU opcodes pass.

Open functional work is tracked in `TODO.md`: tape saving, printer/expansion
hardware, authentic mixed text/graphics composition, MP-9730 1200-baud tape,
MP-1803 floppy research, and tape-format/tool cleanup.
