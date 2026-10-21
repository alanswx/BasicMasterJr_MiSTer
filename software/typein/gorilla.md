# ゴリラと探険隊 (Gorira to Tankentai, "The Gorillas and the Expedition")

* **Author:** BMファミリー・ミエ子
* **Source:** マイコンBASICマガジン, January 1984, pp. 56–58. archive.org item
  `micom-basic-magazine-issue-19-january-1984`, leaves n75 (article, variable table,
  photos 1–3), n76 (lines 10–790, article continued, program table, photo 4) and n77
  (lines 800–3110). The listing is printed rotated 90°. Scans are in
  `software/found2/micombasic_archive_org/1984-01_gorilla/`.
* **Files:** `gorilla.bas` (transcription, UTF-8 with half-width kana) and `gorilla.bin`
  (tape image, ASCII `.S` blocks named `GORILA`, built with
  `vsim/csp/typein.py build gorilla.bas GORILA`).

The article is headed "ベーシックマスターJr./LII₂ (カラー・アダプタ使用) 用", so it is
written for a machine with the MP-1710 colour adapter. `C=$E890` is the colour register.
It is the classic river-crossing puzzle: three explorers and three gorillas must cross a
river in a boat that holds two. Only one gorilla (the one holding the oar) can row, and
the gorillas attack if they outnumber the explorers on either bank. Two tunes play
throughout. The intro uses "Grand Canyon" and the main game uses "河は呼んでいる".
The article says the puzzle can be solved in 13 crossings.

## Loading

The game uses the text screen only, so no `CALL $E37E` is needed.

```
LOAD GORILA
RUN
```

The title screen explains the rules. Press `S` to start, then the explorers and
gorillas walk on.

**Controls.** For each crossing:

1. **Who boards first?** (`ﾀﾞﾚｶﾞ ﾉﾙ?`) Press `M` for an explorer, `K` for the gorilla
   that rows or `J` for a gorilla that can't row.
2. **How many?** (`ﾉﾙﾉﾊﾅﾝﾆﾝ?`) Press `1` or `2`.
3. **Who else?** (`ｱﾄﾊ ﾀﾞﾚ?`) Press `M`, `K` or `J`. This prompt only appears if you
   answered `2`.
4. **OK? (Y/N).** Press `Y` to go, or `N` to start the question over.

`P` gives up. On the `GAME AGAIN?` screen, `0` ends, `1` restarts from the title and
`2` restarts from the river.

The program reads the keyboard with `INKEY$` once per note of the background music,
which is roughly every 1.5 s. Hold each key down until the screen reacts.

## Differences from the printed listing

None. Every line is transcribed as printed, and nothing had to be changed for it to run.
Notes:

* **Lines 10 and 20** are overprinted on each other in the scan. They read
  `10 LET C=$E890:CALL $E14F:POKE C,7,0,1:GOTO 2000` and `20 LET I=0:REM %%% MAIN`.
  The variable table in the article confirms `C = $E890`.
* **Spaces inside strings** were counted by measuring the character pitch in the scan:
  * lines 150 and 190: 3 spaces
  * line 800: 4 spaces
  * line 2220: 3 spaces between `ﾀﾝｹﾝﾀｲ` and `ｷﾐﾀﾁ`
  * line 2290: 5 spaces between `ｺﾞﾘﾗ` and `ﾅﾚﾃｲﾙ`
  * line 2300: 3 spaces between `(ｺｹﾞﾅｲ)` and `ｶｽﾞｶﾞ`
  * line 2340: 2 spaces
  * the title strings in lines 2210 to 2330: 2 leading spaces

  The title screen in CSP lines up with photo 1.
* **MUSIC strings** use `ﾐ`, which prints like `≡`. Several strings begin with
  `Mﾄ`/`Mｿ`. BASIC accepts them as printed, and they were tested on their own in CSP.
  The `ｿ`/`ｼ` readings in the DATA lines are the least certain characters, because they
  look alike in this font. The DATA counts check out: 8 items for `M1$` (lines
  3000–3010), 32 for `M2$` (3020–3070) and 32 for `M3$` (3080–3110). These match the
  `READ` loops in lines 2140–2160.

## Verified

Verified in CSP (`vsim/csp/typein.py run`). Keys were held with `--key-down`/`--key-up`
for 180 frames each, because a 4-frame tap is missed by the once-per-note `INKEY$`.

* The program loads and RUNs without errors.
* The title text matches photo 1.
* `S` starts the walk-on animation.
* The river screen appears in colour: yellow explorers, red gorillas, a green boat and
  the bank. The `M/K/J/P` legend matches photo 2.
* The boat carries its passengers across, as in photo 3.

A full 13-crossing solution was played:

`K+J, K, K+J, K, M+M, M+J, M+K, M+J, M+M, K, K+J, K, K+J`

It ends with `ﾀﾞｲｾｲｺｳ ﾂｲﾆ ﾔｯﾀﾈ`, `BEST 13ｶｲ` and the `GAME AGAIN?` menu. Sending two
explorers first leaves one explorer with three gorillas. That gives the attack animation
and `ﾀﾝｹﾝﾀｲﾊ ｺﾞﾘﾗﾀﾁﾆ ｵｿﾜﾚﾁｬｯﾀ`, as in photo 4.
