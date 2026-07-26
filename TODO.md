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

- [ ] **Graphics-mode scanline interleave is wrong** (`rtl/video.v`). Mode select,
      page base and horizontal mapping are all correct and bitmap data renders,
      but vertical placement is off: poking `$2100`/`$2120`/`$2200` on page 2 puts
      full scanlines at y=0 and y=7 instead of y=0, y=1 and y=8. Deterministic
      repro in the task notes; suspect the renderer pipeline (the graphics path
      uses `ram_data` one state earlier than the text path uses `char_data`).

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
attributes and `$E891`, and the keyboard rewrite. Two extra bugs were found and
fixed along the way:

* **`sdram_inst.init` was tied to a reset that includes `ioctl_download`**, so the
  controller sat in `STATE_STARTUP` for the entire transfer and every byte of a
  tape image was discarded. `BasicMasterJr.sv` now has a separate `sdram_init`.
* **The keyboard used the PS/2 shift state to pick the matrix position** while
  also passing shift to the machine, so `:` `@` `=` `^` `_` could never be typed.
  `rtl/keyboard.v` now maps one PS/2 key per matrix position.

## P4 — smaller / structural

- [ ] `$E800-$EDFF` is unconditionally I/O in `core.v:48`. bm2 agrees; MAME makes it
      RAM when the printer view is off. Low risk either way — noted so nobody
      "fixes" it toward MAME without reading `ref/notes/mame-vs-bm2-verdicts.md`.

- [ ] `vma` is ignored: `core.v:104` connects it to an undeclared implicit wire and
      RAM writes (`ram.v` → `wren_a(wr)`) are not gated by it.

- [ ] `key_pressed` is an input to `core.v` that nothing uses.

- [ ] `video.v` renders into a full 256×192×3-bit framebuffer scanned asynchronously
      from the CPU. It works, but it is expensive, and the renderer pipeline is what
      the graphics-mode interleave bug lives in. The colour RAM is also written from
      the CPU clock domain and read in the video domain with no synchroniser.

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
