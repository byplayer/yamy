//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// dlgversion.h


#ifndef _DLGVERSION_H
#  define _DLGVERSION_H

#  include <windows.h>


///
#ifdef MAYU64
INT_PTR CALLBACK dlgVersion_dlgProc(
#else
BOOL CALLBACK dlgVersion_dlgProc(
#endif
	HWND i_hwnd, UINT i_message, WPARAM i_wParam, LPARAM i_lParam);


#endif // !_DLGVERSION_H
