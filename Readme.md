# Hitachi Basic Master Jr. (MB-6885) core for MiSTer

A MiSTer FPGA core for the Hitachi Basic Master Jr., a 1981 Japanese home computer:
HD46800 (6800-compatible) at 750 kHz, 32×24 text, 256×192 graphics, optional MP-1710
colour adapter, Kansas City Standard cassette at 300 baud.

**Working:** BASIC and the machine-language monitor, keyboard, 60 Hz timer interrupt,
BREAK key (NMI), 5-bit DAC sound, and **cassette loading** — a real tape image loads
byte-exact and the program runs.

**Not finished:** graphics-mode scanline placement is wrong, tape *saving* is not
implemented, and one CPU opcode is still a clock out. See [TODO.md](TODO.md).

> **Not yet built in Quartus.** Everything here is verified under Verilator only.
> The RTL is written to be synthesisable, but timing closure and real hardware
> behaviour are unverified.

---

## Loading tapes

### On MiSTer hardware

1. Copy tape images (`.bin`) somewhere under `/media/fat/games/BasicMasterJr/`.
2. In the core's OSD choose **Load Tape** and pick the image.
3. Mounting rewinds the tape to the start, so you can reload the same image freely.

Then drive the machine's own loader. Programs are almost all machine code loaded
from the monitor:

```
MON        <Return>      enter the machine-language monitor
L MAZY     <Space>       load a program by name -- terminated by SPACE, not Return
G 1000     <Space>       run it, from the program's entry address
```

The **Space, not Return** terminator is real — it is how the monitor works, and it
trips up everyone (MAME's driver notes it too). Type `L`, then the name, then Space.

Loading is real-time: 300 baud means a 5 KB program takes about four minutes, the
same as it did in 1981. The screen shows nothing while it loads; `* ERROR` means a
checksum failed.

BASIC programs (saved as ASCII, extension `.S` in the tape header) load with BASIC's
own `LOAD` instead of the monitor's `L`.

Entry addresses differ per title — `MAZY` runs at `$1000`, most others at `$2200`.
`software/bmtape_info.py` prints the name, entry address and checksum status of any
image:

```bash
./software/bmtape_info.py mytape.bin
```

### In the simulator

```bash
cd vsim/
./obj_dir/Vemu --tape ../software/inufuto/mazy.s.bin \
               --send-keys '75:MON\n' --send-keys '200:L' \
               --send-keys '260:MAZY' --send-keys '340: ' \
               --screenshot 15400 --stop-at-frame 15410
```

### Getting tape images

None are included — they are other people's software. The sources are documented in
[`software/README.md`](software/README.md); briefly:

* **[inufuto](http://inufuto.web.fc2.com/8bit/)** — 22 homebrew games, actively
  maintained, published for dozens of retro machines. Take the `bm` (ベーシックマスター)
  build's `.s.bin` file: that is already the format this core wants.
* **[Gaming Alexandria](https://www.gamingalexandria.com/wp/type-in-programs/)** —
  period magazine type-ins, as WAV recordings.
* TOSEC has a Basic Master Jr. set, but it is a subset of inufuto's own downloads.

### Tape image format, and converting your own

A tape image is **not audio**. It is the demodulated byte stream — exactly what the
CPU reads one byte at a time out of `$EE80`. The machine demodulates the tape in
hardware (an LM565 PLL FSK demodulator), so the core stores bytes and shifts them out
as a 300-baud serial line rather than playing samples.

To convert a WAV recording (or an S-record) into that format, use maruhiro's
`bmtape2`, vendored as an upstream tarball:

```bash
cd ref/emulators && tar xzf bmtape2src_20140321.tgz && cd bmtape2
make CFLAGS="-Os -Wall -Wno-implicit-function-declaration -Wno-deprecated-non-prototype" LDFLAGS=""
./bmtape2 -300 recording.wav        # -> recording.wav.bin, plus extracted files
```

It converts both directions, so it will also turn an image back into a WAV you can
play into real hardware.

---

## Building for MiSTer

Quartus 17.0.x or later, as usual for MiSTer:

```bash
quartus_sh --flow compile BasicMasterJr
```

`files.qip` lists the RTL. The ROMs are built in via `$readmemh` from
`rtl/rom0.mem` (character generator), `rtl/romb.mem` and `rtl/romdf.mem` (BASIC +
monitor) and `rtl/rome.mem` (printer), so no ROM file is needed at runtime — only a
tape image.

---

## Simulation

There is a full Verilator simulation in [`vsim/`](vsim/) — windowed with a debug
panel, or headless with screenshots, keystroke injection, memory dump/poke and
tracing. **Run it from `vsim/`**, since the ROM `.mem` files are opened relative to
the working directory.

```bash
cd vsim/
make
./obj_dir/Vemu                                   # windowed
./obj_dir/Vemu --help                            # all options
./regression.sh                                  # screenshots + tape load checks
./regression.sh --no-tape                        # screenshots only
```

`vsim/cputest/` separately checks the CPU core's timing, opcode by opcode, against
MAME's 6800 cycle table:

```bash
cd vsim/cputest && make && ./obj_dir/Vcputb --pad
```

This matters more than it sounds: the monitor times cassette bits with software
delay loops, so a CPU that runs fast makes tapes load at the wrong rate.

---

## Hardware reference

[`ref/`](ref/) holds the research this core was built from, with every hardware fact
tagged by where it came from — factory schematic, ROM disassembly, reference
emulator, or documentation.

* [`ref/notes/hardware-reference.md`](ref/notes/hardware-reference.md) — I/O map,
  cassette format, DAC, video modes, keyboard, interrupts.
* [`ref/notes/mame-vs-bm2-verdicts.md`](ref/notes/mame-vs-bm2-verdicts.md) — where
  MAME and the `bm2` emulator disagree, and which one the schematic or the ROM
  actually supports. Worth reading before "fixing" anything toward MAME.

The scanned manual and factory schematics are not committed (they are third-party
documents); [`ref/README.md`](ref/README.md) lists where to download each one.

See [CLAUDE.md](CLAUDE.md) for the development workflow.

---

## Credits

* Original core by **pcornier** — <https://github.com/pcornier/BasicMasterJr_MiSTer>
* `rtl/m6801.sv` derives from John Kent's 6800/6801 CPU core
* **maruhiro**'s [`bm2` emulator and `bmtape2`](https://ver0.sakura.ne.jp/pc/#bm2)
  were the reference for the cassette format and much of the I/O behaviour (BSD)
* MB-6885 factory schematics published by [旧コン研](http://kyucom.sblo.jp/article/190792009.html)
* Homebrew test software by [inufuto](http://inufuto.web.fc2.com/8bit/)
