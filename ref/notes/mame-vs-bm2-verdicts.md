# Where MAME, bm2, and the hardware disagree — and who to believe

The question behind this file: *don't clone MAME blindly.* For every behaviour the core needs,
we checked MAME's `bmjr.cpp` against maruhiro's `bm2` (an independent Japanese emulator whose
author reverse-engineered from real tapes and real hardware), the factory schematic, and the
monitor ROM itself.

Verdict key: **MAME correct** / **MAME incomplete** / **MAME wrong** / **unresolved**.

---

## 1. `$EE80` read — demodulated bit vs raw audio → **MAME correct**

| | |
|---|---|
| MAME | `kansas_r` samples audio at 40 kHz, `<12 ticks between transitions → 1`. Output is a per-bit level. |
| bm2 | Synthesises the serial line directly: start bit + 8 data LSB-first + 2 stop, 2433 CPU states/bit. |
| Hardware | `IC41 LM565N` PLL FSK demodulator → 3-pole LPF → `IC42 HD75108AP` comparator → data line. |
| ROM | `$F794` samples `TST $EE80` **seven times and majority-votes**. |

Two independent emulators, the schematic, and the ROM all agree. `$EE80` bit 7 is a clean NRZ
serial line. MAME's implementation is a shortcut (it re-derives the bit from audio because
MAME's cassette device stores audio) but the *interface it presents to the CPU is right*.

**Implemented:** `rtl/cassette.v` stores the decoded byte stream and shifts a clean NRZ
line into `$EE80`; no FPGA audio thresholding is involved. The reverse path latches
`$EE80.0` and decodes the ROM's KCS write cadence back into the same byte stream.

Better still for an FPGA: skip audio entirely and adopt bm2's model — store the byte stream
and shift it out as a UART at ~2433 CPU cycles/bit. No PLL, no filter, no WAV.

---

## 2. Tape output bit → **MAME correct**

MAME writes the cassette from `BIT(data, 0)`. Confirmed at ROM `$F535`: `ANDB #$01` right
before `STAB $EE80`. bm2 ignores the bit value and counts write *transitions*, which works but
tells you nothing. Schematic shows the 6th flip-flop of `IC40` going to `R35 3.9k` → J2.

---

## 3. Sound DAC width → **MAME correct** (bm2 is the sloppy one)

MAME: `dac->write((data >> 1) & 0x1f)` on a `DAC_5BIT_BINARY_WEIGHTED`, with its own comment
`TODO: unknown DAC type`. bm2: `writeVolume(x)` treating the whole byte as the level.

The schematic settles it: `IC40 HD74LS174P` drives an **R-2R ladder** built from five
20k series / 10k shunt pairs (`R36-R45`) into the audio amp, with the sixth output going to
tape. So it is a 5-bit linear DAC on `D[5:1]` — MAME's structure is right, only the label
("binary weighted" vs R-2R) is off, and that has no audible consequence.

For the core: `dac_level = cpu_dout[5:1]`, unsigned, zero-extended into `AUDIO_L/R`.

---

## 4. CPU clock → **MAME wrong (mildly); use 750 kHz**

MAME hardcodes `754'560` with `TODO: HD46800, derive from actual clock / divider`. Both the
Wikipedia spec table and the 旧コン研 system list say **0.75 MHz**, and bm2 defaults to
`clock 750` (kHz). Nothing corroborates 754,560.

Practical impact: tape bit timing. At 750 kHz, one 300-baud bit is 2500 CPU cycles
(bm2 uses 2433). The core's current `50 MHz / 74 = 675.7 kHz` is **10% slow** — that error is
far larger than the MAME/bm2 disagreement and will break tape decode on its own.

`50 MHz / 66 = 757.6 kHz` (+1.0%) is the best integer divider from the existing PLL;
a 48 MHz `clk_sys` with `/64` gives exactly 750,000 Hz and would also make
`rtl/cassette.v`'s existing `48MHz` constants correct.

---

## 5. `$EFD0` bit 4 (timer enable) → **MAME incomplete**

MAME comments `---x ---- timer enable?` and never uses it; its IRQ is an unconditional vblank
interrupt. bm2 gates the 60 Hz IRQ on `!(ram_rom & 0x10)`. Nothing contradicts bm2, and it
explains why the bit exists. Follow bm2.

Bits 0-2 (RAM/ROM banking) are identical in both and confirmed by the ROM's use of the region.

---

## 6. `$EFE0` mixed text+graphics mode → **MAME incomplete**

MAME only tests `BIT(screen_mode, 7)` and annotates bit 6 as
`<unknown, looks set in tandem with bit 7>`. The strongest primary source is the assembly
source of Hitachi's supplied colour-graphics utility, published in I/O May 1982. Its
five-entry `PAGMOD` table is:

```
$00  text
$C0  graphics page 1 ($0900)
$40  text + graphics page 1
$CC  graphics page 2 ($2100)
$4C  text + graphics page 2
```

