//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// dlgsetting.h


#ifndef _DLGSETTING_H
#  define _DLGSETTING_H

#  include <windows.h>


///
#ifdef MAYU64
INT_PTR CALLBACK dlgSetting_dlgProc(
#else
BOOL CALLBACK dlgSetting_dlgProc(
#endif
	HWND i_hwnd, UINT i_message, WPARAM i_wParam, LPARAM i_lParam);


#endif // !_DLGSETTING_H
