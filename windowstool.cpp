//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// windowstool.cpp


#include "misc.h"

#include "windowstool.h"
#include "array.h"

#include <windowsx.h>
#include <malloc.h>
#include <shlwapi.h>


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Global variables


// instance handle of this application
HINSTANCE g_hInst = NULL;


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Functions


// load resource string
tstring loadString(UINT i_id)
{
  _TCHAR buf[1024];
  if (LoadString(g_hInst, i_id, buf, NUMBER_OF(buf)))
    return tstring(buf);
  else
    return _T("");
}


// load small icon resource
HICON loadSmallIcon(UINT i_id)
{
  return reinterpret_cast<HICON>(
    LoadImage(g_hInst, MAKEINTRESOURCE(i_id), IMAGE_ICON, 16, 16, 0));
}


// load big icon resource
HICON loadBigIcon(UINT i_id)
{
  return reinterpret_cast<HICON>(
    LoadImage(g_hInst, MAKEINTRESOURCE(i_id), IMAGE_ICON, 32, 32, 0));
}


// set small icon to the specified window.
// @return handle of previous icon or NULL
HICON setSmallIcon(HWND i_hwnd, UINT i_id)
{
  HICON hicon = (i_id == static_cast<UINT>(-1)) ? NULL : loadSmallIcon(i_id);
  return reinterpret_cast<HICON>(
    SendMessage(i_hwnd, WM_SETICON, static_cast<WPARAM>(ICON_SMALL),
		reinterpret_cast<LPARAM>(hicon)));
}


// set big icon to the specified window.
// @return handle of previous icon or NULL
HICON setBigIcon(HWND i_hwnd, UINT i_id)
{
  HICON hicon = (i_id == static_cast<UINT>(-1)) ? NULL : loadBigIcon(i_id);
  return reinterpret_cast<HICON>(
    SendMessage(i_hwnd, WM_SETICON, static_cast<WPARAM>(ICON_BIG),
		reinterpret_cast<LPARAM>(hicon)));
}


// remove icon from a window that is set by setSmallIcon
void unsetSmallIcon(HWND i_hwnd)
{
  HICON hicon = setSmallIcon(i_hwnd, -1);
  if (hicon)
    CHECK_TRUE( DestroyIcon(hicon) );
}


// remove icon from a window that is set by setBigIcon
void unsetBigIcon(HWND i_hwnd)
{
  HICON hicon = setBigIcon(i_hwnd, -1);
  if (hicon)
    CHECK_TRUE( DestroyIcon(hicon) );
}


// resize the window (it does not move the window)
bool resizeWindow(HWND i_hwnd, int i_w, int i_h, bool i_doRepaint)
{
  UINT flag = SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER;
  if (!i_doRepaint)
    flag |= SWP_NOREDRAW;
  return !!SetWindowPos(i_hwnd, NULL, 0, 0, i_w, i_h, flag);
}


// get rect of the window in client coordinates
// @return rect of the window in client coordinates
bool getChildWindowRect(HWND i_hwnd, RECT *o_rc)
{
  if (!GetWindowRect(i_hwnd, o_rc))
    return false;
  POINT p = { o_rc->left, o_rc->top };
  HWND phwnd = GetParent(i_hwnd);
  if (!phwnd)
    return false;
  if (!ScreenToClient(phwnd, &p))
    return false;
  o_rc->left = p.x;
  o_rc->top = p.y;
  p.x = o_rc->right;
  p.y = o_rc->bottom;
  ScreenToClient(phwnd, &p);
  o_rc->right = p.x;
  o_rc->bottom = p.y;
  return true;
}


// get toplevel (non-child) window
HWND getToplevelWindow(HWND i_hwnd, bool *io_isMDI)
{
  while (i_hwnd)
  {
    LONG style = GetWindowLong(i_hwnd, GWL_STYLE);
    if ((style & WS_CHILD) == 0)
      break;
    if (io_isMDI && *io_isMDI)
    {
      LONG exStyle = GetWindowLong(i_hwnd, GWL_EXSTYLE);
      if (exStyle & WS_EX_MDICHILD)
	return i_hwnd;
    }
    i_hwnd = GetParent(i_hwnd);
  }
  if (io_isMDI)
    *io_isMDI = false;
  return i_hwnd;
}


