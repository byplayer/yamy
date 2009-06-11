//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// windowstool.h


#ifndef _WINDOWSTOOL_H
#  define _WINDOWSTOOL_H


#  include "stringtool.h"
#  include <windows.h>


/// instance handle of this application
extern HINSTANCE g_hInst;


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// resource

/// load resource string
extern tstring loadString(UINT i_id);

/// load small icon resource (it must be deleted by DestroyIcon())
extern HICON loadSmallIcon(UINT i_id);

///load big icon resource (it must be deleted by DestroyIcon())
extern HICON loadBigIcon(UINT i_id);


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// window

/// resize the window (it does not move the window)
extern bool resizeWindow(HWND i_hwnd, int i_w, int i_h, bool i_doRepaint);

/** get rect of the window in client coordinates.
    @return rect of the window in client coordinates */
extern bool getChildWindowRect(HWND i_hwnd, RECT *o_rc);

/** set small icon to the specified window.
    @return handle of previous icon or NULL */
extern HICON setSmallIcon(HWND i_hwnd, UINT i_id);

/** set big icon to the specified window.
    @return handle of previous icon or NULL */
extern HICON setBigIcon(HWND i_hwnd, UINT i_id);

/// remove icon from a window that is set by setSmallIcon
extern void unsetSmallIcon(HWND i_hwnd);

/// remove icon from a window that is set by setBigIcon
extern void unsetBigIcon(HWND i_hwnd);

/// get toplevel (non-child) window
extern HWND getToplevelWindow(HWND i_hwnd, bool *io_isMDI);

/// move window asynchronously
extern void asyncMoveWindow(HWND i_hwnd, int i_x, int i_y);

/// move window asynchronously
extern void asyncMoveWindow(HWND i_hwnd, int i_x, int i_y, int i_w, int i_h);

/// resize asynchronously
extern void asyncResize(HWND i_hwnd, int i_w, int i_h);

/// get dll version
extern DWORD getDllVersion(const _TCHAR *i_dllname);
#define PACKVERSION(major, minor) MAKELONG(minor, major)

// workaround of SetForegroundWindow
extern bool setForegroundWindow(HWND i_hwnd);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// dialog

/// get/set GWL_USERDATA
template <class T> inline T getUserData(HWND i_hwnd, T *i_wc)
{
  return (*i_wc = reinterpret_cast<T>(GetWindowLong(i_hwnd, GWL_USERDATA)));
}

///
template <class T> inline T setUserData(HWND i_hwnd, T i_wc)
{
  SetWindowLong(i_hwnd, GWL_USERDATA, reinterpret_cast<long>(i_wc));
  return i_wc;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// RECT

///
inline int rcWidth(const RECT *i_rc) { return i_rc->right - i_rc->left; }

///
inline int rcHeight(const RECT *i_rc) { return i_rc->bottom - i_rc->top; }

///
inline bool isRectInRect(const RECT *i_rcin, const RECT *i_rcout)
{
  return (i_rcout->left <= i_rcin->left &&
	  i_rcin->right <= i_rcout->right &&
	  i_rcout->top <= i_rcin->top &&
	  i_rcin->bottom <= i_rcout->bottom);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// edit control

/// returns bytes of text
extern size_t editGetTextBytes(HWND i_hwnd);

/// delete a line
extern void editDeleteLine(HWND i_hwnd, size_t i_n);

/// insert text at last
extern void editInsertTextAtLast(HWND i_hwnd, const tstring &i_text,
				 size_t i_threshold);


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Windows2000/XP specific API

/// SetLayeredWindowAttributes API
typedef BOOL (WINAPI *SetLayeredWindowAttributes_t)
  (HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags);
extern SetLayeredWindowAttributes_t setLayeredWindowAttributes;

/// MonitorFromWindow API
extern HMONITOR (WINAPI *monitorFromWindow)(HWND hwnd, DWORD dwFlags);

/// GetMonitorInfo API
extern BOOL (WINAPI *getMonitorInfo)(HMONITOR hMonitor, LPMONITORINFO lpmi);

/// EnumDisplayMonitors API
extern BOOL (WINAPI *enumDisplayMonitors)
  (HDC hdc, LPRECT lprcClip, MONITORENUMPROC lpfnEnum, LPARAM dwData);


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// WindowsXP specific API

/// WTSRegisterSessionNotification API
typedef BOOL (WINAPI *WTSRegisterSessionNotification_t)
  (HWND hWnd, DWORD dwFlags);
extern WTSRegisterSessionNotification_t wtsRegisterSessionNotification;

/// WTSUnRegisterSessionNotification API
typedef BOOL (WINAPI *WTSUnRegisterSessionNotification_t)(HWND hWnd);
extern WTSUnRegisterSessionNotification_t wtsUnRegisterSessionNotification;

/// WTSGetActiveConsoleSessionId API
typedef DWORD (WINAPI *WTSGetActiveConsoleSessionId_t)(void);
extern WTSGetActiveConsoleSessionId_t wtsGetActiveConsoleSessionId;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Utility

// PathRemoveFileSpec()
tstring pathRemoveFileSpec(const tstring &i_path);


#endif // _WINDOWSTOOL_H
