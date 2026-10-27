# Reference emulator (Common Source Code Project)

`~/dev2/common_source_code` is Takeda's Common Source Code Project ported to macOS. Its `bmjr` target
(eBASICMasterJr) has a headless CLI for comparing against this core; see `MAC_BMJR_CLI.md` there.

```bash
./make_roms.py /tmp/bmjr-roms           # BASIC/MONITOR/PRINTER/FONT.ROM from ../../rtl/*.mem
cmake -S ~/dev2/common_source_code -B /tmp/csp-build -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/csp-build --target bmjr -j 8   # --target matters: the default builds every machine
/tmp/csp-build/bmjr.app/Contents/MacOS/bmjr --headless --rom-dir /tmp/bmjr-roms --out /tmp/run1 \
    --type '150:PRINT 123\n' --screenshot 400 --stop-at-frame 400
```

`--tape`, `--poke` and `--dump-mem` take the same arguments as `Vemu`. `.s.bin`, `.bin` and `.cmt` images load
directly. A full MAZY load at real 300 baud takes about 7 seconds. `--record-tape` writes what the ROM sends to
`$EE80` bit 0 as a WAV, which is the reference for the unimplemented tape save.

CLI gotchas:

* `--type FRAME:TEXT` uses `\n` for Return and the tokens `{KANA}`, `{EISU}`, `{SHIFT}x`, `{KANAKIGO}x`,
  `{WAITn}`.
* `--type-rate P:R` sets the press and release frames for every key, globally. The default is 4:4.
* Monitor `L` needs the `L`, the name and the Space typed as separate events with a gap between them. Typed in
  one string, the name is lost.
* `--screenshot` takes one frame per flag. Give the flag once for each frame you want.
* `--tape` and `--record-tape` can't be used together.
* CSP never advances BASIC `TIME`, because it has no timer IRQ.

## Tools here

* **`compare_tapes.py`** loads each image in both vsim and CSP and runs it. It diffs the loaded RAM against
  the image, compares screen RAM and pixels, and writes `report.txt` and `montage.png`. Last result: 22/22
  inufuto titles byte-exact, 0.0% pixel difference on title screens, at most 0.4% during gameplay.
  ```bash
  ./compare_tapes.py --csp /tmp/csp-build/bmjr.app/Contents/MacOS/bmjr --roms /tmp/bmjr-roms \
      --out /tmp/cmp ../../software/inufuto/*.s.bin
  ```
* **`typein.py build LISTING.bas NAME`** turns a UTF-8 BASIC listing (half-width kana) into an ASCII `.S`
  tape with `bmtape2`. The tape loads with `LOAD NAME`. NAME is at most 6 characters, because BASIC
  truncates longer names silently and the LOAD then fails.
  **`typein.py run TAPE NAME --csp … --roms … --out …`** boots BASIC in CSP, optionally types `--pre 'CALL
  $E37E'`, LOADs, RUNs, and prints each screenshot alongside the text page, so errors like `SYNTAX ERROR` stand
  out. BASIC games read INKEY$ rarely, so give them long holds with `--rate 60:20`.

## CSP's own bugs

CSP's bmjr VM had two colour bugs. Both are fixed in the CSP working tree, and **those changes are not
committed there yet**.

* `draw_screen` indexed `color_table[taddr]` with `taddr` starting at `$100`. Every cell took the colour 8
  rows below it. This was the main bug.
* `$E892` now follows bm2: bit 0 gates colour-RAM writes, and any write turns colour on.

Treat it as a second opinion, not an authority. Elsewhere it follows MAME, including on the points
`ref/notes/mame-vs-bm2-verdicts.md` settles against MAME (CPU clock, `$EF80`, no timer IRQ, no mixed mode).