The utility's `SCREEN` documentation calls `$40` and `$4C` **superimpose** modes. The
ordinary Hitachi manual documents only `$00`, `$C0`, and `$CC`, which explains why mixed
mode is easy to miss. Software that writes `$40` renders as plain text under MAME and as a
composite under bm2, so MAME is definitively incomplete.

MAME, bm2, and the CH32V203 BasicmasterEmulator all generalize the graphics base as
`$900 + $200*(mode & $0F)`. That formula is consistent, but the primary documentation
only names nibble `$0` and `$C` as the two full pages. Intermediate offsets should be
treated as emulator-supported/undocumented behavior, not fourteen additional pages.

**Composition remains unresolved, but the evidence has changed.** The CH32V203 emulator
ORs character and graphics bytes pixel by pixel. Conversely, the factory schematic appears
to feed those bytes through IC79/IC80 (HD74LS157 selectors) into one parallel-to-serial
video shift register, with no obvious eight-bit OR stage. bm2's alternating frames may
therefore represent authentic CRT-persistence multiplexing rather than a host-display
shortcut. Before changing the core, trace the selector control in `$40/$4C` and establish
whether its cadence is per frame, scanline, character, or pixel. The I/O article proves
that the intended result is superimposition, but does not state that cadence.

---

## 7. MP-1710 colour → **MAME incomplete on two counts**

1. **Blink attributes.** bm2 treats attribute bit 3 as "blink foreground" and bit 7 as
   "blink background" and drives them from a ~½ Hz blink phase. MAME masks the attribute to
   `fg = attr>>0 & 7`, `bg = attr>>4 & 7` and drops bits 3 and 7 on the floor. Since MAME
   also has no blinking cursor, bm2's reading is the only one that explains those bits.
2. **`$E891` background/border.** bm2 implements it as the screen background colour
   (default `0x70` with no adapter). MAME has `//map(0x01, 0x01) border color?` — commented out,
   and lists "Border color for MP-1710" as an open TODO.

Also different: MAME gates *display* on `color_mode & 1` and writes colour RAM unconditionally;
bm2 gates colour-RAM *writes* on `$E892` bit 0 (`wtenbl`) and switches the display on once
`$E892` has been written at all. bm2's reading matches the register's Japanese name
(色情報書き込み許可 = "colour info write enable"), so prefer bm2. **The core currently follows
MAME here and additionally has a bug: it latches colour RAM on reads of `$0100-$03FF` too.**

---

## 8. `$EF80` — **unresolved; treat MAME with suspicion**

MAME maps `$EF80` to a port whose bit 7 is the **BREAK key**, and separately fires NMI from
that key. bm2 returns a constant `$80` with the comment `???`.

The ROM says it is an **interrupt-source flag**: `$F0BD` does `LDAA $EF80 / BPL` to choose
between the 1-second timer at `$000C` and the `$000A-$000B` counters — which is exactly what
MAME's own comment describes. Meanwhile BREAK's real path to the CPU is NMI, gated by `$EEC0`
bit 7, which both emulators implement.

So MAME is probably conflating a timer status line with the BREAK key. The core returns `$00`
today (= MAME with BREAK released) and BASIC boots, so this is low-risk — but don't wire the
BREAK key to `$EF80`. Wire it to NMI.

---

## 9. `$EF40` read → **missing from both emulators**

`$F0B9: LDAA $EF40` sits immediately before an `RTI` in the interrupt handler — the classic
shape of a read-to-clear interrupt acknowledge. Neither MAME nor bm2 decodes `$EF40` at all
(MAME only has `$EE40` for screen reverse; note the different address). If the core ends up
with a latched timer IRQ, this is where it gets cleared.

---

## 10. Things MAME and bm2 agree on — just implement them

* `$EE00` / `$EE20` are motor **stop/start** and both return `$01`.
* `$EEC0` bit 7 = NMI enable; clearing it clears a pending NMI.
* `$EE40` bit 7 = screen reverse, implemented as a nibble swap of the attribute.
* Bitmap base `$900 + ((mode & 0xF) << 9)`, row-interleaved 8 scanlines × 32 bytes apart.
* Attribute layout `fg=[2:0]`, `bg=[6:4]`, BRG bit order.
* Writes to ROM-mapped regions still reach the RAM underneath.

## 11. Things neither emulator implements

Expansion-bus PIA at `$E800-$E803` (beyond bm2's printer), the **MP-1803** floppy adapter,
the **MP-9730** 1200-baud cassette ROM (only `bmtape2` understands its tape format), and
real printer output (MAME declares `feature::PRINTER` unemulated; bm2 writes to a text file).

---

## Bottom line for this core

Follow **MAME** for: `$EE80` read semantics, tape-out bit 0, 5-bit DAC on `D[5:1]`, bank bits 0-2.

Follow **bm2** for: 750 kHz clock, `$EFD0` bit 4 timer gating, `$E891` background colour,
`$E892` as a write-enable, blink attributes, the `$40` mixed screen mode, and — most
importantly — the **byte-stream tape image format** instead of streaming audio samples.

Follow the **ROM/schematic** for: `$EF40` interrupt acknowledge, `$EF80` as a timer flag
rather than the BREAK key.