// move window asynchronously
void asyncMoveWindow(HWND i_hwnd, int i_x, int i_y)
{
  SetWindowPos(i_hwnd, NULL, i_x, i_y, 0, 0,
	       SWP_ASYNCWINDOWPOS | SWP_NOACTIVATE | SWP_NOOWNERZORDER |
	       SWP_NOSIZE | SWP_NOZORDER);
}


// move window asynchronously
void asyncMoveWindow(HWND i_hwnd, int i_x, int i_y, int i_w, int i_h)
{
  SetWindowPos(i_hwnd, NULL, i_x, i_y, i_w, i_h,
	       SWP_ASYNCWINDOWPOS | SWP_NOACTIVATE | SWP_NOOWNERZORDER |
	       SWP_NOZORDER);
}


// resize asynchronously
void asyncResize(HWND i_hwnd, int i_w, int i_h)
{
  SetWindowPos(i_hwnd, NULL, 0, 0, i_w, i_h,
	       SWP_ASYNCWINDOWPOS | SWP_NOACTIVATE | SWP_NOOWNERZORDER |
	       SWP_NOMOVE | SWP_NOZORDER);
}


// get dll version
DWORD getDllVersion(const _TCHAR *i_dllname)
{
  DWORD dwVersion = 0;
  
  if (HINSTANCE hinstDll = LoadLibrary(i_dllname))
  {
    DLLGETVERSIONPROC pDllGetVersion
      = (DLLGETVERSIONPROC)GetProcAddress(hinstDll, "DllGetVersion");
    /* Because some DLLs may not implement this function, you
     * must test for it explicitly. Depending on the particular 
     * DLL, the lack of a DllGetVersion function may
     * be a useful indicator of the version.
     */
    if (pDllGetVersion)
    {
      DLLVERSIONINFO dvi;
      ZeroMemory(&dvi, sizeof(dvi));
      dvi.cbSize = sizeof(dvi);

      HRESULT hr = (*pDllGetVersion)(&dvi);
      if (SUCCEEDED(hr))
	dwVersion = PACKVERSION(dvi.dwMajorVersion, dvi.dwMinorVersion);
    }
        
    FreeLibrary(hinstDll);
  }
  return dwVersion;
}


// workaround of SetForegroundWindow
bool setForegroundWindow(HWND i_hwnd)
{
  int nForegroundID = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
  int nTargetID = GetWindowThreadProcessId(i_hwnd, NULL);
  
  //if (!AttachThreadInput(nTargetID, nForegroundID, TRUE))
  //return false;
  AttachThreadInput(nTargetID, nForegroundID, TRUE);

  DWORD sp_time;
  SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, &sp_time, 0);
  SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, (void *)0, 0);

  SetForegroundWindow(i_hwnd);

  SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, (void *)sp_time, 0);
  
  AttachThreadInput(nTargetID, nForegroundID, FALSE);
  return true;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// edit control


// get edit control's text size
// @return bytes of text
size_t editGetTextBytes(HWND i_hwnd)
{
  return Edit_GetTextLength(i_hwnd);
}


// delete a line
void editDeleteLine(HWND i_hwnd, size_t i_n)
{
  int len = Edit_LineLength(i_hwnd, i_n);
  if (len < 0)
    return;
  len += 2;
  int index = Edit_LineIndex(i_hwnd, i_n);
  Edit_SetSel(i_hwnd, index, index + len);
  Edit_ReplaceSel(i_hwnd, _T(""));
}
  

