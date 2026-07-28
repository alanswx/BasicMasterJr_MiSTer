# TODO — Basic Master Jr. MiSTer core

Ordered by "what unblocks the most". Each item says what is wrong, what the correct
behaviour is, and where the evidence lives. Background:
[`ref/notes/hardware-reference.md`](ref/notes/hardware-reference.md) and
[`ref/notes/mame-vs-bm2-verdicts.md`](ref/notes/mame-vs-bm2-verdicts.md).

**Status:** BASIC and the monitor boot and take keyboard input. **Tape loading and
saving work end to end in simulation and on physical MiSTer hardware**: MAZY loads
byte-exact, while BASIC SAVE is recovered from the real `$EE80` KCS waveform and
flushed through MiSTer's generic save-sidecar interface. The 512-byte hardware
sidecar contains four checksum-valid BASIC blocks followed by `$FF` sector padding.
Sound, interrupts, `$EFE0` graphics modes and blink attributes are implemented.
Remaining work is listed below.

Verify with:
```bash
cd vsim/ && ./regression.sh          # screenshots + tape load checks, ~2 min
cd vsim/ && ./regression.sh --no-tape # screenshots only, ~1 min
cd vsim/ && ./tape_test.sh full ../software/inufuto/mazy.s.bin MAZY  # + run it, ~15 min
cd vsim/cputest && make && ./obj_dir/Vcputb --pad   # CPU cycle accuracy
```

## Implementation order

1. [x] Fix extended `JMP` (`$7E`) timing. It now completes in the 6800's three clocks; the complete padded CPU suite has no measurable fast or slow legal opcodes.
2. [x] Render the MP-1710 `$E891` background color as bm2's 32x24 border around the native 256x192 image.
3. [x] Revalidate and optimize simulation performance after the dual-port framebuffer work. The simulator now uses a timing-equivalent 25 MHz timebase and runs about 1.8x faster.
4. [x] Implement tape saving: `$EE80` bit 0 is latched, the ROM's 300-baud KCS
       waveform is decoded in CPU-state time (so 16x Fast Tape is transparent), and
       complete bytes overwrite/append in the 64 KiB tape RAM. MiSTer's generic `FS1`
       save-sidecar path flushes occupied 512-byte sectors when the motor stops.
5. [x] Integrate MiSTer `video_mixer`: forced scandoubling, HQ2x, CRT scanlines, and gamma.
6. [ ] Implement printer compatibility and output capture, then the `$E800-$E803`
       expansion PIA; settle the `$E800-$EDFF` mapping while doing this. Capture
       the original printer byte stream as a MiSTer save-sidecar `.prn` file;
       there is no generic MiSTer printer/PDF service.
7. [ ] Finish the gate-level trace of authentic mixed text+graphics composition,
       then implement it. Primary documentation now confirms `$40` and `$4C` as
       text/graphics "superimpose" modes and identifies the canonical `$0900`
       and `$2100` pages. Trace the IC79/IC80 HD74LS157 select signal to determine
       whether hardware switches sources per frame, scanline, character, or
       another interval before replacing the current bm2-style frame alternation.
8. [ ] Add authentic MP-9730 1200-baud cassette support, including its P-ROM
       and distinct tape block format, using `bmtape2` as the format reference.
       This is compatibility work, not a replacement for Fast Tape Loading:
       Fast Tape transparently accelerates the existing 300-baud machine,
       whereas MP-9730 runs the CPU normally and implements the real 1200-baud
       protocol.
9. [ ] Research the MP-1803 floppy adapter before implementation. Locate and
       preserve its technical documentation/ROM, the MP-3370 disk geometry, and
       at least one known-good Jr. disk image (ideally MA-5380 Disk BASIC).
       No verified public Jr./MP-1803 image corpus is currently known; Level 3
       disk archives are not evidence of Jr. compatibility.
10. [x] Standardize the decoded Basic Master tape byte stream as `.bmt`. Legacy
        `.bin` and MiSTer `.sav` sidecars remain loadable. WAV stays the archival
        audio master and is converted off-core with `bmtape2`; FPGA WAV parsing is
        deliberately out of scope.

---

## Done

P0 build/correctness (`sdram_wtbt` double drive, `$EFD0` read/write inversion,
colour RAM written on reads, 1-bit attribute latch), the whole P1 cassette list,
the 750 kHz CPU clock, IRQ/NMI, the 5-bit DAC, `$EFE0` mode decode, blink
attributes, `$E891`, the visible MP-1710 border, the keyboard rewrite, `$EE80` tape output and KCS recording, `.bmt` native images,
default-on 16x fast tape loading, cycle-correct extended `JMP`, and MiSTer
`video_mixer` support, and the timing-equivalent 25 MHz Verilator timebase.
Three extra bugs were found and
fixed along the way:

