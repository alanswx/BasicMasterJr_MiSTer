# T.KUROKAWA (ティー・クロカワ)

*"バリバリのレーザー・シューティングだぞ"* ("A full-on laser shooting game"). It is a
side-scrolling shooter: your ship sits at the left edge, obstacles (green `+` lattices)
and enemies scroll in from the right, and you fire a long laser.

* **Author:** 黒川 智章 (Tomoaki Kurokawa; call sign 7K1PSV in the title screen)
* **Magazine:** マイコンBASICマガジン (Micom BASIC Magazine), December 1990, ランダム・プログラム
  section, entry code BJ-1, pp. 174-175
* **Scans:** archive.org item `basic-1990-12`, leaves n165 (article, p.174) and n166
  (listing, p.175). Copies are in
  `software/found2/micombasic_archive_org/1990-12_tkurokawa/`.
* **Machine:** ベーシックマスターJr. **with the colour adapter** (MP-1710). The program writes the
  colour register at `$E890` throughout.

## Loading and running

No `CALL $E37E` is needed; it uses the text screen only. Line 10 lowers the BASIC memory top to
`$3FA9`. Lines 730-800 then POKE a 47-byte machine-code scroll routine to `$3FB4-$3FE2`.

    LOAD KUROKA
    RUN

The tape name is `KUROKA` because BASIC's file-name field holds only 6 characters. At the
title screen, press any key. If the screen shows `DATA ERROR`, the checksum of the
machine-code DATA (lines 780-800) is not 5465. The transcription sums to 5465.

## Controls (from the article)

* **カナ記号** key: move up
* **カナ** key: move down
* **Space**: fire the laser

Picking up a power-up ("0" in the article) gives a twin laser. The game ends after 3 hits.
Above 999999 points it plays the ending (`THE END`).

The author warns that his code style often gives `ERROR 2` (probably lines too long to type in), so you should
type keywords in their abbreviated forms. Lines 730 and 750 must be typed exactly, or the
checksum check stops working. If you see `DATA ERROR`, check lines 780-800.

## Transcription notes

* There are no known misprints, and the listing is as printed.
* In this printer font, half-width `ﾌ` (fa) looks exactly like the digit `7`. Inside MUSIC
  strings, a `7` that follows `P` is a length digit (`P7`). Anywhere else it is `ﾌ`.
  * One spot needs care: line 250 `MUSIC T1P0Q2ﾌ` looks like `T1P0Q27` on the page. I first
    typed `7` there, and BASIC gave `SYNTAX ERROR`.
  * The manual's own MUSIC example (`MUSIC P0ﾌ`) shows the same glyph.
* `P0` in MUSIC strings is written with a zero. The font's 0 and O are nearly the same, and
  BASIC accepts `P0`.
* I took the asterisk counts from character-pitch measurements of the scan:
  * line 20: 31
  * line 70: 8
  * line 110: 7
  * line 130: 6
  * line 410: 5
  * line 840: 32
* Line 650 has 10 + 11 dots around `GAME OVER`. The dots in lines 30-50 and 650 are the
  katakana middle dot `･`, which sits at mid-height, unlike the period.
* Line 270 erases 9 cells (9 `E`s) and line 280 erases 10. Line 490 clears them again with 9
  and 10 `Z`s. I checked these against the line lengths.
* Line 440 reads `POKE P+Z,F,F,F`, with 3 F's (checked by line length).
* Line 550 `INT(7)` is as printed. It is probably meant to be `INT(RND(7))`, but it runs either way.
* Line 120 `MUSIC T1Q2P3ﾐｼUﾐﾐｼUﾐﾐｼRUｿUﾐUﾌUｿUﾗUｼUﾗUｿRR`: the ﾐ/ｼ/ｿ glyphs are small and some are
  uncertain. A wrong note there only changes the tune.

## Verified in CSP

* The tape (`software/typein/tkurokawa.bin`) loads with `LOAD KUROKA` and runs without
  errors.
* The title screen matches magazine photo 1.
* After a key press, the screen setup takes about 10 seconds of slow BASIC: it fills the
  border and clears 32 columns via the machine code. Then the game screen appears, with
  SCORE, HI-SCORE **10000 and `*BY T.KUROKAWA*`.
* **Up and down:** カナ moves the ship down and カナ記号 moves it up to the top row of the
  playfield.
* **Laser:** Space fires a red laser across the field.
* **Scoring and enemies:** the score went to 2000, then 5050, then 5150 as the laser hit
  things. Enemies fire their own lasers, and the green obstacle lattices scroll in. All of
  this matches photos 2-3.
* **Game over:** after three hits the `GAME OVER` banner and tune play (photo 4). A key then
  restarts the game.
* I did not try to reach the ending.
