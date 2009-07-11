

			窓使いの憂鬱ドライバ


1. コンパイル方法

	Windows 200 DDK と Visual C++ 6.0 で build ユーティリティを利
	用してコンパイルします。

	> cd mayu\d
	> build
	> cd nt4
	> build

	mayud.sys を %windir%\system32\drivers\ へコピーし test.reg を
	入力すれば、手動でデバイスを on/off できます。(又は Windows
	NT4.0 の場合は mayudnt4.sys を mayud.sys という名前でコピー)


2. 使い方

	mayud を動作させると

	      \\.\MayuDetour1

	というデバイスができます。このデバイスを GENERIC_READ |
	GENERIC_WRITE で開きます。

	ReadFile / WriteFile では、以下の構造体を使います。デバイスを
	開いたあとに、ReadFile するとユーザーが入力したキーを取得でき
	ます。WriteFile するとユーザがあたかもキーを入力したかのように 
	Windows を操作することができます。

	struct KEYBOARD_INPUT_DATA
	{
	  enum { BREAK = 1,
		 E0 = 2, E1 = 4,
		 TERMSRV_SET_LED = 8 };
	  enum { KEYBOARD_OVERRUN_MAKE_CODE = 0xFF };
	  
	  USHORT UnitId;
	  USHORT MakeCode;
	  USHORT Flags;	
	  USHORT Reserved;
	  ULONG ExtraInformation;
	};

	UnitId と Reserved は常に 0 です。ExtraInformation に値を設定
	すると、WM_KEYDOWN などのメッセージが来た時に 
	GetMessageExtraInfo() API でその値を取得することができます。
	MakeCode はキーボードのスキャンコードです。Flags は BREAK, E0,
	E1, TERMSRV_SET_LED が組み合わさっています。BREAK はキーを離し
	たとき、E0 と E1 は拡張キーを押したときに設定されます。


3. バグ

	* ReadFile が ERROR_OPERATION_ABORTED で失敗した場合もう一度 
	  ReadFile する必要があります。

	* 複数のスレッドから mayud デバイスを read すると
	  MULTIPLE_IRP_COMPLETE_REQUESTS (0x44) で落ちることがあるよう
	  です。再現性は不明。

	* ReadFile するとユーザーが入力するまで永遠に帰ってきません。
	  NT4.0 ならば別スレッドで CancelIo することで ReadFile をキャ
	  ンセルすることができますが、Windows 2000 では方法がありませ
	  ん。

	* PnP は考慮していません。つまり、キーボードをつけたり離したり
	  するとどうなるかわかりません。

	* キーボードが二つ以上あるときでも、デバイスは一つしかできませ
	  ん。
