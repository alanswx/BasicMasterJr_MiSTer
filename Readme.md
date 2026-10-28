# Hitachi Basic Master Jr. (MB-6885) core for MiSTer

A MiSTer FPGA core for the Hitachi Basic Master Jr., a 1981 Japanese home computer:
HD46800 (6800-compatible) at 750 kHz, 32×24 text, 256×192 graphics, optional MP-1710
colour adapter, Kansas City Standard cassette at 300 baud.

**Working:** BASIC and the machine-language monitor, keyboard, 60 Hz timer interrupt,
BREAK key (NMI), 5-bit DAC sound, and cassette loading **and saving**. Real images
load byte-exact; BASIC SAVE is decoded from the machine's KCS output into a checksummed
native tape image.

**Not finished:** printer/expansion hardware, authentic mixed-mode composition, and
the optional MP-9730 cassette and MP-1803 floppy adapters. See [TODO.md](TODO.md)
for the ordered roadmap.

**Hardware verified:** the Quartus 17.0.2 build boots with stable HDMI at 320x240,
accepts keyboard input, mounts a tape through MiSTer, loads MAZY byte-for-byte and
runs it, and draws the MP-1710 border. In simulation, all 22 inufuto titles load
byte-exact and match the CSP reference emulator pixel for pixel on their title
screens. A tester build with the latest video fix is out for hardware testing.

---

## OSD

| Row | Values | Notes |
|---|---|---|
| Tape | `.bin` image | Mounting rewinds; the OSD says "Tape ready (rewound)" |
| Tape speed | **32x**, Normal | A change takes effect the next time the tape starts |
| Rewind tape | button | |
| Scandoubler Fx | None, HQ2x, CRT 25/50/75% | |
| Aspect ratio | Original, Full Screen, ARC1, ARC2 | |
| Tape overlay | **On**, Off | Status bar while a tape plays; what to type next while it is stopped |
| Swap Alt/Win keys | **No**, Yes | Alt is the machine's カナ記号 (Kana Shift) key; Yes moves it to the Win keys |
| Reset and close OSD | button | |

The rows follow `~/dev2/osd-standard.md` and `~/dev2/tape-standard.md` where they
make sense: the same names and order as the other computer cores, but one flat
page (there are only a few rows) and no "Apply and reset", because every setting
here acts at once without changing the running machine.

## Video options

The active picture is 320x240: the Basic Master Jr.'s native 256x192 image plus the
MP-1710's 32x24 border on each axis. The OSD offers **Scandoubler Fx** choices for
None, HQ2x, and CRT 25%/50%/75% scanlines. MiSTer gamma profiles are supported.

## Loading tapes

### On MiSTer hardware

1. Copy tape images (`.bmt`, legacy `.bin`, or a saved `.sav`) somewhere under
   `/media/fat/games/BasicMasterJr/`.
2. In the core's OSD choose **Tape** and pick the image.
3. Mounting rewinds the tape to the start, so you can reload the same image freely.

An MGL can launch the core and mount a tape in one step. For example, if the MGL
is stored in `/media/fat/`:

```xml
<mistergamedescription>
  <rbf>BasicMasterJr</rbf>
  <file delay="2" type="f" index="1"
        path="../../../../../media/fat/games/BasicMasterJr/mazy.s.bin"/>
</mistergamedescription>
```

Then drive the machine's own loader. Programs are almost all machine code loaded
from the monitor:

```
MON        <Return>      enter the machine-language monitor
L MAZY     <Space>       load a program by name -- terminated by SPACE, not Return
G 1000     <Space>       run it, from the program's entry address
```

The **Space, not Return** terminator is real — it is how the monitor works, and it
trips up everyone (MAME's driver notes it too). Type `L`, then the name, then Space.

**Tape speed** defaults to **32x** in the OSD. It runs the CPU and the serial tape
together 32 times faster while the tape loads, so the ROM still performs its normal
filename, block and checksum processing and sees exactly the bits it expects. MAZY's
7,446-byte image loads in about 8 seconds; a BASIC program such as Gorilla (13,929
bytes) in about 17. **Normal** is the original 300-baud timing (MAZY: about four and
a half minutes). Turbo stays on for a quarter of a second after the motor stops,
because BASIC's `LOAD` stops the motor after every block to store it; without that,
half of every BASIC load ran at normal speed. A program may therefore start a
fraction of a second fast after its last block. The screen lists blocks as they
load; `* ERROR` means the monitor rejected a command or detected bad tape data.

While the tape plays, **Tape overlay** (On by default) shows
`TAPE ▶ 0:52/ 4:33 x32` and a progress bar in the bottom-left corner: elapsed and
total time at the real 300 baud, and the speed. It disappears when the load is done.
**Rewind tape** puts the tape back at the start (mounting a tape also rewinds it, and
the OSD says "Tape ready (rewound)").

While the tape is stopped, the same corner says what to type next, read from the
tape's block headers: `TYPE MON ↵ L MAZY ␣` (or `TYPE LOAD GORILA ↵` for a BASIC
tape) before loading, and `TYPE G 1000 ␣` (or `TYPE RUN ↵`) once the tape has played
to the end, until that last Space or Return is pressed. On a tape holding a
machine-code file and then a BASIC one (Tiny Galaxy, MISSILE FIGHT) it shows
`TYPE E ↵ LOAD GALAXY ↵` between the two. The start address is the first block's
load address; the tape format has no start address, but that is right for every
machine-code tape we have. A BASIC program that switches to the graphics screen
(its text mentions `$EFE0`) needs `CALL $E37E` before it is loaded, and then the hint
takes two lines: `TYPE CALL $E37E ↵` / `THEN LOAD WINE ↵`. The call moves BASIC's
program area off graphics page 1. Returning from the monitor with `E` undoes it, so on a
tape with a machine-code file first the hint shows `TYPE E ↵ CALL $E37E ↵` /
`THEN LOAD GU ↵`.