* **The renderer clock-domain crossing and graphics pipeline were rebuilt.** The
  character renderer now runs with system RAM and the character ROM on `clk_sys`,
  waits for both synchronous read latencies, and writes a true dual-clock M10K
  framebuffer scanned by `clk_vid`. This removes the two failing CDC paths and
  fixes the one-request-early graphics scanline data. Quartus now passes setup
  and hold timing.

* **The original external-SDRAM tape path lost or misaligned download bytes.**
  Tape images now download into a dedicated 64 KiB dual-port block RAM, and
  `hps_io` asserts `ioctl_wr` in the same cycle as its address and data. This was
  verified on MiSTer by dumping all 7,446 MAZY image bytes and all 5,560 loaded
  program bytes over JTAG.
* **The keyboard used the PS/2 shift state to pick the matrix position** while
  also passing shift to the machine, so `:` `@` `=` `^` `_` could never be typed.
  `rtl/keyboard.v` now maps one PS/2 key per matrix position.

## Unimplemented or incomplete hardware features

- [ ] **Printer output and expansion PIA.** The printer ROM is now readable, but
      `$E800-$E803` printer/PIA behavior and actual output are not implemented.
      On MiSTer the useful result is software compatibility plus a captured raw
      `.prn` file retrieved through the same generic `FS` save-sidecar path now used by tape recording. MiSTer has no
      generic printer spooler or PDF renderer; conversion or physical printing
      belongs on the host. A UART output option could follow, but is secondary.
- [ ] **MP-9730 1200-baud cassette.** This is an authentic optional device with
      its own P-ROM and a tape layout different from the standard 300-baud
      checksum-block format. It differs from default-on 16x Fast Tape Loading,
      which speeds the CPU and 300-baud tape together while preserving their
      emulated timing relationship. `bmtape2` is the format reference. Fast Tape
      could also accelerate MP-9730 in wall-clock time after it is implemented.
- [ ] **MP-1803 floppy adapter research.** The adapter connected the MP-3370
      3-inch compact floppy drive, and MA-5380 Disk BASIC existed, but no
      verified public Basic Master Jr./MP-1803 disk-image set is currently
      known. Do not guess at an untestable controller. Resume implementation
      after obtaining adapter documentation/ROM, disk geometry, and known-good
      Jr. media. Basic Master Level 3 disk images may aid research but must not
      be assumed compatible.
- [ ] **Authentic mixed text+graphics composition.** I/O May 1982 publishes
      Hitachi's utility and confirms `$40` = text + page 1 and `$4C` = text +
      page 2, explicitly described as "superimpose." The factory schematic routes
      character and graphics bytes through IC79/IC80 selectors into one video
      shift register, so a direct pixelwise OR is not apparent and bm2's
      alternating frames may be hardware-authentic rather than an approximation.
      Trace the selector control and establish its switching cadence before
      changing the renderer.

## P4 — smaller / structural

- [ ] `$E800-$EDFF` is unconditionally I/O in `core.v:48`. bm2 agrees; MAME makes it
      RAM when the printer view is off. Low risk either way — noted so nobody
      "fixes" it toward MAME without reading `ref/notes/mame-vs-bm2-verdicts.md`.

- [x] **Tape format/tool cleanup.** `.bmt` now names the compact demodulated byte
      stream; `.bin` remains compatible and `.sav` permits direct sidecar reloads.
      The core records that stream directly and documentation points WAV conversion
      to `bmtape2`. `wav2cas.py` is retained only as historical tooling until a
      separate cleanup removes it.

---

## Simulation / tooling

- [x] Tape functional test: `vsim/tape_test.sh` loads an image through the monitor
      and diffs RAM against the image. `./tape_test.sh full` loads all of MAZY and
      runs it. Kept out of `regression.sh` because it takes minutes.
- [x] Test software: 22 inufuto homebrew titles in `software/inufuto/`, all
      checksum-verified. They are already in the byte-stream tape format, so they
      are the right target for the P1 rewrite. `software/bmtape_info.py` decodes
      and verifies them. See `software/README.md`.
- [x] **Simulation performance.** Verilator now uses a simulation-only 25 MHz
      system timebase, halving idle system-clock evaluations while retaining
      enough bandwidth for the six-cycle renderer. CPU, video, cassette,
      keyboard injection and the IRQ watchdog remain scaled to real time; the
      hardware default stays 50 MHz. On the development host, a 120-frame
      headless run improved from 11.50 s (10.4 fps) to 6.37 s (18.8 fps), about
      1.8x. Reproduce with `time ./obj_dir/Vemu --headless --stop-at-frame 120`.
      The timebase changes the asynchronous framebuffer phase, so the golden
      images were refreshed after verifying that all changes were confined to
      scanline zero and were deterministic.
