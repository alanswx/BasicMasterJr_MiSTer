# Reference emulator (Common Source Code Project)

`~/dev2/common_source_code` is Takeda's Common Source Code Project ported to macOS. Its `bmjr` target
(eBASICMasterJr) has a headless CLI for comparing against this core; see `MAC_BMJR_CLI.md` there.

```bash
./make_roms.py /tmp/bmjr-roms           # BASIC/MONITOR/PRINTER/FONT.ROM from ../../rtl/*.mem
cmake -S ~/dev2/common_source_code -B /tmp/csp-build -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/csp-build --target bmjr -j 8
/tmp/csp-build/bmjr.app/Contents/MacOS/bmjr --headless --rom-dir /tmp/bmjr-roms --out /tmp/run1 \
    --type '150:PRINT 123\n' --screenshot 400 --stop-at-frame 400
```

`--tape`, `--poke` and `--dump-mem` take the same arguments as `Vemu`, and `.s.bin` images load directly. A full
MAZY load at real 300 baud takes about 7 seconds. `--record-tape` writes what the ROM sends to `$EE80` bit 0 as a
WAV, which is the reference for the unimplemented tape save.

Treat it as a second opinion, not an authority: it follows MAME on the points
`ref/notes/mame-vs-bm2-verdicts.md` settles against MAME (CPU clock, `$EF80`, no timer IRQ, no mixed mode).