BASIC programs (saved as ASCII, extension `.S` in the tape header) load with BASIC's
own `LOAD` instead of the monitor's `L`.

Tape images must be no larger than 64 KiB. Entry addresses differ per title —
`MAZY` runs at `$1000`, most others at `$2200`.
`software/bmtape_info.py` prints the name, entry address and checksum status of any
image:

```bash
./software/bmtape_info.py mytape.bin
```

### Recording tapes on MiSTer

The core records the real `$EE80` bit-0 KCS waveform; **Tape speed** 32x also
speeds up saving without changing the bytes. To make a fresh recording:

1. Copy [`software/blank.bmt`](software/blank.bmt) to the MiSTer with a unique name,
   then select it under **Tape**. MiSTer loads that image and mounts a
   growable save sidecar at `/media/fat/saves/BasicMasterJr/<name>.sav`.
2. Choose **New tape recording** in the OSD. This rewinds and clears the FPGA tape
   buffer; later motor starts append until this action is chosen again.
3. Use the machine normally, for example enter a BASIC program and issue `SAVE TEST`.
4. Wait for the cassette motor to stop. The core then flushes all recorded sectors to
   the `.sav` sidecar. Do not reset or power off before the motor stops.

The sidecar is itself a loadable tape image; `.sav` is accepted by the OSD. Rename it
to `.bmt` when moving it out of MiSTer's save directory. The last 512-byte sector is
padded with `$FF` leader bytes, so the on-disk file can be longer than the meaningful
checksummed blocks. Use a new blank filename when replacing a prior recording because
MiSTer's growable sidecars are not truncated when a shorter tape is written.

### In the simulator

```bash
cd vsim/
./obj_dir/Vemu --tape ../software/inufuto/mazy.s.bin \
               --send-keys '75:MON\n' --send-keys '200:L' \
               --send-keys '260:MAZY' --send-keys '340: ' \
               --screenshot 15400 --stop-at-frame 15410

./obj_dir/Vemu --headless --send-keys '75:10 PRINT "HI"\n' \\
               --send-keys '180:SAVE TEST\n' --record-out saved.bmt \\
               --stop-at-frame 700
```

### Getting tape images

What is in this repo, and where the rest comes from, is documented in
[`software/README.md`](software/README.md); briefly:

* **[inufuto](http://inufuto.web.fc2.com/8bit/)** — 22 homebrew games (+ UCHOTOS),
  actively maintained, published for dozens of retro machines. Take the `bm`
  (ベーシックマスター) build's `.s.bin` file: that is already the format this core
  wants. Not committed here (no licence stated); download them yourself.
* **`software/typein/`** — magazine type-ins as ready tape images: six transcribed
  from マイコンBASICマガジン scans (Graphic Helmet, Gorilla, MISSILE FIGHT, Wine
  Cellar, DRINK CAN, T.KUROKAWA) plus Graphic OPA and WILWESP from the
  [Typing Task Force / Gaming Alexandria](https://www.gamingalexandria.com/wp/type-in-programs/).
  Each has its listing, load commands and controls.
* **`software/found/`** — zu2's BASICMASTER repository (Tiny Galaxy, Dendai Tiny
  BASIC, LIFE2, …; `.cmt` files are the same byte stream and load as tapes).
* TOSEC has a Basic Master Jr. set, but it is a subset of inufuto's own downloads.
  (TOSEC's "Basic Master Jr. Level 3" set is really Level 3 software and does not run
  here.)

### Tape image format, and converting your own

`.bmt` is the canonical extension used by this core. Existing `.s.bin`/`.bin` images
contain exactly the same bytes and remain supported.

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

Quartus 17.0.x, as used by the verified hardware build:

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
* Homebrew test software by [inufuto](http://inufuto.web.fc2.com/8bit/) and
  [zu2](https://github.com/zu2/BASICMASTER)
* Takeda's [Common Source Code Project](https://takeda-toshiya.my.coocan.jp/) (eBASICMasterJr),
  used headless as a reference emulator (`vsim/csp/`)
* Type-ins from マイコンBASICマガジン (電波新聞社), scans preserved on archive.org,
  and the Typing Task Force / Gaming Alexandria
