# MISSILE FIGHT (ミサイル・ファイト)

* **Title:** MISSILE FIGHT (ミサイル・ファイト, "Missile Fight")
* **Author:** 鈴木 祐 (Yū Suzuki), credited on the title screen as "HOMC SOFT 1984" (HOMC is
  人吉鬼木マイコン・クラブ, the Hitoyoshi Oniki Microcomputer Club). Entry code BJ②, a "THE
  BEST PROGRAMMER" pick.
* **Source:** マイコンBASICマガジン (Micom BASIC Magazine), June 1984, pp. 56-57,
  "ベーシック・マスターJr.用"
* **Scan:** archive.org item `micom-basic-magazine-issue-24-june-1984`, leaves n91 (article,
  p.56) and n92 (p.57, List 1 BASIC and List 2 machine code, printed sideways). Copies are in
  `software/found2/micombasic_archive_org/1984-06_missilefight/`.
* **Files:**
  * `missilefight.bas`: List 1, the BASIC part (61 lines)
  * `missilefight_ml.hex`: List 2, the machine code, `$2500-$29EF` (1264 bytes), one row per
    printed row
  * `missilefight.bin`: tape image of the BASIC part only (`MISSIL.S`, built by `typein.py`)
  * `missilefight_full.bin`: tape image of both parts. It holds the machine code as monitor
    file `MISSML.B` (load address `$2500`), followed by `MISSIL.S`. bmtape2 built it from an
    S-record of the hex file and the listing.

## Play

A formation of 8 aliens attacks. Shoot them all with your unlimited energy shots. Each
round the aliens get smarter, fire more anti-missiles and fly faster. Clearing a round adds
a bonus to the score, and the bonus grows by 500 points each round.

* **,** moves left, **.** moves right, and **Space** fires. The machine code reads the key
  matrix through `$EEC0` directly, so you can move and fire at the same time.
* Title screen: press **Space** to start. You get 5 cannons ("LAST n" after each hit), then
  GAME OVER, then back to the title screen.
* The article's modifications: POKE `$25A3` with `$7F` for autofire (normally `$7C`).
  Alien flight patterns are the direction table at `$28E4` (bit 0 right, 1 left, 2 down,
  3 up).

(The printed "SPEAS" in the title screen text is the author's spelling of SPACE.)

## Loading

The game is mostly machine code. The magazine says to enter List 2 from the monitor
(`MON` Return), then type in List 1 in BASIC. Nothing reserves memory for the code, and
the BASIC program does not reach `$2500`. No `CALL $E37E` is needed.

With `missilefight_full.bin`:

```
MON             (Return)
L               then type MISSML and press Space (Space ends the name, not Return)
E               back to BASIC once "* INPUT COMMAND" returns
LOAD MISSIL
RUN
```

In CSP, typing `L MISSML ` in one go lost the name (the space right after `L` was taken as
an empty name). Type `L`, wait for `PROGRAM NAME :`, then type the name and press Space.
`E` returns to BASIC without touching `$2500-$29EF`.

The tape name is `MISSIL` because a BASIC tape name holds only 6 characters.

## Deviations / uncertain readings

The BASIC listing needed no fixes. All of it is entered as printed.

* Title-art strings (lines 70-160): the spaces were counted on a 33-pixel character grid
  laid over the scan. `@` and `*` become graphics blocks: the routine at `$28B6` turns `@`
  into `$9D` and `*` into `$60`. The result matches photo 1 in the magazine.
* Line 190: the dots in `LEFT ･･･ [,]` are raised middle dots, entered as the half-width
  katakana middle dot `･` (`$A5`). Line 200 `LAZER ･･` is the same.
* Line 250: 4 spaces between `000000` and `HI-SCORE`. Line 180: 2 spaces between `SOFT`
  and `1984`. Both were measured.
* Line 540 `MUSIC T3Q2P3ﾗ...`: the character after `T3` looks like `Q`, not `O`. BASIC
  accepts it, and the line runs at every game over without an error.
* MUSIC strings in lines 265 and 540 are entered as printed (ﾐ prints as ≡, ﾄ as ト). A
  misread note would only change the tune.
* REM lines 10/50/270 read `ｼｮｷ 1/2/3` (shoki, "initialise").

The machine-code listing has no checksum column, so it was checked by disassembling it:

* Every branch target lands on an instruction boundary.
* Every address the BASIC part CALLs (`$2500 $252C $253C $2667 $27F1 $289D $28B6 $28D0
  $28DE`) is the start of a routine.
* The article's facts match: `$25A3` holds `$7C`, and the flight table starts at `$28E4`.
* The table holds only valid direction codes (01/02/04/05/06/08/09/0A).
* Loading `MISSML` through the monitor from the generated tape gave a byte-exact
  `$2500-$29EF`.

The disassembly also shows the scheme. `$2000` holds the cannon column, `$2010-$2017` the
player shots, `$2018-$202F` the 8 aliens (x, y, pattern step), `$2030-$203F` the alien
missiles and `$2050-$206F` the star field. `$2040-$204C` hold the score and hi-score
digits. `$2100-$22FF` is a random table that line 20 fills.

## Verified in CSP

* `missilefight.bin` with the machine code poked to `$2500`: the title screen matches
  photo 1 (big MISSILE / FIGHT block letters, credits, key help). Space starts the game:
  "GAME START", a tune, then 8 aliens fly in over a star field.
* Hold `,` and `.` and the cannon moves. Fire with Space and the score counts up (10 a
  hit). Several waves were cleared. A screenshot caught "BONUS 1500" (the third
  wave: 500, 1000, 1500), and the score went past 5000.
* An alien missile hit shows "LAST 4". With `$200D` (missile rate) poked to `$FF`, all
  five lives went: LAST 4…1, then GAME OVER, which matches photo 4. The game returned to
  the title screen.
* The full tape path was checked end to end: monitor `L MISSML`, `E`, `LOAD MISSIL`,
  `RUN`. That game played to a score of 3260.
