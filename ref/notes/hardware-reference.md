# Hitachi Basic Master Jr. (MB-6885) — consolidated hardware reference

Every fact below carries a provenance tag so you can tell verified hardware behaviour from
emulator guesswork:

| Tag | Meaning |
|---|---|
| **[SCH]** | Read off the factory schematic (`ref/docs/MB-6885_schematic_a{1,2}.jpg`) |
| **[ROM]** | Disassembled from the monitor/BASIC ROM images in `rtl/*.mem` |
| **[BM2]** | maruhiro's `bm2` emulator source (`ref/emulators/bm2_official/`) |
| **[TAPE2]** | maruhiro's `bmtape2` tape tool source (`ref/emulators/bmtape2/`) |
| **[MAME]** | MAME `src/mame/hitachi/bmjr.cpp` |
| **[DOC]** | Vendor/reference documentation (系統図, magazine, Wikipedia, 旧コン研 system list) |

Confidence is highest where **[SCH]** or **[ROM]** agrees with two independent emulators.

---

## 1. System

| Item | Value | Source |
|---|---|---|
| CPU | Hitachi **HD46800** (MC6800-compatible), `IC1 HD46800DP` | [SCH] [DOC] |
| Clock | **750 kHz** (0.75 MHz) | [DOC] ×2 (Wikipedia spec table, 旧コン研 system list), [BM2] default `clock 750` |
| ROM | 18 KB total: BASIC 12 KB (`$B000-$DFFF`), printer 2 KB (`$E000-$E7FF`), monitor 4 KB (`$F000-$FFFF`) | [MAME] [BM2] |
| RAM | 16 KB standard, expandable to 64 KB (8 sockets; MP-9785 64 KB kit) | [DOC] |
| Text | 32 × 24 characters, 8×8 cells → 256×192 | [DOC] |
| Graphics | 256×192 dots, **2 pages** (Jr.-only addition over L2II) | [DOC] [BM2] |
| Colour | optional **MP-1710** colour adapter (¥10,000), RGB out | [DOC] |
| Cassette | **300 baud** standard; **1200 baud** via optional **MP-9730** P-ROM (standard on late "MB-6885T TURBO") | [DOC] [TAPE2] |

> MAME's `M6800(config, m_maincpu, 754'560)` carries its own `TODO: derive from actual clock /
> divider`. Two independent documentation sources and bm2 all say **750 kHz**. Treat 750,000 Hz
> as the target; MAME's 754,560 is an unverified guess (0.6% high).

---

## 2. I/O map (`$E800-$EFFF`)

The I/O window is **always decoded**, independent of the `$EFD0` RAM/ROM bank bits. [BM2]
(MAME instead lets `$E800-$EDFF` fall back to RAM when the printer view is off — a small
difference; bm2's behaviour matches what the core already does.)

| Addr | R/W | Function | Source |
|---|---|---|---|
| `$E802` | R/W | Line printer data | [BM2] |
| `$E803` | R/W | Line printer status / busy | [BM2] |
| `$E890` | W | **MP-1710** attribute latch — `[2:0]` fg colour, `[3]` blink fg, `[6:4]` bg colour, `[7]` blink bg | [BM2] |
| `$E891` | W | **MP-1710** screen background / border colour | [BM2] — *MAME has this as `// border color?` unimplemented* |
| `$E892` | W | **MP-1710** colour-RAM write enable (`bit0`) + marks adapter active | [BM2] |
| `$EE00` | R/W | Cassette **motor OFF / stop**, returns `$01` | [ROM] [BM2] [MAME] |
| `$EE20` | R | Cassette **motor ON / start**, returns `$01` | [ROM] [BM2] [MAME] |
| `$EE40` | W | Screen reverse, `bit7` | [ROM] [BM2] [MAME] |
| `$EE80` | R | **Demodulated** tape data on `bit 7` | [SCH] [ROM] |
| `$EE80` | W | `bit0` → tape output; `bits[5:1]` → 5-bit R-2R DAC → speaker | [SCH] [ROM] |
| `$EEC0` | R | Keyboard: `[3:0]` selected column, `[7:4]` modifiers | [SCH] [BM2] [MAME] |
| `$EEC0` | W | `[3:0]` keyboard column strobe, `bit7` = **NMI (BREAK) enable** | [ROM] [BM2] [MAME] |
| `$EF00` | R | Timer status, `bit7` tested (`BMI`) at `$F050` — both emulators stub it to `$FF` | [ROM] |
| `$EF40` | R | **IRQ acknowledge** — read immediately before `RTI` at `$F0B9` | [ROM] *(not implemented by MAME **or** bm2)* |
| `$EF80` | R | Interrupt-source flag, `bit7` tested (`BPL`) at `$F0BD` | [ROM] |
| `$EFD0` | R/W | RAM/ROM bank + timer enable (see below) | [ROM] [BM2] [MAME] |
| `$EFE0` | W | Screen mode (see below) | [DOC: manual `POKE $EFE0,0`] [BM2] |

