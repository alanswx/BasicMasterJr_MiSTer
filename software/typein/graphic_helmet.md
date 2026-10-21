# グラフィック・ヘルメット (Graphic Helmet)

* **Author:** 中村賢之
* **Source:** マイコンBASICマガジン, December 1982, pp. 36–39. archive.org item
  `micom-basic-magazine-issue-6-december-1982`, leaves n51–n54. Scans are in
  `software/found2/micombasic_archive_org/1982-12_graphic_helmet/`.
* **Files:** `graphic_helmet.bas` (transcription, UTF-8 with half-width kana) and
  `graphic_helmet.bin` (tape image, ASCII `.S` blocks named `HELMET`, built with
  `vsim/csp/typein.py build graphic_helmet.bas HELMET`).

A Jr-only BASIC game on the graphics page, after Nintendo's Game & Watch *Helmet*. Walk
from the left house to the right one while tools fall. A door on the right house opens
and closes. Get hit three times and the game is over.

## Loading

The program uses the graphics page. The article says to run `CALL $E37E` before typing
or loading it, and that without it the machine runs away. Then:

```
CALL $E37E
LOAD HELMET
RUN
```

After the title screen and `WAIT!` (about 40 s), press `S` to start.

**Controls:** `,` moves left and `.` moves right. After the game ends, `Y` plays again
and `N` stops. If it stops with an error, `GOTO 10000` (or `POKE $EFE0,0`) returns to
text mode.

## Differences from the printed listing

* **Line 90:** printed as `READ Z(I,1);Z(I,2)`. BASIC stops there with `SYNTAX ERROR`,
  so it is `,` here.
* **Line 690:** the last character of `MUSIC T3P4U#ｼ` is faint in the scan; it reads as `ｼ`.
* **Line 1170:** `MUSIC O4T1P6ﾄﾄP3ﾄP7ﾄBﾐP3ﾚP6ﾚP3ﾄP6ﾄP3DｼP7ﾄQ0`. The printer's `ﾐ`
  looks like `≡`. The notes are the opening of Chopin's funeral march, which confirms
  the reading.

## Verified

In CSP (`vsim/csp/typein.py run ... --pre 'CALL $E37E'`): it loads, shows the title and
key help, draws the scene (a starfield, two houses and the ground), waits for `S`, then
plays. Tools fall, the man walks with `,` and `.`, and `死` appears when he is hit. These
screens match the magazine's photos 1–3.
