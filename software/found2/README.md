# found2: second sweep for Basic Master Jr. software (2026-10-21)

This sweep looked for software that runs on the Hitachi Basic Master Jr. (MB-6885),
with the emphasis on type-in programs. It skips everything already in
`software/inufuto/`, `software/found/` and `software/typein/`. Nothing here has been
executed. Each source has its own subdirectory.

**Short version:** very few ready-to-load images exist beyond what we already had.
The large find is that **archive.org holds Micom BASIC Magazine (マイコンBASICマガジン,
"ベーマガ") scans covering every month from July 1982**, and I/O scans for 1978–1989.
There is also a per-issue index of every Basic Master L2/Jr program that ベーマガ
featured. Together these give about 115 type-in titles with known issue months. For
most of them, the OCR also points to the archive.org page they are on.

## Downloaded

| dir | item | format | load / entry | type-in? | licence / redistribution |
|---|---|---|---|---|---|
| `typingtaskforce_gamingalexandria/` | **Graphic OPA** (一夜/Kazuya), Micom BASIC 1984-12, pp. 56–57 | zip: `.cas` (CSP-style sampled cassette, 0x7F fill, **not** the bm2 byte stream), 18 MB `.wav`, readme, 3 screenshots, 2 page scans | BASIC; `CALL $E37E` first, then `LOAD OPA`, `RUN` | yes (BASIC magazine listing) | Typing Task Force / Gaming Alexandria preservation release; no licence stated (magazine copyright) |
| ″ | **WILWESP** (ORANGE JUICE), Micom BASIC 1984-12 | zip: 23.6 MB `.wav` + readme; separate zip with the 2 page scans (pp. 54–55) | BASIC; `LOAD WILWSP`, `RUN` | yes | same as above. This WAV is the single entry in MAME's `bmjr_cass` list (crc d46d46e6) |
| `micombasic_archive_org/1982-12_graphic_helmet/` | **グラフィック・ヘルメット** (中村賢之), Micom BASIC 1982-12, pp. 36–39 | 4 full-resolution page JPGs (archive.org leaves n51–n54) | BASIC (graphics page); run `CALL $E37E` before typing or LOADing; Jr-only | yes, listing only, **needs typing in** | magazine scan from archive.org item `micom-basic-magazine-issue-6-december-1982`; magazine copyright |
| `micombasic_archive_org/bemaga_bm_index.tsv` | Index of every BM L2/Jr program in ベーマガ 1982-07 … 1990-12, each with its archive.org item and OCR page-leaf hits | TSV | – | – | titles from s-sasaji's list (see below); page hits are my own OCR search |
| `zu2_BASICMASTER_additions/hires/` | zu2 hi-res graphics routine for the Jr (GET/PUT, string drawing on the 256×192 page) | `.asm` + `.mot` (S-record, ORG $1000/$1200/$1300) | `CALL $1000` | no (homebrew utility) | no repo licence stated |
| `zu2_BASICMASTER_additions/KUMAJIRI/` | KUMAJIRI-language sample programs: STAR FIRE 2 (TEW SOFT game), 8 QUEEN, LIFE, DIS-AS | KUMAJIRI source text (`.txt`) | needs the KUMAJIRI compiler + K-CPU already in `found/zu2_BASICMASTER/KUMAJIRI` (8 QUEEN: object $5000, source $3400) | partly (KUMAJIRI was published in I/O別冊 システムプログラムライブラリ2, p. 224) | **MIT** (KUMAJIRI/LICENSE.txt, with permission from the original author 津田伸秀 and the I/O editors) |

On zu2/BASICMASTER: everything else in the repo (as of commit 5f0c40e, 2026-10-18)
already sits in `software/found/zu2_BASICMASTER`. Besides life2, that copy already holds
**TinyGalaxy** (BMUG newsletter May 1982 type-in, `.cmt` + scan), **L2BASCOM** (BMUG
1982-04, `.cmt` + scan PDF), **DendaiTinyBasic** (`DENDAI.cmt`), GAME68, NTB,
microPASCAL68 (ASCII 1980-12 dump-list patch) and TL/1 notes. Only `hires/` and the
four KUMAJIRI `.txt` programs were new.

None of the downloads is a bm2 byte stream (leader `$FF`, then `$01` blocks). The
byte-stream `.bin` versions of OPA and WILWESP are already in `software/typein/`.

## Checked and nothing new

