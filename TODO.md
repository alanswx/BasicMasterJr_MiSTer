# TODO — Basic Master Jr. MiSTer core

Ordered by "what unblocks the most". Each item says what is wrong, what the correct
behaviour is, and where the evidence lives. Background:
[`ref/notes/hardware-reference.md`](ref/notes/hardware-reference.md) and
[`ref/notes/mame-vs-bm2-verdicts.md`](ref/notes/mame-vs-bm2-verdicts.md).

**Status:** BASIC and the monitor boot and take keyboard input. **Tape loading works
end to end** — `MAZY` loads from a real tape image, all 5,560 bytes byte-exact, and
runs. Sound, interrupts, the `$EFE0` graphics modes and blink attributes are
implemented. Remaining work is listed below.

Verify with:
```bash
cd vsim/ && ./regression.sh          # screenshots + tape load checks, ~10 min
cd vsim/ && ./regression.sh --no-tape # screenshots only, ~5 min
cd vsim/ && ./tape_test.sh full ../software/inufuto/mazy.s.bin MAZY  # + run it, ~30 min
cd vsim/cputest && make && ./obj_dir/Vcputb --pad   # CPU cycle accuracy
```

---

## P0 — open

- [ ] **`JMP` extended (`$7E`) runs 1 clock long.** The only remaining cycle
      inaccuracy: 4 clocks against the 6800's 3. `rtl/m6800_pad.v` can add clocks
      but not remove them, so this needs the same kind of state-machine fix that
      the accumulator-B ops got (drop a state). Everything else measurable is
      exact — see "CPU cycle accuracy" below.

---

## Done

P0 build/correctness (`sdram_wtbt` double drive, `$EFD0` read/write inversion,
colour RAM written on reads, 1-bit attribute latch), the whole P1 cassette list,
the 750 kHz CPU clock, IRQ/NMI, the 5-bit DAC, `$EFE0` mode decode, blink
attributes and `$E891`, the keyboard rewrite, and default-on 16x fast tape
loading. Three extra bugs were found and
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

- [ ] **Tape saving/output.** `$EE80` bit 0 is documented but ignored; there is no
      upload path that writes a saved tape image back to MiSTer.
- [ ] **Printer output and expansion PIA.** The printer ROM is now readable, but
      `$E800-$E803` printer/PIA behavior and actual output are not implemented.
- [ ] **MP-1803 floppy adapter** and **MP-9730 1200-baud cassette** support. Neither
      MAME nor bm2 emulates these fully; `bmtape2` is the reference for the 1200-baud
      tape format.
- [ ] **Visible MP-1710 border/background color.** `$E891` stores and reads back
      the value, but the renderer does not yet use `bckreg` outside the 256x192 image.
- [ ] **Authentic mixed text+graphics composition.** `$40-$4F` is supported using
      bm2's alternating-frame approximation; the real hardware overlay/interlace
      behavior remains unresolved.

## P4 — smaller / structural

- [ ] `$E800-$EDFF` is unconditionally I/O in `core.v:48`. bm2 agrees; MAME makes it
      RAM when the printer view is off. Low risk either way — noted so nobody
      "fixes" it toward MAME without reading `ref/notes/mame-vs-bm2-verdicts.md`.

- [ ] `wav2cas.py` has three different sample rates in one file (docstring 19200,
      function default 8000, `main()` default 19200). Settle it once the RTL rate is
      settled — or retire it in favour of `bmtape2`.

- [ ] No `video_mixer` in `BasicMasterJr.sv`: no scandoubler, scanlines, or gamma in
      the OSD, and analog VGA users get a raw 15 kHz signal.

---

## Simulation / tooling

- [x] Tape functional test: `vsim/tape_test.sh` loads an image through the monitor
      and diffs RAM against the image. `./tape_test.sh full` loads all of MAZY and
      runs it. Kept out of `regression.sh` because it takes minutes.
- [x] Test software: 22 inufuto homebrew titles in `software/inufuto/`, all
      checksum-verified. They are already in the byte-stream tape format, so they
      are the right target for the P1 rewrite. `software/bmtape_info.py` decodes
      and verifies them. See `software/README.md`.
- [ ] `vsim` runs at ~8 fps. If that becomes painful, the `video.v` framebuffer is
      the first thing to look at.
