# Handoff — tape-support branch verified on MiSTer

Date: 2026-07-26. Branch: `tape-support`. Quartus Prime 17.0.2 Lite.

## Current result

The core now works on physical MiSTer hardware:

- HDMI locks and displays the native 256x192 picture.
- BASIC and MONITOR boot and accept keyboard input.
- An MGL launches the core and mounts `mazy.s.bin` through `hps_io` index 1.
- The complete 7,446-byte image reached the FPGA byte-for-byte.
- MONITOR loaded all 5,560 program bytes at `$1000-$25B7` byte-for-byte.
- `G 1000 <Space>` started MAZY and reached its `PUSH SPACE KEY` screen.

The installed test MGL is `/media/fat/.LASTLAUNCH.mgl`; its tape is
`/media/fat/games/BasicMasterJr/mazy.s.bin`.

## Bugs fixed

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
- The cassette reads port B at real 300-baud timing.
- `ioctl_wait` is tied low.
- `sys/hps_io.sv` asserts `ioctl_wr` in the same cycle as `ioctl_addr` and
  `ioctl_dout`.
- Mounting a new image records its size and rewinds playback.

Temporary JTAG source/probe instances used to prove both byte streams were
removed from the release RTL after validation.

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

The clean release build completes with 0 errors and 60 warnings. Twelve additional expected warnings report the deliberately
idle SDRAM address pins as constant. Its RBF SHA-256 is
`07126a0f7ccb6381a420085ba6a379715f546e5717899b8331818ed34a38fe3f`. The known
critical warning is the existing `clk_sys`/`clk_vid` renderer-memory crossing:
worst setup slack is -6.208 ns. Both clocks are PLL-derived; this remains an
open structural video issue documented in `TODO.md`. It did not prevent stable
HDMI or tape operation on the tested MiSTer.

## Verification commands

```bash
cd vsim
make
./regression.sh --no-tape
./tape_test.sh full ../software/inufuto/mazy.s.bin MAZY

cd cputest
make
./obj_dir/Vcputb --pad
```

The Verilator top mirrors the hardware tape RAM rather than the obsolete
external-SDRAM tape path. Tape images are limited to 64 KiB.

## Remaining work

The open functional items are unchanged and tracked in `TODO.md`: graphics-mode
scanline interleave, one-cycle-long extended `JMP`, tape saving, and structural
video-domain cleanup. No commit should claim those are fixed.
