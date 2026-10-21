# DRINK CAN (ドリンク・カン)

* **Title:** DRINK CAN (ドリンク・カン, "Drink Can")
* **Author:** 一夜 (Ichiya); the listing's REM credits "ONNANOKO SOFT"
* **Source:** マイコンBASICマガジン (Micom BASIC Magazine), March 1985, pp. 54-55,
  "ベーシックマスターJr.用"
* **Scan:** archive.org item `micomBASIC-1985-03`, leaves n69 (article, p.54) and n70
  (listing, p.55). Copies are in `software/found2/micombasic_archive_org/1985-03_drinkcan/`
  (n68 and n71 are neighbouring pages from other articles).
* **Files:** `drinkcan.bas` (transcription), `drinkcan.bin` (tape image, file `DRINKC.S`)

## Story / play

A mischievous "Mokomem" (♣) has got inside a juice vending machine. You (♠) move with
**A** (up), **Z** (down), **<** / **,** (left), **>** / **.** (right). Pick up a coin (●) and you turn
into ♥; then walk slowly up to one of the five drink switches [1]-[5] at the top. Touching
a number presses it (10 × round points). A can then drops into the slot at the bottom;
go back down and take it (another 10 × round). Five drinks clear the round.
Being caught by a Mokomem, having the can taken, or running out of time (the bar along
the top row) costs a chance; three chances. Title screen: press **S** to start.
Game over asks for your name if you beat the high score, then REPLAY Y / N.

The program sets colours through `$E890` (the colour-board register, `POKE A,...`),
so it is designed for the colour adaptor; it runs on a mono Jr too.

## Loading

No `CALL $E37E` is needed (text page only).

```
LOAD DRINKC
RUN
```

The tape file name is `DRINKC`, not `DRINKCAN`: the BASIC tape header holds only 6
characters (bmtape2 truncates `DRINKCAN` to `DRINKC.S`, and `LOAD DRINKCAN` then fails
with `ERROR 3`).

## Deviations / uncertain readings

No misprints needed fixing; every line is as printed.

* Line 140 `(C=$98)`: the last digit is smudged and could be read as `$93`; `$98` (♥, the
  player after taking a coin, set in line 130) is what the logic requires.
* Variable `O` (miss flag) in lines 80, 160, 270, 310 is the letter O (unslashed in print).
* Space counts inside strings were measured on the scan: line 620 `"GRAPE    COKE"` (4),
  line 650 5 spaces, line 890 `"[A]UP  LEFT RIGHT"` (2), line 940 `"  "` (2).
* MUSIC strings transcribed as printed (ﾐ printed as ≡, ﾄ as ト): line 340 `UﾌUﾚ`,
  400 `Uｿ`, 420 `Uｼ`, 450/500/800/980 as in the .bas. A misread note would only
  change the tune.
* DATA 1080-1130 (title-logo graphics, each char minus $30): rows of 17/17/17/11/11/11
  characters, consistent with "DRINK" and "CAN" laid out side by side in line 940.

## Verified in CSP

Loads cleanly at 300 baud, `RUN` shows "MAKE CHARACTER" and then the title screen
(colour "DRINK CAN" logo, ♠→♥ YOU / ● COIN / ♣ MOKOMEM, HI SCORE 320 MOMOKO, controls,
PUSH [S] KEY). S starts the game, and the board matches the magazine's photo 1
(APPLE ORANGE MILK / GRAPE COKE, switches 1-5, DRINK logo, coins, two ♣ chasing).
Keyboard input moved the player left and up; picking up a coin turned the ♠ into ♥;
being caught gave "OH! MISS", and CHANCE dropped from 3 to 2 on the redrawn board. Left
idle, the timer bar runs out and costs a chance too. No switch was pressed in testing
(the route up through the maze is long), so scoring and round clear were not exercised.