* **inufuto** (<http://inufuto.web.fc2.com/8bit/>): 23 titles, the 22 we have plus
  UCHOTOS. Nothing else.
* **ver0.sakura.ne.jp/pc/** (bm2 author): emulator and bmtape2 only. No tape images.
  `jr_games.html` and `hitachi.html` are screenshot pages that list titles (below),
  with no downloads.
* **takeda-toshiya bmjr** (eBASICMasterJr / CSP): emulator only.
* **kwhr0.g2.xrea.com/bm2.html**: kwhr0's bm2 fork plus MIT C demos on GitHub
  (`kwhr0/bm2-mml-demo`, `bm2-baremetal-demo`, `bm2-chibicc-demo`, `bm2-xevious`).
  They are source only and need CC6303/chibicc/fcc. The xevious port needs 64 KB RAM
  and a clock of 10 MHz or more, so it will not run on a stock machine. Not downloaded.
* **fuckin.rash.jp HeAVeN@Wiki**: hardware notes and links only.
* **zukeran.org blog**: the BM category is hardware articles. The site was refusing
  connections; I read it through the Wayback Machine.
* **MAME `bmjr_cass`**: one entry, WILWESP (WAV), which is already above.
* **j68** (upd780c1.g1.xrea.com): screenshots of CARRYING FRUITS Jr. (ベーマガ 1988-09)
  and 4D-アステロイド (ベーマガ 1984-09) running. No downloads. Recent j68 builds have a
  "no redistribution" clause.
* `romjacket/bmjr` on GitHub: one 105 KB `bmjr.7z`, almost certainly the system ROMs.
  Not downloaded.
* basicmaster.fan-site.net: dead. The Wayback copy has no files.
* No "Oh!HITACHI" magazine scans turned up on archive.org.

## Type-in sources

### 1. Micom BASIC Magazine (マイコンBASICマガジン): the main source

* **Index of BM programs per issue:** <http://s-sasaji.ddo.jp/bm/pr_bmjr.htm>
  ("ベーマガ特選プログラム名一覧 日立ベーシックマスターレベルII / Jr."). It covers
  1982-07 to 1990-12, about 115 titles. Ones marked "Jr" are Jr-only (graphics page or
  colour adapter). The rest are L2/L2II programs, which the Jr runs; their listing
  headers usually read "BMJr/L2".
* **Scans on archive.org:**
  * 1982-07 to 1984-12: `micom-basic-magazine-issue-N-<month>-<year>` (600 dpi).
    Watch the ids: Oct 1984 is `issue-29-october-1984` and Jul 1983 is `issue-12-july-1983`.
  * 1984-12 to 1994: `micomBASIC_YYYY-MM`, `micomBASIC-YYYY-MM` and `basic-YYYY-MM`.
    These are three scan sets of the same issues.
  * Specials: `micom-basic-magazine-computer-game-collection-march-1982` and
    `micom-basic-computer-game-collection-ii-may-1982`. Neither had BM listings in the
    OCR; the hits were Hitachi ads.
* **Finding the page:** `bemaga_bm_index.tsv` lists the leaves where a word from the
  title occurs in the OCR. Open them as `https://archive.org/details/<item>/page/n<leaf>`,
  or get an image with `https://archive.org/download/<item>/page/n<leaf>.jpg`. Low leaf
  numbers that repeat across one issue (25–45) are the table of contents; the larger
  number is usually the article. These leaves are confirmed:
  * グラフィック・ヘルメット: n51–54
  * ゴリラと探検隊: n75 (BMJr/L2II, colour adapter)
  * DRINK CAN: n70 (full BASIC listing)
  * T.KUROKAWA: n166 (one-page BASIC listing)

  For OPA and WILWESP, Typing Task Force cites `micomBASIC-1984-12` n85–88.

Jr-only titles (Jr = Jr. only):

| issue | title | author | archive.org item | page leaf hint |
|---|---|---|---|---|
| 1982-12 | グラフィックヘルメット | 中村賢之 | micom-basic-magazine-issue-6-december-1982 | **n51–54 (downloaded)** |
| 1983-11 | 楊夫人(マダム・ヤン) | | micom-basic-magazine-issue-17-november-1983 | ? |
| 1984-01 | ゴリラと探検隊 | BMファミリー・ミエ子 | micom-basic-magazine-issue-19-january-1984 | **n75** |
| 1984-02 | 将棋パズル | | micom-basic-magazine-issue-20-february-1984 | n76–77 |
| 1984-04 | イン石爆破ゲーム | | micom-basic-magazine-issue-22-april-1984 | n85–86 |
| 1984-05 | HONEY FLYER | 一夜 | micom-basic-magazine-issue-23-may-1984 | ? |
| 1984-06 | MISSILE FIGHT (machine code) | | micom-basic-magazine-issue-24-june-1984 | n92 |
| 1984-07 | エイリアン・ハウス / CHOP RESCUE | | micom-basic-magazine-issue-25-july-1984 | ? |
| 1984-09 | 4D-アステロイド | 市村哲 | micom-basic-magazine-issue-27-september-1984 | ? |
| 1984-10 | WINE CELLAR | 一夜 | micom-basic-magazine-issue-29-october-1984 | ? |
| 1984-12 | グラフィックOPA | 一夜 | (already in typein/) | |
| 1985-03 | DRINK CAN | 一夜 | micomBASIC-1985-03 | **n70** |
| 1985-05 | FIGHT!! POKURIN | | micomBASIC_1985-05 | n69 |
| 1985-06 | STRAWBERRY FIELD | 一夜 | micomBASIC-1985-06 | n69 |
| 1985-08 | CRISIS EARTH | | micomBASIC-1985-08 | ? |
| 1985-09 | タムヤン・スゴイヤー | | micomBASIC_1985-09 | ? |
| 1986-11 | TOSHI'S MORM (colour adapter) | | micomBASIC-1986-11 | n59 |
| 1987-08 | 移植版 Snow Cape | | micomBASIC-1987-08 | n83 |
| 1988-05 | DRAGON CRYSTAL | | micomBASIC_1988-05 | ? |
| 1988-06 | OTOS | | micomBASIC-1988-06 | n95–96 |
| 1988-07 | RUN! RUN! | | micomBASIC-1988-07 | ? |
| 1988-08 | FINAL STAGE | | micomBASIC-1988-08 | ? |
| 1988-10 | KONTIHA VEGETABLES | | micomBASIC-1988-10 | n103 |
| 1988-12 | GAZATY | | micomBASIC-1988-12 | n101 |
| 1989-01 | THE MAGICAL WIND | | micomBASIC-1989-01 | n55 |
| 1990-12 | T.KUROKAWA | | basic-1990-12 | **n166** |

About 85 more L2/L2II titles, which also run on the Jr, are in the TSV. Examples:
UFOアタック (1982-07), 森巧尚's run of games (モスキート, HONEY BEE, フルコース, E.T.1/2,
MUSIC PLANS, 君に胸キュン!, SWEEPER, ILLUMINATI☆N, PIDER, DRACULA), 核分裂, ICE ALIEN,
TIME FIGHTER, POKURIN 2/4, CARRYING FRUITS Jr. (1988-09).

The ver0 `jr_games.html` screenshot page agrees with that index and adds titles from
other sources:

* RAM (富士音響マイコンセンター): バグバグモンスター, ギャラクシーウォー
* 日立 Program Pack No.2 (commercial): ガムダム, 宇宙戦争, もぐらたたき, 栄養診断, Jr.オセロ
* マイコンゲームの本2 (工学社 1982): ラリー7, プロ・ゴルフ, クレイジーフライ
* I/O 1981-01: パックマン
* ベーマガ: カバトリゲーム 1982-08, 雀牌デモ 1983-04, E.T.1/2, 核分裂, MUSIC PLANS,
  ICE ALIEN, TIME FIGHTER, スペース・ディフェンダー, ちょうちょう, and
  キンギョノウンコゲーム (issue unknown)

### 2. I/O (工学社)

archive.org has about 115 issues, uploaded under creator `工学社`. Ids look like
`io-198101`, `IO198109` and `Io19826`; 1978–1989 is mostly complete. I/O ran mostly
machine-language dump listings for several machines at once. For example, 1981-04
n166–169 is a general guide to saving, loading and running ML listings that covers the
MB-6880(L2)/6881. jr_games names **I/O 1981-01 パックマン** as a BM title, but its exact
page in `io-198101` was not pinned down. The OCR hits for 6880/6881/6885 that I checked
were mostly shop ads, so I/O still needs a manual look at each issue's contents page.
I/O別冊 システムプログラムライブラリ2 (KUMAJIRI) and PiO vol.1 (`PioVol.1`) are also
there.

### 3. Books and newsletters (not found online)

* マイコンゲームの本2 (工学社 1982): ラリー7, プロ・ゴルフ, クレイジーフライ. Not on archive.org.
* BMUG 会報 (BASIC MASTER USER'S GROUP newsletter, 1982): zu2 has scans of the TinyGalaxy
  and L2BASCOM listings and notes TL/1 in No.2–3. There is no archive of whole issues.
* RAM magazine (広済堂): only 1980-09 is on archive.org
  (`ram-september-1980-09-600dpi-ozidual`). It has OCR hits for ベーシックマスター at
  n35/40 and 6881 at n85/148/152, which may be L1/L2 listings. Unverified.
* ASCII: micro PASCAL 68 is in 1980-12, which is not on archive.org; zu2's patch
  covers it. ASCII 1980-10 (`ascii-1980-10-1-game`, the GAME compiler) is there and
  relevant to GAME68.
* Super Soft Magazine (Micom BASIC supplement, 1983-12 to 1984-11, `super-soft-magazine-*`):
  no BM hits.

### 4. Typing Task Force (Gaming Alexandria)

Category page:
<https://www.gamingalexandria.com/wp/category/type-in-programs/type-in-programs-hitachi-basic-master-jr/>.
It has only OPA and WILWESP, which are mirrored above; the directory also has a
`bmjr.zip`, presumably the emulator and BIOS, which was not downloaded. It is still the
place most likely to publish new BMJr type-in conversions.
