//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// vkeytable.cpp


#include "vkeytable.h"
#include <ime.h>


// Vkey table (terminated by NULL)
const VKeyTable g_vkeyTable[] =
{
#define VK(name) { VK_##name, _T(#name) }

/*
 * from WinUser.h
 */

  VK(LBUTTON),		// 0x01
  VK(RBUTTON),		// 0x02
  VK(CANCEL),		// 0x03
  VK(MBUTTON),		// 0x04    /* NOT contiguous with L & RBUTTON */

#if(_WIN32_WINNT >= 0x0500)
  VK(XBUTTON1),		// 0x05    /* NOT contiguous with L & RBUTTON */
  VK(XBUTTON2),		// 0x06    /* NOT contiguous with L & RBUTTON */
#endif /* _WIN32_WINNT >= 0x0500 */

/*
 * 0x07 : unassigned
 */

  VK(BACK),		// 0x08
  VK(TAB),		// 0x09

/*
 * 0x0A - 0x0B : reserved
 */

  VK(CLEAR),		// 0x0C
  VK(RETURN),		// 0x0D

  VK(SHIFT),		// 0x10
  VK(CONTROL),		// 0x11
  VK(MENU),		// 0x12
  VK(PAUSE),		// 0x13
  VK(CAPITAL),		// 0x14

  VK(KANA),		// 0x15
  VK(HANGEUL),		// 0x15  /* old name - should be here for compatibility */
  VK(HANGUL),		// 0x15
  VK(JUNJA),		// 0x17
  VK(FINAL),		// 0x18
  VK(HANJA),		// 0x19
  VK(KANJI),		// 0x19

  VK(ESCAPE),		// 0x1B

  VK(CONVERT),		// 0x1C
  VK(NONCONVERT),	// 0x1D
  VK(ACCEPT),		// 0x1E
  VK(MODECHANGE),	// 0x1F

  VK(SPACE),		// 0x20
  VK(PRIOR),		// 0x21
  VK(NEXT),		// 0x22
  VK(END),		// 0x23
  VK(HOME),		// 0x24
  VK(LEFT),		// 0x25
  VK(UP),		// 0x26
  VK(RIGHT),		// 0x27
  VK(DOWN),		// 0x28
  VK(SELECT),		// 0x29
  VK(PRINT),		// 0x2A
  VK(EXECUTE),		// 0x2B
  VK(SNAPSHOT),		// 0x2C
  VK(INSERT),		// 0x2D
  VK(DELETE),		// 0x2E
  VK(HELP),		// 0x2F

/*
 * VK_0 - VK_9 are the same as ASCII '0' - '9' (0x30 - 0x39)
 * 0x40 : unassigned
 * VK_A - VK_Z are the same as ASCII 'A' - 'Z' (0x41 - 0x5A)
 */

  { _T('0'), _T("_0") },		// 30 0
  { _T('1'), _T("_1") },		// 31 1
  { _T('2'), _T("_2") },		// 32 2
  { _T('3'), _T("_3") },		// 33 3
  { _T('4'), _T("_4") },		// 34 4
  { _T('5'), _T("_5") },		// 35 5
  { _T('6'), _T("_6") },		// 36 6
  { _T('7'), _T("_7") },		// 37 7
  { _T('8'), _T("_8") },		// 38 8
  { _T('9'), _T("_9") },		// 39 9

  { _T('A'), _T("A") },			// 41 A
  { _T('B'), _T("B") },			// 42 B
  { _T('C'), _T("C") },			// 43 C
  { _T('D'), _T("D") },			// 44 D
  { _T('E'), _T("E") },			// 45 E
  { _T('F'), _T("F") },			// 46 F
  { _T('G'), _T("G") },			// 47 G
  { _T('H'), _T("H") },			// 48 H
  { _T('I'), _T("I") },			// 49 I
  { _T('J'), _T("J") },			// 4A J
  { _T('K'), _T("K") },			// 4B K
  { _T('L'), _T("L") },			// 4C L
  { _T('M'), _T("M") },			// 4D M
  { _T('N'), _T("N") },			// 4E N
  { _T('O'), _T("O") },			// 4F O
  { _T('P'), _T("P") },			// 50 P
  { _T('Q'), _T("Q") },			// 51 Q
  { _T('R'), _T("R") },			// 52 R
  { _T('S'), _T("S") },			// 53 S
  { _T('T'), _T("T") },			// 54 T
  { _T('U'), _T("U") },			// 55 U
  { _T('V'), _T("V") },			// 56 V
  { _T('W'), _T("W") },			// 57 W
  { _T('X'), _T("X") },			// 58 X
  { _T('Y'), _T("Y") },			// 59 Y
  { _T('Z'), _T("Z") },			// 5A Z

  VK(LWIN),		// 0x5B
  VK(RWIN),		// 0x5C
  VK(APPS),		// 0x5D

/*
 * 0x5E : reserved
 */

  VK(SLEEP),		// 0x5F

  VK(NUMPAD0),		// 0x60
  VK(NUMPAD1),		// 0x61
  VK(NUMPAD2),		// 0x62
  VK(NUMPAD3),		// 0x63
  VK(NUMPAD4),		// 0x64
  VK(NUMPAD5),		// 0x65
  VK(NUMPAD6),		// 0x66
  VK(NUMPAD7),		// 0x67
  VK(NUMPAD8),		// 0x68
  VK(NUMPAD9),		// 0x69
  VK(MULTIPLY),		// 0x6A
  VK(ADD),		// 0x6B
  VK(SEPARATOR),	// 0x6C
  VK(SUBTRACT),		// 0x6D
  VK(DECIMAL),		// 0x6E
  VK(DIVIDE),		// 0x6F
  VK(F1),		// 0x70
  VK(F2),		// 0x71
  VK(F3),		// 0x72
  VK(F4),		// 0x73
  VK(F5),		// 0x74
  VK(F6),		// 0x75
  VK(F7),		// 0x76
  VK(F8),		// 0x77
  VK(F9),		// 0x78
  VK(F10),		// 0x79
  VK(F11),		// 0x7A
  VK(F12),		// 0x7B
  VK(F13),		// 0x7C
  VK(F14),		// 0x7D
  VK(F15),		// 0x7E
  VK(F16),		// 0x7F
  VK(F17),		// 0x80
  VK(F18),		// 0x81
  VK(F19),		// 0x82
  VK(F20),		// 0x83
  VK(F21),		// 0x84
  VK(F22),		// 0x85
  VK(F23),		// 0x86
  VK(F24),		// 0x87

/*
 * 0x88 - 0x8F : unassigned
 */

  VK(NUMLOCK),		// 0x90
  VK(SCROLL),		// 0x91

/*
 * NEC PC-9800 kbd definitions
 */
  VK(OEM_NEC_EQUAL),	// 0x92	// '=' key on numpad

/*
 * Fujitsu/OASYS kbd definitions
 */
  VK(OEM_FJ_JISHO),	// 0x92	// 'Dictionary' key
  VK(OEM_FJ_MASSHOU),	// 0x93	// 'Unregister word' key
  VK(OEM_FJ_TOUROKU),	// 0x94	// 'Register word' key
  VK(OEM_FJ_LOYA),	// 0x95	// 'Left OYAYUBI' key
  VK(OEM_FJ_ROYA),	// 0x96	// 'Right OYAYUBI' key

/*
 * 0x97 - 0x9F : unassigned
 */

/*
 * VK_L* & VK_R* - left and right Alt, Ctrl and Shift virtual keys.
 * Used only as parameters to GetAsyncKeyState() and GetKeyState().
 * No other API or message will distinguish left and right keys in this way.
 */
  VK(LSHIFT),		// 0xA0
  VK(RSHIFT),		// 0xA1
  VK(LCONTROL),		// 0xA2
  VK(RCONTROL),		// 0xA3
  VK(LMENU),		// 0xA4
  VK(RMENU),		// 0xA5

#if(_WIN32_WINNT >= 0x0500)
  VK(BROWSER_BACK),	// 0xA6
  VK(BROWSER_FORWARD),	// 0xA7
  VK(BROWSER_REFRESH),	// 0xA8
  VK(BROWSER_STOP),	// 0xA9
  VK(BROWSER_SEARCH),	// 0xAA
  VK(BROWSER_FAVORITES),// 0xAB
  VK(BROWSER_HOME),	// 0xAC

  VK(VOLUME_MUTE),	// 0xAD
  VK(VOLUME_DOWN),	// 0xAE
  VK(VOLUME_UP),	// 0xAF
  VK(MEDIA_NEXT_TRACK),	// 0xB0
  VK(MEDIA_PREV_TRACK),	// 0xB1
  VK(MEDIA_STOP),	// 0xB2
  VK(MEDIA_PLAY_PAUSE),	// 0xB3
  VK(LAUNCH_MAIL),	// 0xB4
  VK(LAUNCH_MEDIA_SELECT),	// 0xB5
  VK(LAUNCH_APP1),	 // 0xB6
  VK(LAUNCH_APP2),	 // 0xB7

#endif /* _WIN32_WINNT >= 0x0500 */

/*
 * 0xB8 - 0xB9 : reserved
 */

  VK(OEM_1),		// 0xBA	// ';:' for US
  VK(OEM_PLUS),		// 0xBB	// '+' any country
  VK(OEM_COMMA),	// 0xBC	// ',' any country
  VK(OEM_MINUS),	// 0xBD	// '-' any country
  VK(OEM_PERIOD),	// 0xBE	// '.' any country
  VK(OEM_2),		// 0xBF	// '/?' for US
  VK(OEM_3),		// 0xC0	// '`~' for US

/*
 * 0xC1 - 0xD7 : reserved
 */

/*
 * 0xD8 - 0xDA : unassigned
 */

  VK(OEM_4),		// 0xDB	//  '[{' for US
  VK(OEM_5),		// 0xDC	//  '\|' for US
  VK(OEM_6),		// 0xDD	//  ']}' for US
  VK(OEM_7),		// 0xDE	//  ''"' for US
  VK(OEM_8),		// 0xDF

/*
 * 0xE0 : reserved
 */

/*
 * Various extended or enhanced keyboards
 */
  VK(OEM_AX),		// 0xE1	//  'AX' key on Japanese AX kbd
  VK(OEM_102),		// 0xE2	//  "<>" or "\|" on RT 102-key kbd.
  VK(ICO_HELP),		// 0xE3	//  Help key on ICO
  VK(ICO_00),		// 0xE4	//  00 key on ICO

#if(WINVER >= 0x0400)
  VK(PROCESSKEY),	// 0xE5
#endif /* WINVER >= 0x0400 */

  VK(ICO_CLEAR),	// 0xE6

#if(_WIN32_WINNT >= 0x0500)
  VK(PACKET),		// 0xE7
#endif /* _WIN32_WINNT >= 0x0500 */

/*
 * 0xE8 : unassigned
 */

/*
 * Nokia/Ericsson definitions
 */
  VK(OEM_RESET),	// 0xE9
  VK(OEM_JUMP),		// 0xEA
  VK(OEM_PA1),		// 0xEB
  VK(OEM_PA2),		// 0xEC
  VK(OEM_PA3),		// 0xED
  VK(OEM_WSCTRL),	// 0xEE
  VK(OEM_CUSEL),	// 0xEF
  VK(OEM_ATTN),		// 0xF0
  VK(OEM_FINISH),	// 0xF1
  VK(OEM_COPY),		// 0xF2
  VK(OEM_AUTO),		// 0xF3
  VK(OEM_ENLW),		// 0xF4
  VK(OEM_BACKTAB),	// 0xF5

  VK(ATTN),		// 0xF6
  VK(CRSEL),		// 0xF7
  VK(EXSEL),		// 0xF8
  VK(EREOF),		// 0xF9
  VK(PLAY),		// 0xFA
  VK(ZOOM),		// 0xFB
  VK(NONAME),		// 0xFC
  VK(PA1),		// 0xFD
  VK(OEM_CLEAR),	// 0xFE

/*
 * from Ime.h
 */

#if !defined(VK_DBE_ALPHANUMERIC)
  VK(DBE_ALPHANUMERIC),			// 0x0f0
  VK(DBE_KATAKANA),			// 0x0f1
  VK(DBE_HIRAGANA),			// 0x0f2
  VK(DBE_SBCSCHAR),			// 0x0f3
  VK(DBE_DBCSCHAR),			// 0x0f4
  VK(DBE_ROMAN),			// 0x0f5
  VK(DBE_NOROMAN),			// 0x0f6
  VK(DBE_ENTERWORDREGISTERMODE),	// 0x0f7
  VK(DBE_ENTERIMECONFIGMODE),		// 0x0f8
  VK(DBE_FLUSHSTRING),			// 0x0f9
  VK(DBE_CODEINPUT),			// 0x0fa
  VK(DBE_NOCODEINPUT),			// 0x0fb
  VK(DBE_DETERMINESTRING),		// 0x0fc
  VK(DBE_ENTERDLGCONVERSIONMODE),	// 0x0fd
#endif

  { 0, NULL },
#undef VK
};
