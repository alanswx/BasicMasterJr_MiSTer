# TODO — Basic Master Jr. MiSTer core

Ordered by "what unblocks the most". Each item says what is wrong, what the correct
behaviour is, and where the evidence lives. Background:
[`ref/notes/hardware-reference.md`](ref/notes/hardware-reference.md) and
[`ref/notes/mame-vs-bm2-verdicts.md`](ref/notes/mame-vs-bm2-verdicts.md).

**Status:** BASIC and the monitor boot and take keyboard input. **Tape loading and
saving work end to end.** `MAZY` loads from a real tape image, all 5,560 bytes
byte-exact, and runs. All 22 inufuto titles load byte-exact and match the CSP
reference emulator pixel for pixel on their title screens. BASIC `SAVE` is recovered
from the real `$EE80` KCS waveform and flushed through MiSTer's save-sidecar
interface. Sound, interrupts, the `$EFE0` graphics modes, blink attributes, the
MP-1710 border and `video_mixer` are implemented. Remaining work is listed below;
`HANDOFF.md` has the current state.

Verify with:
```bash
cd vsim/ && ./regression.sh          # screenshots + tape load checks (7 tests)
cd vsim/ && ./regression.sh --no-tape # screenshots only
cd vsim/ && ./tape_test.sh full ../software/inufuto/mazy.s.bin MAZY  # + run it, ~15 min
cd vsim/cputest && make && ./obj_dir/Vcputb --pad   # CPU cycle accuracy
```

## Next up (small, from the 2026-10 session)

- [ ] **Hardware-test the tester RBF** (`~/dev2/BasicMasterJr_tester_20261021`, built
      from `1b100f6`). It is the first hardware build with the video RGB/blanking fix.
      Check that no column is lost at the picture edges and that the border is right.
- [ ] Run MISSILE FIGHT (`software/typein/missilefight_full.bin`) on the core. So far
      it has only run in CSP.
- [ ] Re-run Gorilla on the core with a longer key hold (`--key-hold 1500` or so). At
      120 ms it reached the title screen only, because its INKEY$ poll is about 1.5 s.
- [ ] Decide with the user how to commit the CSP-side bmjr fixes in
      `~/dev2/common_source_code` (see `vsim/csp/README.md`).
- [x] **Tape speed 32x, the tape overlay and Rewind tape** (2026-10-25) tested on
      mister14: MAZY loads in ~11 s and runs, Gorilla (BASIC) loads in ~20 s and
      runs in colour, the overlay sits in the bottom border, Rewind tape reloads
      from block 01. The first build crashed at 32x: CPU RAM port A had a
      registered output (two-clock reads) while the sim modelled one clock. Port A
      is now unregistered and the sim model matches both ports.
- [x] "Tape ready (rewound)" shows on hardware when the tape is mounted from the
      OSD file browser. After an MGL mount, Main's video-mode popup covers it.
- [ ] Run `vsim/csp/compare_tapes.py` over all 22 titles with a `make
      SIM_HZ=50000000` build (32x). Not done yet: the CSP build in the scratchpad
      had been cleaned away.
- [x] OSD and tape standards (2026-10-26), adopted where they make sense: `Tape`
      row name, rows in standard order, one `Reset and close OSD`, `Swap Alt/Win keys` (tested on
      mister14: Win types カナ記号 when swapped), Tape speed latched when the motor
      starts. Deliberately skipped: submenu pages and Apply and reset / Changes pending
      (few rows, no setting changes the running machine), Load ROM (ROMs are built
      in), Tape audio, quick start (no autoboot path). Deliberate deviations: turbo
      (250 ms) and the overlay (1.5 s) stay on after the motor stops, because BASIC
      stops it after every block.
- [x] **Type-next hint** (2026-10-27): with the deck stopped, the overlay shows
      `TYPE MON ↵ L MAZY ␣`, then `TYPE G 1000 ␣` at the end of the tape (`RUN` /
      `LOAD` for BASIC tapes, `E ↵ LOAD NAME ↵` between the two files of Tiny Galaxy).
      Checked in sim on MAZY (through to the game), Gorilla and Tiny Galaxy.