### `$EFD0` — bank / timer control

```
bit 0 : 1 = $B000-$DFFF is RAM   (0 = BASIC ROM)
bit 1 : 1 = $E000-$E7FF is RAM   (0 = printer ROM)
bit 2 : 1 = $F000-$FFFF is RAM   (0 = monitor ROM)
bit 4 : 1 = disable the 60 Hz timer IRQ        <-- bm2: `if(!(ram_rom & 0x10)) m68irq()`
```
Reads return the last written value. Writes select. MAME and bm2 agree on bits 0-2; **bm2
additionally gates the IRQ on bit 4**, which MAME does not (MAME's comment says
`---x ---- timer enable?`). Writes to a ROM-mapped region still land in the shadow RAM
underneath in both emulators.

### `$EFE0` — screen mode

```
$00        : text
$40-$4F    : text + graphics page mixed  (bm2 alternates frames)
$C0-$CF    : graphics only
bits [3:0] : bitmap base = $0900 + $200 * (mode & 0x0F)
             page 1 = $0900 ($C0..$CB), page 2 = $2100 ($CC..$CF)
```
Bitmap layout is row-interleaved per character cell: byte at `base + col + row*8*32`, i.e.
the 8 scanlines of one cell are 32 bytes apart. Colour/blink attributes still come from the
per-cell colour RAM, so graphics mode is one fg/bg pair per 8×8 cell.

MAME implements only `bit7` (the `$C0` case) and flags `bit6` as
`<unknown, looks set in tandem with bit 7>` — the **`$40` mixed mode is missing from MAME**.
The `$900 + ((mode & 0xf) << 9)` base calculation is identical in both. [BM2] [MAME]

---

## 3. Cassette interface — the important part

### 3.1 Input is demodulated **in hardware**

The schematic (sheet 2/2, bottom right) shows the tape input path:

```
tape in ──R50 1k──┬── D1/D2 IS2076 clamp ──┬── IC41 LM565N (PLL FSK demodulator)
                  R51/R52 1k               │   R53 5K-B trimmer = VCO centre freq
                                           │   C115 0.001µ timing cap
                        IC41 pin 7 (RF out)─┴─ R54/R55/R56 4k7 + C117/C118/C119 0.01µ
                                              (3-pole post-detection LPF)
                                                 └── IC42 HD75108AP (line receiver/comparator)
                                                        └── TP10 ──> data bit to $EE80.7
```

**An LM565 is a PLL tone/FSK demodulator.** So `$EE80` bit 7 is a clean NRZ serial data
line, *not* raw tape audio. [SCH]

The monitor ROM confirms it. `$F794` is the bit sampler:

