# WINE CELLAR (ワイン・セラー)

* **Title:** WINE CELLAR ("wine cellar"; the article calls it ワイン貯蔵室)
* **Author:** 一夜 (Ichiya); the listing's REM credits "ONNANOKO SOFT"
* **Source:** マイコンBASICマガジン (Micom BASIC Magazine), October 1984, pp. 56-57,
  "ベーシックマスターJr.用"
* **Scan:** archive.org item `micom-basic-magazine-issue-29-october-1984`, leaves n107
  (article + 4 screenshots, p.56) and n108 (listing, p.57, printed rotated 90°).
  The OCR hit at n65 is only the table of contents. Copies of n107/n108 are in
  `software/found2/micombasic_archive_org/1984-10_winecellar/`.
* **Files:** `winecellar.bas` (transcription), `winecellar.bin` (tape image, file `WINE.S`)

## Story / play

In the wine cellar live lively wine glasses that turn grapes into wine, but they won't
help you, so you throw grapes at them. Move with **A** (up), **Z** (down), **,** (left),
**.** (right); **Space** throws a grape in the direction you last moved. Hit a wandering or
warping glass and it turns wine-coloured and stops for a while; catch (walk into) it in
that time for 10 × round points. Once one glass is hit the others chase you, and you
can't throw another grape until the wine glass is caught or reverts. Touching a normal
glass, or running out of time (the ⊗ marker that crawls along the top wall), costs a
life; three lives. The magenta glasses down the right edge count the catches still
needed to clear the round. Game over returns to the title (asks NAME if you beat the
high score), then **Y** replays, **N** ends.

N (line 80) is the number of glasses (2); the article says 3 also works but is slow.

## Loading

The program uses the graphics page and its own character patterns, so the article says to
run `CALL $E37E` in direct mode **before typing it in** (and therefore before loading it):

```
CALL $E37E
LOAD WINE
RUN
```

Line 1100 POKEs 273 bytes of machine code and character data to `$3B10-$3C20` and checks
the sum against `$6AC9`. The article says: once the DATA is debugged, delete the `REM` in
line 1090 (the check then skips re-reading the DATA when it is already in memory). Don't
press BREAK during "READ DATA". If the program stops in graphics mode, `GOTO 50` returns to
the text screen.

## Deviations / uncertain readings

No misprints needed fixing; every line is as printed, including the `REM` in line 1090.

* The DATA in lines 1160-1340 is exactly 273 values and sums to `$6AC9`, matching the
  program's own checksum, so the machine code/character data transcription is confirmed.
* Variable `O` (miss flag) in lines 90, 150, 180, 260, 290 is the letter O (unslashed).
* String spacing measured from the scan: `"HI   SCORE:"` (3 spaces, lines 740/990),
  `"[A] UP  LEFT  RIGHT"`, `"[Z] DOWN [,]  [.]"`. Cosmetic only.
* MUSIC strings (ﾐ printed as ≡): 100 `T2P3ｿｿｿｿﾗﾗﾗﾗP5ｼP3ｼUﾚP7Uﾄ`, 170 `Uﾐ`, 400 `Uﾄ`,
  440 `Uｿ`, 470 `"Uｼ"`, 580 `"ｿ"`, 660 `T2P5ｼｿP7ﾌP3ｿﾗｼP7UﾄP3Uﾄ`,
  700 `T2P7UﾄｿP3ｼｼﾗP7UﾄP3Uﾄ`, 710 `T2P5RUﾄｿﾗｿﾌｿP7ﾐ`. The repeat counts in line 100
  (4×ｿ, 4×ﾗ) and the ｼ/ｿ distinctions are the least certain; a misread only changes a tune.

## Verified in CSP

Loads with `CALL $E37E` / `LOAD WINE` / `RUN`, prints READ DATA with no DATA ERROR, and
shows the title screen exactly like the magazine's photo 4 (YOUR SCORE 0, HI SCORE 210,
BEST:MOMOKO, key help, PUSH [Y] OR [N] ?). **Y** starts the game: the brick-walled field
with 20 random wine racks, the player, two glasses wandering and warping, and the
catches-remaining column, matching photo 1. With keys held (direct `bmjr --key-down/--key-up`;
the 4-frame taps of `typein.py --keys` are mostly missed by the slow BASIC loop) the player
moves in all four directions and Space throws a grape that flies until it hits a rack or wall.
I did not manage to land a grape on a glass in the automated runs, so the wine/catch/
score path was not exercised.

CSP does not advance BASIC's `TIME` (it has no timer interrupt, following MAME), so the
time-out marker never moves on its own there. Poking the time byte (`$000B`) showed the
marker move, the time-out → MUSIC → "YOUR REST 1 / YOUR SCORE / HI SCORE / NEXT ROUND"
screen, a redrawn field, and after the third life game over back to the coloured title.
On real hardware (or a core with the `$EFD0` timer IRQ) the timer should run normally.