- [x] Reset row renamed `Reset and close OSD`, and it did nothing when selected: Main's
      Enter handler counts the undrawn `I,` line as a row, so the `I,` line now
      comes after it.
- [ ] Optional auto-type (like SharpMZ's `rtl/tape_autotype.sv`): from the tape
      header, type `MON` / `L NAME` Space / `G addr` Space for `.B` files, or
      `LOAD NAME` / `RUN` for `.S`. The tape format holds no start address; the first
      block's address is right for all 22 inufuto titles. Worth it if testers keep
      typing Return instead of Space.

## Implementation order

1. [x] Fix extended `JMP` (`$7E`) timing. It now completes in the 6800's three clocks; the complete padded CPU suite has no measurable fast or slow legal opcodes.
2. [x] Render the MP-1710 `$E891` background color as bm2's 32x24 border around the native 256x192 image.
3. [x] Revalidate and optimize simulation performance after the dual-port framebuffer work. The simulator now uses a timing-equivalent 25 MHz timebase and runs about 1.8x faster.
4. [x] Implement tape saving: `$EE80` bit 0 is latched, the ROM's 300-baud KCS
       waveform is decoded in CPU-state time (so Tape speed 32x is transparent), and
       complete bytes overwrite/append in the 64 KiB tape RAM. MiSTer's generic `FS1`
       save-sidecar path flushes occupied 512-byte sectors when the motor stops.
5. [x] Integrate MiSTer `video_mixer`: forced scandoubling, HQ2x, CRT scanlines, and gamma.
6. [ ] Implement printer compatibility and output capture, then the `$E800-$E803`
       expansion PIA; settle the `$E800-$EDFF` mapping while doing this. Feed the
       printer byte stream to the MiSTer printer server in
       `~/dev2/printeremulation` (its parallel transport drains an FPGA FIFO over
       `UIO_PRINTER_GET`; it renders PDFs to `/media/fat/printers/`).
7. [ ] Finish the gate-level trace of authentic mixed text+graphics composition,
       then implement it. Primary documentation now confirms `$40` and `$4C` as
       text/graphics "superimpose" modes and identifies the canonical `$0900`
       and `$2100` pages. Trace the IC79/IC80 HD74LS157 select signal to determine
       whether hardware switches sources per frame, scanline, character, or
       another interval before replacing the current bm2-style frame alternation.
8. [ ] Add authentic MP-9730 1200-baud cassette support, including its P-ROM
       and distinct tape block format, using `bmtape2` as the format reference.
       This is compatibility work, not a replacement for Tape speed 32x:
       turbo transparently accelerates the existing 300-baud machine,
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
attributes and `$E891`, the visible MP-1710 border, the keyboard rewrite,
`$EE80` tape output and KCS recording, `.bmt` native images,
default-on tape turbo (now 32x, with the tape overlay and Rewind tape), cycle-correct extended `JMP`, and MiSTer
`video_mixer` support, the timing-equivalent 25 MHz Verilator timebase, and RGB
aligned with blanking (it lagged by 1 px on hardware and 2 px in vsim; vsim also
dropped picture line 0).
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

- [ ] **MP-9730 1200-baud cassette.** This is an authentic optional device with
      its own P-ROM and a tape layout different from the standard 300-baud
      checksum-block format. It differs from default-on 32x Tape speed,
      which speeds the CPU and 300-baud tape together while preserving their
      emulated timing relationship. `bmtape2` is the format reference. Turbo
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
- [ ] **Printer output and expansion PIA.** The printer ROM is now readable, but
      `$E800-$E803` printer/PIA behavior and actual output are not implemented.
      Output goes to the MiSTer printer server (`~/dev2/printeremulation`),
      which accepts a parallel FIFO via `UIO_PRINTER_GET` or the HPS UART and
      renders PDFs; it would need a model for the Jr.'s printer.

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