```
F794: 36        PSHA
F795: C6 07     LDAB #7            ; 7 samples
F797: 4F        CLRA
F798: 7D EE 80  TST  $EE80         ; test bit 7
F79B: 2A 03     BPL  $F7A0
F79D: 4C        INCA               ; count the highs
F79E: 20 03     BRA  $F7A3
F7A0: 01 01 01  NOP NOP NOP        ; timing balance
F7A3: 5A        DECB
F7A4: 26 F2     BNE  $F798
F7A6: 44 44 44  LSRA LSRA LSRA     ; C = bit2 of count => majority of 7
F7A9: 32        PULA
F7AA: 39        RTS                ; carry = received bit
```

A 7× oversampling **majority vote** only makes sense against a demodulated level. [ROM]

> **Consequence for this core:** feeding raw thresholded tape audio into `$EE80` (what
> `rtl/cassette.v` does today via `tape_bit_raw`) cannot work. The unused `kc_bit` output in
> that same file is the right idea.

### 3.2 Output is bit-banged **in software**

`$F531` (write a `1` bit) and `$F546` (write a `0` bit):

```
F531: 37        PSHB
F532: C6 10     LDAB #$10          ; 16 half-cycles  => 8 cycles of 2400 Hz  = '1'
F534: 37        PSHB
F535: C4 01     ANDB #$01          ; only bit 0 reaches the tape
F537: F7 EE 80  STAB $EE80
F53A: 33        PULB
F53B: 5A        DECB
F53C: 27 05     BEQ  ...
F53E: 8D 1A     BSR  <half-cycle delay>
F540: 7E F5 34  JMP  $F534
                                   ; $F546 is identical with LDAB #$08
                                   ; => 8 half-cycles = 4 cycles of 1200 Hz = '0'
```

`ANDB #$01` proves **tape out = D0**, which validates MAME's `BIT(data, 0)`. [ROM]

### 3.3 Kansas City Standard, 300 baud

From `bmtape2/kansas.c` `putWaveBit()`, cross-checked against the ROM timing above:

| Bit | Encoding at 300 baud | at 1200 baud |
|---|---|---|
| `1` | 8 cycles of **2400 Hz** | 2 cycles of 2400 Hz |
| `0` | 4 cycles of **1200 Hz** | 1 cycle of 1200 Hz |

Framing per byte (`putWaveByte`, matches `bm2/tape.c` `readTape`):

```
1 start bit (0) + 8 data bits LSB-first + 2 stop bits (1) = 11 bit-times/byte
```

bm2 clocks this at **2433 CPU states per bit**; at 750 kHz that is 3.244 ms ≈ 308 baud. [BM2]

### 3.4 Tape *image* format (the thing to load from SD)

bm2's tape file is **not audio** — it is the raw demodulated byte stream, one byte per 11 bit
times. `bmtape2` converts WAV ⟷ this byte stream ⟷ S-record/text. Recommended WAV for
capture is 44.1 kHz mono 8-bit; default FSK split frequency 1400 Hz. [TAPE2]

Block structure inside the byte stream, **300 baud**:

```
65 × $FF          leader
$01               block start
kind              $01 = binary, $10 = ASCII, $00 = end-of-file marker
asc_block         ASCII block number
8 bytes           file name (space padded; ext .B = binary, .S = ASCII/BASIC)
bin_block         binary block number
size              block length, 0 means 256
addr_hi, addr_lo  load address
hsum              header checksum: -(sum of the 12 header bytes) & 0xff
size × data
dsum              data checksum: -(sum of data) & 0xff
```

**1200 baud** (MP-9730) uses a different framing entirely:

```
255 × $55         leader
$AA               block start
begin_hi/lo       program area start
end_hi/lo         program area end
varbegin_hi/lo    variable area start (0 if none)
varend_hi/lo      variable area end
8 bytes           file name
data              program area, then variable area
$F0               terminator
```

Everything in `ref/emulators/bmtape2/bmtape2/bm.c`.

### 3.5 Motor control

