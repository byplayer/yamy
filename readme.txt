Yet Another Mado tsukai no Yuutsu(YAMY) ver.0.03

1. 概要

Windows用汎用キーバインディング変更ソフト「窓使いの憂鬱(以後mayuと表記)」
(http://mayu.sourceforge.net/)のキー入力置換をドライバベースからユーザ
モードフックベースに変更した派生ソフトウェアです。
既に開発を終了したmayuをforkすることにより、Windows Vista以降のOSのサポート
を目指しています。

オリジナルのmayuではフィルタドライバによりキーの置き換えを実現していましたが、
本プロジェクトではそれをWH_KEYBOARD_LLのフックとSendInput() APIに変更します。
これにより、mayuほどの低層での強力な
置換は期待できなくなるものの、ドライバへの署名を要することなく、
Vista以降のWindows(特に64bit版)への対応を目指します。


2. ファイル構成

yamy.exe	... yamy32/yamy64のどちらかを起動するランチャ
yamy32		... 32bit版yamy本体
yamy64		... 64bit版yamy本体
yamy32.dll	... 32bit版フックDLL
yamy64.dll	... 64bit版フックDLL
yamyd32		... 64bit環境で32bitプロセスをフックするための補助プログラム
yamy.ini	... 設定ファイル(mayu でのレジストリ設定に相当)
workaround.reg	... 特定キー問題対策用Scancode Mapレジストリサンプル
readme.txt      ... 本ドキュメント
*.mayu		... キーバインド設定ファイル


3. 使用方法

基本的な使用方法は「窓使いの憂鬱」と同じです。
http://mayu.sourceforge.net/mayu/doc/README-ja.html
を参照して下さい。

以下、「窓使いの憂鬱」と異なる部分について記載します。

3.1. マウスイベントの置換

いくつかのマウスイベントをキーイベントと同様に置換可能です。
各マウスイベントはE1-プレフィックスを持つ擬似的なスキャンコード
として扱われます。
# WH_KEYBOARD_LLを使うyamyではE1-プレフィックスのキーコードを
# を拾うことができないため、実際のキーコードと衝突する可能性はない。

マウスイベントの置換はデフォルトではオフです。
有効化するためには.mayu ファイルに

def option mouse-event = true

と記述します。

置換可能なマウスイベントは以下の通りです。
# ()内は使われる疑似スキャンコード
* マウスドラッグ Drag(E1-0x00)
* 左ボタン LButton(E1-0x01)
* 右ボタン RButton(E1-0x02)
* 中ボタン MButton(E1-0x03)
* ホイール前進 WheelForward(E1-0x04)
* ホイール後退 WheelBackward(E1-0x05)
* Xボタン1 XButton1(E1-0x06)
* Xボタン2 XButton1(E1-0x07)
* 横スクロール(チルト)右 TiltRight(E1-0x08) ※Vista以降
* 横スクロール(チルト)左 TiltLeft(E1-0x09) ※Vista以降

このうちDragイベントは、いずれからマウスボタンを押したままボタンを
押した場所から一定以上マウスを移動させた際にDownが発生し、Down発生後
にボタンを離すとUpが発生する疑似イベントです。
Dragイベント発生までの移動距離の閾値はピクセル単位で

def option drag-threshold = 30

のように指定します。閾値として0を指定するもしくは閾値を指定しない
場合、Dragイベントは発生しません。

※注意1※
WheelForward/WheelBackward/TiltRight/TiltLeftには物理的に"Up"
イベントがありませんので、yamy内部では押し下げ時にDown/Upの
両イベントが発生します。このためこれらのイベントをモディファイア
にすることはできません。

※注意2※
キーイベントと同様にマウスイベントも「調査」ウィンドウを使って
コードを調査することができますが、キーイベントと異なり調査時も
イベントは捨てません。これは「調査」モードから抜けられなくなら
ないための措置です。

※注意3※
Vista以降ではyamyを標準権限で起動し、option mouse-event を有効に
した場合、管理者権限のアプリに(置換の有無にかかわらず)マウス
イベントが届かなくなります。yamyを管理者権限で起動すれば標準権限
・管理者権限どちらにもマウスイベントが届きます。


3.2. NLSキーのエスケープ

日本語環境の場合、日本語処理に使われるいくつかのキーに対しては
WH_KEYBOARD_LLフック前に特殊処理が行われるため、yamyによって
正常にフックできません。
以下、便宜上これらのキーをNLSキー(National Language Support Key)
と呼びます。

キーボードレイアウトドライバとしてkbd106.dllを用いている場合は
NLSキーは以下の4つです。
# []内はスキャンコード
* 半角・全角[0x29]
* 英数(CapsLock)[0x3a]
* ひらがな[0x70]
* 無変換[0x7b]

キーボードレイアウトドライバとしてkbd101.dllを用いている場合は
NLSキーは以下の2つです。
# []内はスキャンコード
* `(~)[0x29]
* CapsLock[0x3a]

これらのNLSキーが正しくフックできないことへの対策としてはレジストリ
の Scancode Map を使ってこれらのキーを特殊扱いされない別のキーに
置き換える方法があります。Scancode Map の仕様については、

http://www.microsoft.com/whdc/archive/w2kscan-map.mspx

に情報があります。また以下のサイトの記述も参考になります。

http://www.jaist.ac.jp/~fujieda/scancode.html
http://sgry.jp/articles/scancodemap.html

尚、RC版で確認した限りでは Windows7 の場合、HKEY_LOCAL_MACHINE
の Scancode Map が有効のようです。RTM版でどうかは未確認です。

同梱している workaround.reg は具体的な置き換えのサンプルです。
このサンプルではこれらNLSキーに E0 プレフィックスを付加することにより、
別キーに変換しています。同梱の *.mayu はこの Scancode Map の下でこれら
E0を付加されたキーがあたかも本来のキーのように動作するように
設定が追加されています。

workaround.mayu にはこの対策に対応した追加部分を抽出していますので、
独自の .mayu を使っている場合はこれを参考にして下さい。

また「英数キーとCtrlキーの入れ替え」等の単純な置き換えで十分な
場合はこれらに絞った Scancode Map を作成しても良いでしょう。

workaround.reg のような「存在しないキーへの置き換え」による対策は
yamyが動作していない場合これらのキーが機能しなくなるという副作用
があります。

そこでworkaround.reg相当の置き換え(以下、これを「NLSキーのエスケープ」
と呼ぶ)をyamyの動作中のみ行う機能を実験的に実装しました。

yamy起動時にレジストリをworkaround.reg相当に書き換えてから
(ログアウトすることなく)OSにScancode Map読み込ませた後、すぐに
レジストリを元に戻します。yamy終了時には(レジストリは既に元に
戻っているので)単にOSに再読み込みのみを指示します。
これにより、yamyの動作中のみNLSのエスケープを実現します。

尚、スクリーンロック(別ユーザへの簡易ユーザ切り替えを含む)した場合
及び、yamy を「一時停止」した場合はエスケープが解除され、元に戻ったら
再度エスケープを行います。

使用するレジストリはWindows7以外の場合は、
HKEY_CURRENT_USER\KeyBoard Layout\Scancode Map
Windows7の場合は、
HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\KeyBoard Layout\Scancode Map
です。

この機能はデフォルトでは無効であり、有効にするためには yamy.ini において、

escapeNLSKeys=0

を 

escapeNLSKeys=1

に変更します。ただしこの機能の利用に際しては以下の点に留意して下さい。

* 実験的な機能であり十分な動作実績がなく危険が伴います。

* yamyを実行するユーザにSeDebugPrivilege特権が必要です。Administrators
  グループに属するユーザは既定でこの特権を持っています。
  ただしUACが有効な場合は、加えて管理者として実行する必要があります。

* 対象となるNLSキーが既にScancode Mapで置き換えられている場合は
  エスケープは行われません。

* エスケープのためレジストリを書き換えている一瞬の間にyamyが
  異常終了した場合、エスケープ用のScancode Mapがレジストリに
  残ります。この場合、regeditを使って元に戻して下さい。

* 上記の瞬間以外にyamyが異常終了した場合、レジストリは元に戻って
  いますが、OS内部のScancode Mapは残っているので、一旦ログオフ・
  ログオンして元に戻すか、yamyを再起動して下さい。

* VMware に対し、Scancode Map は有効ですが、yamy によるキー置換は
  は働きません。このためエスケープされた(E0-が付加された)ままで
  ゲストOSに届きます。ゲストも Windows の場合はゲスト内でも yamy
  を動作させれば元のキーに置換できます。また、Linux の場合は
  setkeycodes コマンドを使ってエスケープされたキーを元のスキャン
  コードに置換できます。例えば101キーボードを使用している場合は
  以下のコマンドによってエスケープされた `(~)[0x29] と CapsLock[0x3a]
  を元のコードに戻せます。
  > setkeycodes e029 41 e03a 58
  その他のスキャンコードの置換ができないOSがゲスト場合は yamy を
  一時停止してエスケープを解除して下さい。


3.3. &CancelPrefix関数

Prefix状態を強制的に解除するための関数&CancelPrefixが追加されました。
One Shotモディファイアに指定しているキーを離した際にPrefixを解除する
ために導入しました。

3.4. その他

* インストーラはありません。yamy-0.03.zip を任意のフォルダに展開し、
  yamy.exe を実行して下さい。

* レジストリではなく、yamy.exe と同じフォルダにある yamy.ini に
  設定情報の保存します。

* 設定ファイルはホームディレクトリではなく、yamy.exe のあるフォルダに
  .mayu というファイル名で置いて下さい。

* キーボードの種別の判定は行いませんので、初回起動時にメニューの
  「選択」で適切な設定を選択して下さい。

* リモートデスクトップでのログオン時でも起動を抑制しません。


4. 制限事項・不具合

* 画面ロック時はキー置換が働きません。また、この制限により画面ロック
  への遷移時に押し下げられているキーがあった場合、そのキーが押しっぱなし
  になることがあります。この場合、そのキーを空押しすることによって
  押しっぱなしが解消します。特に Alt キーが押しっぱなしだと、パスワード
  が入力できなくなるので注意して下さい。

* ユーザモードでのフックのため、以下の場合は機能しないと思われます。
  - WH_KEYBOARD_LL をフックする他アプリとの共存
  - DirectInput を使ったプログラム

* Pauseキーのようにスキャンコードに E1 プレフィックスが付いたキー
  は置き換えられません。そのようなキーを使用したい場合は Scancode Map
  レジストリを併用して下さい。

* セキュリティソフトによってはフックDLLのインストールをブロックされる
  場合がありますので、その場合は yamy32/yamy64 を例外として登録して下さい。


5. ビルド方法

Visual Studio 2008 Professional + Windows SDK v6.1で確認しています。
yamyのビルドにはx64用コンパイラが必要になりますが、Visual Studio 2008
の既定のインストールではインストールされませんので追加でインストール
する必要があります。

5.1.
yamy と boost_1_38_0 のソースを入手し、以下の配置にて展開します。

./
   |
   +---boost_1_38_0/ ... http://www.boost.org/ から入手したアーカイブを展開
   |
   +---yamy/ ... "git clone git://git.sourceforge.jp/gitroot/yamy/yamy.git"等により展開
       |
       +---proj/ ...
       +---tools/ ...

5.2.
yamy/proj/yamy.sln を Visual Studio で開き、ソリューションをビルドします。

5.3.
yamy/{Debug,Release}/ 以下にバイナリと zip パッケージが生成されます。


6. 著作権・ライセンス

YAMYの著作権・ライセンスは以下の通りです:

  Yet Another Mado tsukai no Yuutsu(YAMY)

    Copyright (C) 2009, KOBAYASHI Yoshiaki <gimy@users.sourceforge.jp>
      All rights reserved.

    Redistribution and use in source and binary forms,
    with or without modification, are permitted provided
    that the following conditions are met:

      1. Redistributions of source code must retain the above copyright
         notice, this list of conditions and the following disclaimer.
      2. Redistributions in binary form must reproduce the above
         copyright notice, this list of conditions and the following
         disclaimer in the documentation and/or other materials provided
         with the distribution.
      3. The name of the author may not be used to endorse or promote
         products derived from this software without specific prior
         written permission. 

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
    OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
    INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
    HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
    STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
    IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
    THE POSSIBILITY OF SUCH DAMAGE.


YAMYの派生元である「窓使いの憂鬱」の著作権・ライセンスは以下の通りです:

  窓使いの憂鬱

    Copyright (C) 1999-2005, TAGA Nayuta <nayuta@users.sourceforge.net>
      All rights reserved.

    Redistribution and use in source and binary forms,
    with or without modification, are permitted provided
    that the following conditions are met:

      1. Redistributions of source code must retain the above copyright
         notice, this list of conditions and the following disclaimer.
      2. Redistributions in binary form must reproduce the above
         copyright notice, this list of conditions and the following
         disclaimer in the documentation and/or other materials provided
         with the distribution.
      3. The name of the author may not be used to endorse or promote
         products derived from this software without specific prior
         written permission. 

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
    OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
    INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
    HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
    STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
    IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
    THE POSSIBILITY OF SUCH DAMAGE.


YAMYが利用しているBoostライブラリのライセンスは以下の通りです:

  Boost Software License - Version 1.0 - August 17th, 2003

  Permission is hereby granted, free of charge, to any person or organization
  obtaining a copy of the software and accompanying documentation covered by
  this license (the "Software") to use, reproduce, display, distribute,
  execute, and transmit the Software, and to prepare derivative works of the
  Software, and to permit third-parties to whom the Software is furnished to
  do so, all subject to the following:

  The copyright notices in the Software and this entire statement, including
  the above license grant, this restriction and the following disclaimer,
  must be included in all copies of the Software, in whole or in part, and
  all derivative works of the Software, unless such copies or derivative
  works are solely in the form of machine-executable object code generated by
  a source language processor.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
  SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
  FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
  DEALINGS IN THE SOFTWARE.


7. 謝辞

言うまでもなく「窓使いの憂鬱」がなければYAMYは存在し得ませんでした。
「窓使いの憂鬱」の作者である多賀奈由太さんと開発に貢献した方々にこの
場を借りて深くお礼申し上げます。


8. 履歴

2009/09/19 ver.0.03

* マウスイベント置換有効時に固まる場合がある問題を修正

* 入力処理スレッドにおいてキューの開放と待ちを不可分に行うよう変更

* 特定操作でIEをアクティブにするとフォーム内でEmacsEditにならないことがある問題(チケット#18663)を修正

* メールスロットが使えない場合にはWM_COPYDATAを使って通知する(チケット#17769,#18662を修正)

* 一時停止中はフックしたスキャンコードをそのままスルーするように変更(チケット#18691参照)

* yamy64 で &InvestigateCommand が機能しない問題を修正

* 終了後に特定のプロセスが原因で mayu{32,64}.dll が削除できなくなる問題を修正

2009/08/30 ver.0.02

* yamy{32,64}/yamyd32 を yamy.exe と同じフォルダから探すように変更

* Vistaでの権限昇格実行時に標準権限アプリのキーマップがグローバルになる問題を修正

* NLSキーのエスケープ機能を実験的に実装

* &CancelPrefix関数を追加

* マウスイベントの置換機能を追加

* リモートデスクトップ時の起動抑制を廃止

* ビルドシステムを変更
  - makefileからVC++2008のプロジェクトに移行
  - makefuncとzipでのパッケージ作成をJScriptで再実装

* 不具合修正
  - ハングしているプロセスがあると終了できない(チケット#17643)
  - 右シフトが押されたままになることがある(チケット#17607)
  - yamyのダイアログを消す際に5秒程度フリーズすることがある(チケット#17767)
  - 数秒間キー入力が滞ることがある(チケット#17576)

2009/06/28 ver.0.01

初リリース
以下は「窓使いの憂鬱」の最終版からの変更点

* キー入力置換をドライバからユーザモードに変更(NO_DRIVERマクロ)
  - ドライバへのアクセスを排除
  - キー入力のフックに WH_KEYBOARD_LL を使う
  - キーイベント生成にSendInput() APIを使う
  - WM_COPYDATA での通知でストールする場合があるのでメールスロットで通知(USE_MAILSLOTマクロ)
  - 多重メッセージ対策として !PM_REMOVE なメッセージをフックDLLで無視
  - RShiftにE0が付加されることに対応して{104,109}.mayuにworkaroundを追加

* 64bit対応(MAYU64マクロ)
  - GetWindowLong -> GetWindowLongPtr 等の使用API変更
  - LONG -> LONG_PTR 等の型変更
  - HWND を DWORD にキャストして 32bit<->64bit 間で共有
  - 64bit 時に 32bit プロセスへのフックをインストールする yamyd.cpp を新設
  - objの出力ディレクトリを32bitと64bitで分けた
  - WPARAM/LPARAM の実体が 64bit では異なるので、load_ARGUMENT()のオーバーロードを追加
  - INVALID_HANDLE_VALUE=0xffffffff と仮定しない
  - notifyCommand()を無効化(一時的措置)

* インストール無しでの実行
  - インストーラをビルド対象から外す
  - レジストリの替りに yamy.ini で設定する(USE_INIマクロ)

* ログ関連
  - hook.cpp にデバッグマクロ追加
  - デバッガ等の特定プロセスではフックDLLのデバッグ出力を抑止
  - ログをファイルに記録する機能を追加(LOG_TO_FILEマクロ:既定は無効)
  - OS側のキー押し下げ状態をログ出力する「チェック」機能を追加

* バグ修正
  - Engine::setFocus()でクラッシュする問題を修正
  - KeyIterator::KeyIterator()で空リスト処理時にassert failする問題を修正
  - デバッグビルドではデバッグ版ランタイムをリンクする

* その他
  - exeやdllのベースネームを mayu から yamy に変更
  - 32bit/64bit の exe を呼び分けるランチャを導入
  - フックDLLの初期化処理の大半を DllMain から外した
  - boost::regex の更新に伴い tregex::use_except の明示を削除
  - VC++9をデフォルトのコンパイラに変更
  - LOGNAME -> USERNAME
  - -GX を -EHsc に変更
  - nmake のオプションから -k を削除
  - フックを解除するため WM_NULL をブロードキャスト
