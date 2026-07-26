ベーシックマスターJr.のテープ音声を処理するツール


* 概要 *
 このプログラムは, 日立ベーシックマスターJr.のテープ音声と, エミュレータで使用
 するテープイメージの相互変換を行うツールである.
 テープ音声からのファイルの取り出し, ファイルのテープ音声の変換も可能である.


* 使用法 1 *
 bmtape [-300|-1200] [-s <smoothing>] [-m <mid>] [-t <threshold>] [-display] <ファイル名> ...

 <ファイル名>
 テープを録音したWAVEファイル, テープイメージファイル, テキストファイル, また
 はモトローラSレコードファイル(バイナリファイル).
 WAVEファイルは無圧縮のみ可能. (44.1kHz, モノラル推奨)
 ファイルは複数指定できる.

 WAVEファイルを指定した場合, テープイメージとテキスト・バイナリファイルを出力
 する.
 テープイメージを指定した場合, WAVEファイルを出力する.
 テキストまたはバイナリファイルを指定した場合, テープイメージとWAVEファイルを
 出力する.

 -300
 テープ音声が300baudで記録されていること想定する.
 または300baudのテープ音声を生成する.
 デフォルト

 -1200
 テープ音声が1200baudで記録されていること想定する.
 または1200baudのテープ音声を生成する.

 -s
 平均値フィルタを適用する長さ. (単位は 1/44100秒, デフォルトは3)
 WAVEファイルを指定した場合のみ有効.

 -m
 指定した長さの平均を波形の中央とする. (単位はHz, デフォルトは1400)
 デフォルトでうまくいかない場合は2400とするとよい場合がある.
 WAVEファイルを指定した場合のみ有効.

 -t
 低音部と高音部の閾値. (単位はHz)
 WAVEファイルを指定した場合のみ有効.

 -display
 波形を表示する.
 WAVEファイルを指定した場合のみ有効.


* 使用法 2 *
 (1)ベーシックマスターJr.の実機のテープ出力(赤色)とPCのライン入力またはマイク
    をつなげる.
 (2)B000-E7FF, F000-FFFFを出力し録音する.
    o Window標準のサウンドレコーダは短い時間しか記録できないため別のソフトを
      用意する必要がある.
      作者は「ぽけっとれこーだー」(http://radical.xrea.jp/)を使用した.
    o フォーマットは44.1kHz, 8bit, モノラルを推奨する.
    o 録音時間はB000-E7FFは約13:20, F000-FFFFは約4:00. (300baudの場合)
    o コマンドは次のとおりである.
      MON <RETURN>  (モニタに移行する)
      P             (テープに出力するコマンド)
      ROM1 <RETURN> (ファイル名. 他の名前でもよい)
      B000 <SPACE>  (開始アドレス)
      E800 <SPACE>  (終了アドレス+1. SPACEを押すと出力を開始する.)
      P
      ROM2 <RETURN>
      F000 <SPACE>
      FFFF <SPACE>
 (3)bmtapeでMotorola Sレコード形式にする.
 (4)ファイル名をB000-E7FFはb000-e7ff.s, F000-FFFFはf000-ffff.sに変更する.
 (5)f000-ffff.sをテキストエディタで開く.
    最後から2番目の行が
    S112FFF0??????????????????????????????42
    の様になっているので, 最後の1byteをモニタのDコマンドで表示して挿入する.
    o ベーシックマスターJr.のモニタは入力したアドレス-1までを出力するため,
      FFFFの内容を出力できない. そのためこの作業が必要である.
    o 挿入する場所は後ろから2文字目と3文字目の間(上の例ならば??と42の間).
      またデータ長(前から3文字目と4文字目, 書き換え前は12)を13に書き換える.
    o 最後の2文字(上の例では42)はチェックサムだが, bm2はチェックしないため書
      き換えなくてもよい.
      (気になる人はチェックサムから書き加えた値を引いたものに置き換えること.)
    o 最後の1byteを表示するコマンドは次のとおりである.
      MON <RETURN>
      D
      FF80 <SPACE>


* その他 *
 ・チェックサムが一致しないときは警告を出力するがそのままファイルに書き込む.
 ・1200baudのテープ音声とテキストファイルを相互変換する機能はない(未実装).


* 参考Webページ *
 Kansas City standard - Wikipedia, http://en.wikipedia.org/wiki/Kansas_City_standard


* スペシャルサンクス *
 ヒマジンガーＺさん 情報提供ありがとうございました.


* ライセンス *
Copyright (c) 2010 maruhiro
All rights reserved. 

Redistribution and use in source and binary forms, 
with or without modification, are permitted provided that 
the following conditions are met: 

 1. Redistributions of source code must retain the above copyright notice, 
    this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright notice, 
    this list of conditions and the following disclaimer in the documentation 
    and/or other materials provided with the distribution. 

THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, 
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND 
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL 
THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

(日本語訳・参考)
Copyright (c) 2010 maruhiro
All rights reserved.

ソースコード形式かバイナリ形式か、変更するかしないかを問わず、以下の条件を満たす
場合に限り、再頒布および使用が許可されます。

 1. ソースコードを再頒布する場合、上記の著作権表示、本条件一覧、および下記免責条
    項を含めること。
 2. バイナリ形式で再頒布する場合、頒布物に付属のドキュメント等の資料に、上記の著
    作権表示、本条件一覧、および下記免責条項を含めること。

本ソフトウェアは、著作権者およびコントリビューターによって「現状のまま」提供され
ており、明示黙示を問わず、商業的な使用可能性、および特定の目的に対する適合性に関
する暗黙の保証も含め、またそれに限定されない、いかなる保証もありません。著作権者
もコントリビューターも、事由のいかんを問わず、損害発生の原因いかんを問わず、かつ
責任の根拠が契約であるか厳格責任であるか（過失その他の）不法行為であるかを問わず
、仮にそのような損害が発生する可能性を知らされていたとしても、本ソフトウェアの使
用によって発生した（代替品または代用サービスの調達、使用の喪失、データの喪失、利
益の喪失、業務の中断も含め、またそれに限定されない）直接損害、間接損害、偶発的な
損害、特別損害、懲罰的損害、または結果損害について、一切責任を負わないものとしま
す。