// insert text at last
void editInsertTextAtLast(HWND i_hwnd, const tstring &i_text,
			  size_t i_threshold)
{
  if (i_text.empty())
    return;
  
  size_t len = editGetTextBytes(i_hwnd);
  
  if (i_threshold < len)
  {
    Edit_SetSel(i_hwnd, 0, len / 3 * 2);
    Edit_ReplaceSel(i_hwnd, _T(""));
    editDeleteLine(i_hwnd, 0);
    len = editGetTextBytes(i_hwnd);
  }
  
  Edit_SetSel(i_hwnd, len, len);
  
  // \n -> \r\n
  Array<_TCHAR> buf(i_text.size() * 2 + 1);
  _TCHAR *d = buf.get();
  const _TCHAR *str = i_text.c_str();
  for (const _TCHAR *s = str; s < str + i_text.size(); ++ s)
  {
    if (*s == _T('\n'))
      *d++ = _T('\r');
    *d++ = *s;
  }
  *d = _T('\0');
  
  Edit_ReplaceSel(i_hwnd, buf.get());
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Windows2000/XP specific API


// initialize layerd window
static BOOL WINAPI initalizeLayerdWindow(
  HWND i_hwnd, COLORREF i_crKey, BYTE i_bAlpha, DWORD i_dwFlags)
{
  HMODULE hModule = GetModuleHandle(_T("user32.dll"));
  if (!hModule) {
    return FALSE;
  }
  SetLayeredWindowAttributes_t proc = 
    reinterpret_cast<SetLayeredWindowAttributes_t>(
      GetProcAddress(hModule, "SetLayeredWindowAttributes"));
  if (setLayeredWindowAttributes) {
    setLayeredWindowAttributes = proc;
    return setLayeredWindowAttributes(i_hwnd, i_crKey, i_bAlpha, i_dwFlags);
  } else {
    return FALSE;
  }
}


// SetLayeredWindowAttributes API
SetLayeredWindowAttributes_t setLayeredWindowAttributes
  = initalizeLayerdWindow;


// emulate MonitorFromWindow API
static HMONITOR WINAPI emulateMonitorFromWindow(HWND hwnd, DWORD dwFlags)
{
  return reinterpret_cast<HMONITOR>(1); // dummy HMONITOR
}

// initialize MonitorFromWindow API
static HMONITOR WINAPI initializeMonitorFromWindow(HWND hwnd, DWORD dwFlags)
{
  HMODULE hModule = GetModuleHandle(_T("user32.dll"));
  if (!hModule)
    return FALSE;

  FARPROC proc = GetProcAddress(hModule, "MonitorFromWindow");
  if(proc)
    monitorFromWindow =
      reinterpret_cast<HMONITOR (WINAPI *)(HWND, DWORD)>(proc);
  else
    monitorFromWindow = emulateMonitorFromWindow;

  return monitorFromWindow(hwnd, dwFlags);
}

// MonitorFromWindow API
HMONITOR (WINAPI *monitorFromWindow)(HWND hwnd, DWORD dwFlags)
    = initializeMonitorFromWindow;


// emulate GetMonitorInfo API
static BOOL WINAPI emulateGetMonitorInfo(HMONITOR hMonitor, LPMONITORINFO lpmi)
{
  if(lpmi->cbSize != sizeof(MONITORINFO))
    return FALSE;

  lpmi->rcMonitor.left = 0;
  lpmi->rcMonitor.top = 0;
  lpmi->rcMonitor.right = GetSystemMetrics(SM_CXFULLSCREEN);
  lpmi->rcMonitor.bottom = GetSystemMetrics(SM_CYFULLSCREEN);
  SystemParametersInfo(SPI_GETWORKAREA, 0,
                       reinterpret_cast<PVOID>(&lpmi->rcWork), FALSE);
  lpmi->dwFlags = MONITORINFOF_PRIMARY;

  return TRUE;
}

// initialize GetMonitorInfo API
static
BOOL WINAPI initializeGetMonitorInfo(HMONITOR hMonitor, LPMONITORINFO lpmi)
{
  HMODULE hModule = GetModuleHandle(_T("user32.dll"));
  if (!hModule)
    return FALSE;

  FARPROC proc = GetProcAddress(hModule, "GetMonitorInfoA");
  if(proc)
    getMonitorInfo =
      reinterpret_cast<BOOL (WINAPI *)(HMONITOR, LPMONITORINFO)>(proc);
  else
    getMonitorInfo = emulateGetMonitorInfo;

  return getMonitorInfo(hMonitor, lpmi);
}

// GetMonitorInfo API
BOOL (WINAPI *getMonitorInfo)(HMONITOR hMonitor, LPMONITORINFO lpmi)
  = initializeGetMonitorInfo;


// enumalte EnumDisplayMonitors API
static BOOL WINAPI emulateEnumDisplayMonitors(
  HDC hdc, LPRECT lprcClip, MONITORENUMPROC lpfnEnum, LPARAM dwData)
{
  lpfnEnum(reinterpret_cast<HMONITOR>(1), hdc, lprcClip, dwData);
  return TRUE;
}

// initialize EnumDisplayMonitors API
static BOOL WINAPI initializeEnumDisplayMonitors(
  HDC hdc, LPRECT lprcClip, MONITORENUMPROC lpfnEnum, LPARAM dwData)
{
  HMODULE hModule = GetModuleHandle(_T("user32.dll"));
  if (!hModule)
    return FALSE;

  FARPROC proc = GetProcAddress(hModule, "EnumDisplayMonitors");
  if(proc)
    enumDisplayMonitors =
      reinterpret_cast<BOOL (WINAPI *)(HDC, LPRECT, MONITORENUMPROC, LPARAM)>
      (proc);
  else
    enumDisplayMonitors = emulateEnumDisplayMonitors;

  return enumDisplayMonitors(hdc, lprcClip, lpfnEnum, dwData);
}

// EnumDisplayMonitors API
BOOL (WINAPI *enumDisplayMonitors)
    (HDC hdc, LPRECT lprcClip, MONITORENUMPROC lpfnEnum, LPARAM dwData)
  = initializeEnumDisplayMonitors;


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Windows2000/XP specific API


static BOOL WINAPI
initializeWTSRegisterSessionNotification(HWND hWnd, DWORD dwFlags)
{
  LoadLibrary(_T("wtsapi32.dll"));
  HMODULE hModule = GetModuleHandle(_T("wtsapi32.dll"));
  if (!hModule) {
    return FALSE;
  }
  WTSRegisterSessionNotification_t proc = 
    reinterpret_cast<WTSRegisterSessionNotification_t>(
      GetProcAddress(hModule, "WTSRegisterSessionNotification"));
  if (proc) {
    wtsRegisterSessionNotification = proc;
    return wtsRegisterSessionNotification(hWnd, dwFlags);
  } else {
    return 0;
  }
}

// WTSRegisterSessionNotification API
WTSRegisterSessionNotification_t wtsRegisterSessionNotification
  = initializeWTSRegisterSessionNotification;


static BOOL WINAPI initializeWTSUnRegisterSessionNotification(HWND hWnd)
{
  HMODULE hModule = GetModuleHandle(_T("wtsapi32.dll"));
  if (!hModule) {
    return FALSE;
  }
  WTSUnRegisterSessionNotification_t proc = 
    reinterpret_cast<WTSUnRegisterSessionNotification_t>(
      GetProcAddress(hModule, "WTSUnRegisterSessionNotification"));
  if (proc) {
    wtsUnRegisterSessionNotification = proc;
    return wtsUnRegisterSessionNotification(hWnd);
  } else {
    return 0;
  }
}

// WTSUnRegisterSessionNotification API
WTSUnRegisterSessionNotification_t wtsUnRegisterSessionNotification
  = initializeWTSUnRegisterSessionNotification;


static DWORD WINAPI initializeWTSGetActiveConsoleSessionId(void)
{
  HMODULE hModule = GetModuleHandle(_T("kernel32.dll"));
  if (!hModule) {
    return FALSE;
  }
  WTSGetActiveConsoleSessionId_t proc = 
    reinterpret_cast<WTSGetActiveConsoleSessionId_t>(
      GetProcAddress(hModule, "WTSGetActiveConsoleSessionId"));
  if (proc) {
    wtsGetActiveConsoleSessionId = proc;
    return wtsGetActiveConsoleSessionId();
  } else {
    return 0;
  }
}

// WTSGetActiveConsoleSessionId API
WTSGetActiveConsoleSessionId_t wtsGetActiveConsoleSessionId
  = initializeWTSGetActiveConsoleSessionId;


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Utility

// PathRemoveFileSpec()
tstring pathRemoveFileSpec(const tstring &i_path)
{
  const _TCHAR *str = i_path.c_str();
  const _TCHAR *b = _tcsrchr(str, _T('\\'));
  const _TCHAR *s = _tcsrchr(str, _T('/'));
  if (b && s)
    return tstring(str, MIN(b, s));
  if (b)
    return tstring(str, b);
  if (s)
    return tstring(str, s);
  if (const _TCHAR *c = _tcsrchr(str, _T(':')))
    return tstring(str, c + 1);
  return i_path;
}