`J2` (DIN) carries the tape signals, and `RL1` — a relay driven by `Q4 2SC1213` — is the
**cassette motor remote**. `$EE20` energises it, `$EE00` releases it. So motor state is a
**latch**, not a strobe. [SCH] [BM2] [MAME]

---

## 4. Sound

`IC40 HD74LS174P` (hex D flip-flop, ref `6G`) latches the `$EE80` write. [SCH]

```
Q(tape) ── R35 3.9k ──> tape output / J2
5 × Q   ── R36/R38/R40/R42/R44 20k series
        └─ R37/R39/R41/R43/R45 10k shunt      = 5-bit R-2R ladder
           └─ R46 1k / R47 1k / C111 0.01 ──> Q1 2SC458 → Q2 2SC1213 / Q3 2SA673 → SP1
              R60 20K-B = "VR 音量調整" front-panel volume pot
```

So: **1 bit tape out + 5-bit linear DAC**, exactly the structure MAME models
(`DAC_5BIT_BINARY_WEIGHTED` on `(data >> 1) & 0x1f`). MAME says binary-weighted where the
schematic shows R-2R, but both are linear 5-bit — no audible difference.
bm2's `writeVolume(x)` using all 8 bits is the cruder approximation here.

The core currently has **no audio at all** (`AUDIO_L/R` tied to 0).

---

## 5. Interrupts

* **IRQ** — 60 Hz timer, gated by `$EFD0` bit 4. Handler entry at `$F0BD` reads `$EF80` and
  branches on bit 7 to pick the timer path; `$F0B9` reads `$EF40` immediately before `RTI`
  (interrupt acknowledge). [ROM] [BM2]
* **NMI** — the BREAK key, enabled by `$EEC0` bit 7. [ROM] [BM2] [MAME]
* **RESET** — BREAK together with the modifier at `key_mod` bit 6 performs a hard reset. [BM2]

The core currently leaves `.irq()` and `.nmi()` **unconnected** on the CPU, so none of this
works: no `TIME`, no BREAK, no cursor blink.

---

## 6. Keyboard

`$EEC0` write bits `[3:0]` drive `IC396D` (6-bit latch, `16G`) → `IC37`/`IC38` HD74LS138
decoders → 13 column strobes `E1..E13`; rows return on `[3:0]` of a `$EEC0` read. [SCH]

Modifier bits on `[7:4]` of the read, all **active low**: [MAME] [BM2]

```
bit4  Normal     (needed to leave kana mode)
bit5  Shift
bit6  Kana Shift
bit7  Kana
```

`rtl/keyboard.v` implements Shift only — Normal/Kana/Kana-Shift and the cursor keys
(`@↑ [↓ ^→ ¥←`) are unmapped, and BREAK does not exist.

---

## 7. Colour (MP-1710) attribute semantics

Attribute byte per 8×8 cell, stored in a parallel 768-byte colour RAM written whenever the
CPU **writes** `$0100-$03FF` and `$E892` bit 0 is set: [BM2]

```
[2:0] foreground colour (BRG: bit0=B, bit1=R, bit2=G)
[3]   blink foreground
[6:4] background colour
[7]   blink background
```

Blink logic from `bm2/depend.c putpat()`:

```
blink phase on : if (attr & 0x08) attr &= 0x70;  else attr &= 0x77;
blink phase off: if (attr & 0x80) attr &= 0x07;  else attr &= 0x77;
```

Screen reverse (`$EE40` bit 7) is a **nibble rotate** of the whole attribute
(`attr<<4 | attr>>4`), so it swaps the blink bits along with the colours.

**MAME ignores bits 3 and 7 entirely — no blink attribute support.**

Palette: MAME's `BRG_3BIT` = `R=bit1, G=bit2, B=bit0`. The core's
`VGA_R = green; VGA_G = red;` swap in `BasicMasterJr.sv` compensates for `video.v`'s bit
order and lands on the same mapping — confusing but correct.
