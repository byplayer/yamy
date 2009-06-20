◎Yet Another Mado tsukai no Yuutsu(YAMY) ver.0.01

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
yamyd32.dll	... 64bit環境で32bitプロセスをフックするための補助プログラム
yamy.ini	... 設定ファイル(mayu でのレジストリ設定に相当)
workaround.reg	... 特定キー問題対策用Scancode Mapレジストリサンプル
*.mayu		... キーバインド設定ファイル


3. 使用方法

基本的な使用方法は「窓使いの憂鬱」と同じです。
http://mayu.sourceforge.net/mayu/doc/README-ja.html
を参照して下さい。

以下、「窓使いの憂鬱」と異なる部分について記載します。

* インストーラはありません。yamy-0.01.zip を任意のフォルダに展開し、
  yamy.exe を実行して下さい。

* レジストリではなく、yamy.exe と同じフォルダにある yamy.ini に
  設定情報の保存します。

* 設定ファイルはホームディレクトリではなく、yamy.exe のあるフォルダに
  .mayu というファイル名で置いて下さい。

* キーボードの種別の判定は行いませんので、初回起動時にメニューの
  「選択」で適切な設定を選択して下さい。

* 4.にもが記載あるように、日本語キーボードの場合「英数(CapsLock)」
  「半角・全角」「ひらがな」の3キーを正しくフックできないため
  レジストリの Scancode Map による置き換えを利用する必要があります。
  同梱している workaround.reg はそのサンプルです。このサンプルでは
  これら3つのキーに E0 プレフィックスを付加することにより、別キー
  に変換しています。同梱の *.mayu はこの Scancode Map の下で
  これら3つのキーがあたかも本来のキーのように動作するように
  設定が追加されています。
  workaround.mayu にはこの対策に対応した追加部分を抽出していますので、
  独自の .mayu を使っている場合はこれを参考にして下さい。


4. 制限事項・不具合

* 日本語キーボードの場合「英数(CapsLock)」「半角・全角」「ひらがな」
  の3キーについてはレジストリの Scancode Map による置き換えを併用
  する必要があります。

* 数秒間キー入力が滞る現象がたまに発生します。

* 画面ロック時はキー置換が働きません。また、この制限により画面ロック
  への遷移時に押し下げられているキーがあった場合、そのキーが押しっぱなし
  になることがあります。この場合、そのキーを空押しすることによって
  押しっぱなしが解消します。特に Alt キーが押しっぱなしだと、パスワード
  が入力できなくなるので注意して下さい。

* Vistaでの保護モード有効なIE7ではキーマップがグローバルになります。
  IE8では問題ありません。

* 管理者権限で実行した場合、一般権限で動作しているプロセスのキーマップ
  はグローバルになります。

* ユーザモードでのフックのため、以下の場合は機能しないと思われます。
  - WH_KEYBOARD_LL をフックする他アプリとの共存
  - DirectInput を使ったプログラム

* Pauseキーのようにスキャンコードに E1 プレフィックスが付いたキー
  は置き換えられません。そのようなキーを使用したい場合は Scancode Map
  レジストリを併用して下さい。

* セキュリティソフトによってはフックDLLのインストールをブロックされる
  場合がありますので、その場合は yamy32/yamy64 を例外として登録して下さい。


5. 著作権・ライセンス

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


6. 謝辞

言うまでもなく「窓使いの憂鬱」がなければYAMYは存在し得ませんでした。
「窓使いの憂鬱」の作者である多賀奈由太さんと開発に貢献した方々にこの
場を借りて深くお礼申し上げます。


7. 履歴

2009/06/?? ver.0.01

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
  - デバッガ等の特定プロセスではフックDDLLのデバッグ出力を抑止
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
