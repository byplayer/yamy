//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// hook.cpp


#define _HOOK_CPP

#include "misc.h"

#include "hook.h"
#include "stringtool.h"

#include <locale.h>
#include <imm.h>
#include <richedit.h>


///
#define HOOK_DATA_NAME _T("{08D6E55C-5103-4e00-8209-A1C4AB13BBEF}") _T(VERSION)
#ifdef _WIN64
#define HOOK_DATA_NAME_ARCH _T("{290C0D51-8AEE-403d-9172-E43D46270996}") _T(VERSION)
#else // !_WIN64
#define HOOK_DATA_NAME_ARCH _T("{716A5DEB-CB02-4438-ABC8-D00E48673E45}") _T(VERSION)
#endif // !_WIN64

// Some applications use different values for below messages
// when double click of title bar.
#define SC_MAXIMIZE2 (SC_MAXIMIZE + 2)
#define SC_MINIMIZE2 (SC_MINIMIZE + 2)
#define SC_RESTORE2 (SC_RESTORE + 2)

// Debug Macros
#ifdef NDEBUG
#define HOOK_RPT0(msg)
#define HOOK_RPT1(msg, arg1)
#define HOOK_RPT2(msg, arg1, arg2)
#else
#define HOOK_RPT0(msg) if (g.m_isLogging) { _RPT0(_CRT_WARN, msg); }
#define HOOK_RPT1(msg, arg1) if (g.m_isLogging) { _RPT1(_CRT_WARN, msg, arg1); }
#define HOOK_RPT2(msg, arg1, arg2) if (g.m_isLogging) { _RPT2(_CRT_WARN, msg, arg1, arg2); }
#endif

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Global Variables


DllExport HookData *g_hookData;			///

///
class HookDataArch
{
public:
  HHOOK m_hHookGetMessage;			///
  HHOOK m_hHookCallWndProc;			///
};

static HookDataArch *s_hookDataArch;

struct Globals
{
  HANDLE m_hHookData;				///
  HANDLE m_hHookDataArch;			///
  HWND m_hwndFocus;				/// 
  HINSTANCE m_hInstDLL;				///
  bool m_isInMenu;				///
  UINT m_WM_MAYU_MESSAGE;			///
  bool m_isImeLock;				///
  bool m_isImeCompositioning;			///
  HHOOK m_hHookMouseProc;			///
#ifdef NO_DRIVER
  HHOOK m_hHookKeyboardProc;			///
  KEYBOARD_DETOUR m_keyboardDetour;
  Engine *m_engine;
#endif // NO_DRIVER
  DWORD m_hwndTaskTray;				///
  HANDLE m_hMailslot;
  bool m_isInitialized;
#ifndef NDEBUG
  bool m_isLogging;
  _TCHAR m_moduleName[GANA_MAX_PATH];
#endif // !NDEBUG
};

static Globals g;


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Prototypes


static void notifyThreadDetach();
static void notifyShow(NotifyShow::Show i_show, bool i_isMDI);
static void notifyLog(_TCHAR *i_msg);
static bool mapHookData();
static void unmapHookData();
static bool initialize();


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Functions

bool initialize()
{
#ifndef NDEBUG
  _TCHAR path[GANA_MAX_PATH];
  GetModuleFileName(NULL, path, GANA_MAX_PATH);
  _tsplitpath_s(path, NULL, 0, NULL, 0, g.m_moduleName, GANA_MAX_PATH, NULL, 0);
  if (_tcsncmp(g.m_moduleName, _T("Dbgview"), sizeof(_T("Dbgview"))/sizeof(_TCHAR)) != 0 &&
      _tcsncmp(g.m_moduleName, _T("windbg"), sizeof(_T("windbg"))/sizeof(_TCHAR)) != 0)
  {
	g.m_isLogging = true;
  }
#endif // !NDEBUG
#ifdef USE_MAILSLOT
  g.m_hMailslot =
	  CreateFile(NOTIFY_MAILSLOT_NAME, GENERIC_WRITE,
		     FILE_SHARE_READ | FILE_SHARE_WRITE,
		     (SECURITY_ATTRIBUTES *)NULL, OPEN_EXISTING,
		     FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
  if (g.m_hMailslot == INVALID_HANDLE_VALUE)
  {
    HOOK_RPT2("MAYU: %S create mailslot failed(0x%08x)\r\n", g.m_moduleName, GetLastError());
  }
  else
  {
    HOOK_RPT1("MAYU: %S create mailslot successed\r\n", g.m_moduleName);
  }
#endif //USE_MAILSLOT
  if (!mapHookData())
    return false;
  _tsetlocale(LC_ALL, _T(""));
  g.m_WM_MAYU_MESSAGE =
    RegisterWindowMessage(addSessionId(WM_MAYU_MESSAGE_NAME).c_str());
  g.m_hwndTaskTray = g_hookData->m_hwndTaskTray;
  g.m_isInitialized = true;
  return true;
}

/// EntryPoint
BOOL WINAPI DllMain(HINSTANCE i_hInstDLL, DWORD i_fdwReason,
		    LPVOID /* i_lpvReserved */)
{
  switch (i_fdwReason)
  {
    case DLL_PROCESS_ATTACH:
    {
#ifndef NDEBUG
      g.m_isLogging = false;
#endif // !NDEBUG
      g.m_isInitialized = false;
      g.m_hInstDLL = i_hInstDLL;
      break;
    }
    case DLL_THREAD_ATTACH:
      break;
    case DLL_PROCESS_DETACH:
      notifyThreadDetach();
      unmapHookData();
#ifdef USE_MAILSLOT
      if (g.m_hMailslot != INVALID_HANDLE_VALUE)
      {
	CloseHandle(g.m_hMailslot);
	g.m_hMailslot = INVALID_HANDLE_VALUE;
      }
#endif //USE_MAILSLOT
      break;
    case DLL_THREAD_DETACH:
      notifyThreadDetach();
      break;
    default:
      break;
  }
  return TRUE;
}


/// map hook data
static bool mapHookData()
{
  g.m_hHookData = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
				    0, sizeof(HookData),
				    addSessionId(HOOK_DATA_NAME).c_str());
  if (!g.m_hHookData)
  {
    unmapHookData();
    return false;
  }
  
  g_hookData =
    (HookData *)MapViewOfFile(g.m_hHookData, FILE_MAP_READ | FILE_MAP_WRITE,
			      0, 0, sizeof(HookData));
  if (!g_hookData)
  {
    unmapHookData();
    return false;
  }

  g.m_hHookDataArch = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
				    0, sizeof(HookDataArch),
				    addSessionId(HOOK_DATA_NAME_ARCH).c_str());
  if (!g.m_hHookDataArch)
  {
    unmapHookData();
    return false;
  }
  
  s_hookDataArch =
    (HookDataArch *)MapViewOfFile(g.m_hHookDataArch, FILE_MAP_READ | FILE_MAP_WRITE,
			      0, 0, sizeof(HookDataArch));
  if (!s_hookDataArch)
  {
    unmapHookData();
    return false;
  }

  return true;
}


/// unmap hook data
static void unmapHookData()
{
  if (g_hookData)
    UnmapViewOfFile(g_hookData);
  g_hookData = NULL;
  if (g.m_hHookData)
    CloseHandle(g.m_hHookData);
  g.m_hHookData = NULL;
  if (s_hookDataArch)
    UnmapViewOfFile(s_hookDataArch);
  s_hookDataArch = NULL;
  if (g.m_hHookDataArch)
    CloseHandle(g.m_hHookDataArch);
  g.m_hHookDataArch = NULL;
}


/// notify
DllExport bool notify(void *i_data, size_t i_dataSize)
{
  COPYDATASTRUCT cd;
#ifdef MAYU64
  DWORD_PTR result;
#else  // MAYU64
  DWORD result;
#endif // MAYU64

#ifdef USE_MAILSLOT
  DWORD len;
  if (g.m_hMailslot != INVALID_HANDLE_VALUE)
  {
    BOOL ret;
    ret = WriteFile(g.m_hMailslot, i_data, i_dataSize, &len, NULL);
#ifndef NDEBUG
    if (ret == 0)
    {
      HOOK_RPT2("MAYU: %S WriteFile to mailslot failed(0x%08x)\r\n", g.m_moduleName, GetLastError());
    }
    else
    {
      HOOK_RPT1("MAYU: %S WriteFile to mailslot successed\r\n", g.m_moduleName);
    }
#endif // !NDEBUG
  }
#else // !USE_MAILSLOT
  cd.dwData = reinterpret_cast<Notify *>(i_data)->m_type;
  cd.cbData = i_dataSize;
  cd.lpData = i_data;
  if (g.m_hwndTaskTray == 0)
    return false;
  if (!SendMessageTimeout(reinterpret_cast<HWND>(g.m_hwndTaskTray),
			  WM_COPYDATA, NULL, reinterpret_cast<LPARAM>(&cd),
			  SMTO_ABORTIFHUNG | SMTO_NORMAL, 5000, &result))
  {
    _RPT0(_CRT_WARN, "MAYU: SendMessageTimeout() timeouted\r\n");
    return false;
  }
#endif // !USE_MAILSLOT
  return true;
}


/// get class name and title name
static void getClassNameTitleName(HWND i_hwnd, bool i_isInMenu, 
				  tstringi *o_className,
				  tstring *o_titleName)
{
  tstringi &className = *o_className;
  tstring &titleName = *o_titleName;
  
  bool isTheFirstTime = true;
  
  if (i_isInMenu)
  {
    className = titleName = _T("MENU");
    isTheFirstTime = false;
  }

  while (true)
  {
    _TCHAR buf[MAX(GANA_MAX_PATH, GANA_MAX_ATOM_LENGTH)];

    // get class name
    if (i_hwnd)
      GetClassName(i_hwnd, buf, NUMBER_OF(buf));
    else
      GetModuleFileName(GetModuleHandle(NULL), buf, NUMBER_OF(buf));
    buf[NUMBER_OF(buf) - 1] = _T('\0');
    if (isTheFirstTime)
      className = buf;
    else
      className = tstringi(buf) + _T(":") + className;
    
    // get title name
    if (i_hwnd)
    {
      GetWindowText(i_hwnd, buf, NUMBER_OF(buf));
      buf[NUMBER_OF(buf) - 1] = _T('\0');
      for (_TCHAR *b = buf; *b; ++ b)
	if (_istlead(*b) && b[1])
	  b ++;
	else if (_istcntrl(*b))
	  *b = _T('?');
    }
    if (isTheFirstTime)
      titleName = buf;
    else
      titleName = tstring(buf) + _T(":") + titleName;

    // next loop or exit
    if (!i_hwnd)
      break;
    i_hwnd = GetParent(i_hwnd);
    isTheFirstTime = false;
  }
}


/// update show
static void updateShow(HWND i_hwnd, NotifyShow::Show i_show)
{
  bool isMDI = false;

  if (!i_hwnd)
    return;

#ifdef MAYU64
  LONG_PTR style = GetWindowLongPtr(i_hwnd, GWL_STYLE);
#else
  LONG style = GetWindowLong(i_hwnd, GWL_STYLE);
#endif
  if (!(style & WS_MAXIMIZEBOX) && !(style & WS_MAXIMIZEBOX))
    return; // ignore window that has neither maximize or minimize button

  if (style & WS_CHILD)
  {
#ifdef MAYU64
    LONG_PTR exStyle = GetWindowLongPtr(i_hwnd, GWL_EXSTYLE);
#else
    LONG exStyle = GetWindowLong(i_hwnd, GWL_EXSTYLE);
#endif
    if (exStyle & WS_EX_MDICHILD)
    {
      isMDI = true;
    }
    else
      return; // ignore non-MDI child window case
  }

  notifyShow(i_show, isMDI);
}


/// notify WM_Targetted
static void notifyName(HWND i_hwnd, Notify::Type i_type = Notify::Type_name)
{
  tstringi className;
  tstring titleName;
  getClassNameTitleName(i_hwnd, g.m_isInMenu, &className, &titleName);
  
  NotifySetFocus *nfc = new NotifySetFocus;
  nfc->m_type = i_type;
  nfc->m_threadId = GetCurrentThreadId();
  nfc->m_hwnd = reinterpret_cast<DWORD>(i_hwnd);
  tcslcpy(nfc->m_className, className.c_str(), NUMBER_OF(nfc->m_className));
  tcslcpy(nfc->m_titleName, titleName.c_str(), NUMBER_OF(nfc->m_titleName));

  notify(nfc, sizeof(*nfc));
  delete nfc;
}


/// notify WM_SETFOCUS
static void notifySetFocus(bool i_doesForce = false)
{
  HWND hwnd = GetFocus();
  if (i_doesForce || hwnd != g.m_hwndFocus)
  {
    g.m_hwndFocus = hwnd;
    notifyName(hwnd, Notify::Type_setFocus);
  }
}


/// notify sync
static void notifySync()
{
  Notify n;
  n.m_type = Notify::Type_sync;
  notify(&n, sizeof(n));
}


/// notify DLL_THREAD_DETACH
static void notifyThreadDetach()
{
  NotifyThreadDetach ntd;
  ntd.m_type = Notify::Type_threadDetach;
  ntd.m_threadId = GetCurrentThreadId();
  notify(&ntd, sizeof(ntd));
}


/// notify WM_COMMAND, WM_SYSCOMMAND
static void notifyCommand(
  HWND i_hwnd, UINT i_message, WPARAM i_wParam, LPARAM i_lParam)
{
#ifndef _WIN64
  if (g_hookData->m_doesNotifyCommand)
  {
    NotifyCommand ntc;
    ntc.m_type = Notify::Type_command;
    ntc.m_hwnd = i_hwnd;
    ntc.m_message = i_message;
    ntc.m_wParam = i_wParam;
    ntc.m_lParam = i_lParam;
    notify(&ntc, sizeof(ntc));
  }
#endif
}


/// notify show of current window
static void notifyShow(NotifyShow::Show i_show, bool i_isMDI)
{
  NotifyShow ns;
  ns.m_type = Notify::Type_show;
  ns.m_show = i_show;
  ns.m_isMDI = i_isMDI;
  notify(&ns, sizeof(ns));
}


/// notify log
static void notifyLog(_TCHAR *i_msg)
{
  NotifyLog nl;
  nl.m_type = Notify::Type_log;
  tcslcpy(nl.m_msg, i_msg, NUMBER_OF(nl.m_msg));
  notify(&nl, sizeof(nl));
}


/// &Recenter
static void funcRecenter(HWND i_hwnd)
{
  _TCHAR buf[MAX(GANA_MAX_PATH, GANA_MAX_ATOM_LENGTH)];
  GetClassName(i_hwnd, buf, NUMBER_OF(buf));
  bool isEdit;
  if (_tcsicmp(buf, _T("Edit")) == 0)
    isEdit = true;
  else if (_tcsnicmp(buf, _T("RichEdit"), 8) == 0)
    isEdit = false;
  else
    return;	// this function only works for Edit control

#ifdef MAYU64
  LONG_PTR style = GetWindowLongPtr(i_hwnd, GWL_STYLE);
#else
  LONG style = GetWindowLong(i_hwnd, GWL_STYLE);
#endif
  if (!(style & ES_MULTILINE))
    return;	// this function only works for multi line Edit control

  RECT rc;
  GetClientRect(i_hwnd, &rc);
  POINTL p = { (rc.right + rc.left) / 2, (rc.top + rc.bottom) / 2 };
  int line;
  if (isEdit)
  {
    line = SendMessage(i_hwnd, EM_CHARFROMPOS, 0, MAKELPARAM(p.x, p.y));
    line = HIWORD(line);
  }
  else
  {
    int ci = SendMessage(i_hwnd, EM_CHARFROMPOS, 0, (LPARAM)&p);
    line = SendMessage(i_hwnd, EM_EXLINEFROMCHAR, 0, ci);
  }
  int caretLine = SendMessage(i_hwnd, EM_LINEFROMCHAR, -1, 0);
  SendMessage(i_hwnd, EM_LINESCROLL, 0, caretLine - line);
}


// &SetImeStatus
static void funcSetImeStatus(HWND i_hwnd, int i_status)
{
  HIMC hIMC;

  hIMC = ImmGetContext(i_hwnd);
  if (hIMC == INVALID_HANDLE_VALUE)
    return;

  if (i_status < 0)
    i_status = !ImmGetOpenStatus(hIMC);

  ImmSetOpenStatus(hIMC, i_status);
  ImmReleaseContext(i_hwnd, hIMC);
}


// &SetImeString
static void funcSetImeString(HWND i_hwnd, int i_size)
{
  _TCHAR *buf = new _TCHAR(i_size);
  DWORD len = 0;
  _TCHAR ImeDesc[GANA_MAX_ATOM_LENGTH];
  UINT ImeDescLen;
  DWORD error;
  DWORD denom = 1;
  HANDLE hPipe
    = CreateFile(addSessionId(HOOK_PIPE_NAME).c_str(), GENERIC_READ,
		 FILE_SHARE_READ, (SECURITY_ATTRIBUTES *)NULL,
		 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
  error = ReadFile(hPipe, buf, i_size, &len, NULL);
  CloseHandle(hPipe);

  ImeDescLen = ImmGetDescription(GetKeyboardLayout(0),
				 ImeDesc, sizeof(ImeDesc));
  if (_tcsncmp(ImeDesc, _T("SKKIME"), ImeDescLen) > 0)
    denom = sizeof(_TCHAR);

  HIMC hIMC = ImmGetContext(i_hwnd);
  if (hIMC == INVALID_HANDLE_VALUE)
    return;

  int status = ImmGetOpenStatus(hIMC);
  ImmSetCompositionString(hIMC, SCS_SETSTR, buf, len / denom, NULL, 0);
  delete buf;
  ImmNotifyIME(hIMC, NI_COMPOSITIONSTR, CPS_COMPLETE, 0);
  if (!status)
    ImmSetOpenStatus(hIMC, status);
  ImmReleaseContext(i_hwnd, hIMC);
}

/// notify lock state
/*DllExport*/ void notifyLockState(int i_cause)
{
  NotifyLockState n;
  n.m_type = Notify::Type_lockState;
  n.m_isNumLockToggled = !!(GetKeyState(VK_NUMLOCK) & 1);
  n.m_isCapsLockToggled = !!(GetKeyState(VK_CAPITAL) & 1);
  n.m_isScrollLockToggled = !!(GetKeyState(VK_SCROLL) & 1);
  n.m_isKanaLockToggled = !!(GetKeyState(VK_KANA) & 1);
  n.m_isImeLockToggled = g.m_isImeLock;
  n.m_isImeCompToggled = g.m_isImeCompositioning;
  n.m_debugParam = i_cause;
  notify(&n, sizeof(n));
}

DllExport void notifyLockState()
{
  notifyLockState(9);
}


/// hook of GetMessage
LRESULT CALLBACK getMessageProc(int i_nCode, WPARAM i_wParam, LPARAM i_lParam)
{
  if (!g.m_isInitialized)
    initialize();

  if (!g_hookData)
    return 0;
  
  MSG &msg = (*(MSG *)i_lParam);

  if (i_wParam != PM_REMOVE)
    goto finally;

  switch (msg.message)
  {
    case WM_COMMAND:
    case WM_SYSCOMMAND:
      notifyCommand(msg.hwnd, msg.message, msg.wParam, msg.lParam);
      break;
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    {
      if (HIMC hIMC = ImmGetContext(msg.hwnd))
      {
	bool prev = g.m_isImeLock;
	g.m_isImeLock = !!ImmGetOpenStatus(hIMC);
	ImmReleaseContext(msg.hwnd, hIMC);
	if (prev != g.m_isImeLock) {
	  notifyLockState(1);
	}
      }
      int nVirtKey = (int)msg.wParam;
      // int repeatCount = (msg.lParam & 0xffff);
      BYTE scanCode   = (BYTE)((msg.lParam >> 16) & 0xff);
      bool isExtended = !!(msg.lParam & (1 << 24));
      // bool isAltDown  = !!(msg.lParam & (1 << 29));
      // bool isKeyup    = !!(msg.lParam & (1 << 31));
      
      if (nVirtKey == VK_CAPITAL ||
	  nVirtKey == VK_NUMLOCK ||
	  nVirtKey == VK_KANA ||
	  nVirtKey == VK_SCROLL)
	notifyLockState(2);
      else if (scanCode == g_hookData->m_syncKey &&
	       isExtended == g_hookData->m_syncKeyIsExtended)
	notifySync();
      break;
    }
    case WM_IME_STARTCOMPOSITION:
      g.m_isImeCompositioning = true;
      notifyLockState(3);
      break;
    case WM_IME_ENDCOMPOSITION:
      g.m_isImeCompositioning = false;
      notifyLockState(4);
      break;
    default:
      if (i_wParam == PM_REMOVE && msg.message == g.m_WM_MAYU_MESSAGE)
      {
	switch (msg.wParam)
	{
	  case MayuMessage_notifyName:
	    notifyName(msg.hwnd);
	    break;
	  case MayuMessage_funcRecenter:
	    funcRecenter(msg.hwnd);
	    break;
	  case MayuMessage_funcSetImeStatus:
	    funcSetImeStatus(msg.hwnd, msg.lParam);
	    break;
	  case MayuMessage_funcSetImeString:
	    funcSetImeString(msg.hwnd, msg.lParam);
	    break;
	}
      }
      break;
  }
 finally:
  return CallNextHookEx(s_hookDataArch->m_hHookGetMessage,
			i_nCode, i_wParam, i_lParam);
}


/// hook of SendMessage
LRESULT CALLBACK callWndProc(int i_nCode, WPARAM i_wParam, LPARAM i_lParam)
{
  if (!g.m_isInitialized)
    initialize();

  if (!g_hookData)
    return 0;
  
  CWPSTRUCT &cwps = *(CWPSTRUCT *)i_lParam;
  
  if (0 <= i_nCode)
  {
    switch (cwps.message)
    {
      case WM_ACTIVATEAPP:
      case WM_NCACTIVATE:
	if (i_wParam)
	  notifySetFocus();
	break;
      case WM_SYSCOMMAND:
	switch (cwps.wParam)
	{
	  case SC_MAXIMIZE:
	  case SC_MAXIMIZE2:
	    updateShow(cwps.hwnd, NotifyShow::Show_Maximized);
	    break;
	  case SC_MINIMIZE:
	  case SC_MINIMIZE2:
	    updateShow(cwps.hwnd, NotifyShow::Show_Minimized);
	    break;
	  case SC_RESTORE:
	  case SC_RESTORE2:
	    updateShow(cwps.hwnd, NotifyShow::Show_Normal);
	    break;
	  default:
	    break;
	}
	/* through below */
      case WM_COMMAND:
	notifyCommand(cwps.hwnd, cwps.message, cwps.wParam, cwps.lParam);
	break;
      case WM_SIZE:
	switch (cwps.wParam)
	{
	  case SIZE_MAXIMIZED:
	    updateShow(cwps.hwnd, NotifyShow::Show_Maximized);
	    break;
	  case SIZE_MINIMIZED:
	    updateShow(cwps.hwnd, NotifyShow::Show_Minimized);
	    break;
	  case SIZE_RESTORED:
	    updateShow(cwps.hwnd, NotifyShow::Show_Normal);
	    break;
	  default:
	    break;
	}
	break;
      case WM_MOUSEACTIVATE:
	notifySetFocus();
	break;
      case WM_ACTIVATE:
	if (LOWORD(cwps.wParam) != WA_INACTIVE)
	{
	  notifySetFocus();
	  if (HIWORD(cwps.wParam)) // check minimized flag
	  {
	    // minimized flag on
	    notifyShow(NotifyShow::Show_Minimized, false);
	    //notifyShow(NotifyShow::Show_Normal, true);
	  }
	}
	break;
      case WM_ENTERMENULOOP:
	g.m_isInMenu = true;
	notifySetFocus(true);
	break;
      case WM_EXITMENULOOP:
	g.m_isInMenu = false;
	notifySetFocus(true);
	break;
      case WM_SETFOCUS:
	g.m_isInMenu = false;
	// for kana
	if (g_hookData->m_correctKanaLockHandling) {
	  if (HIMC hIMC = ImmGetContext(cwps.hwnd)) {
	    bool status = !!ImmGetOpenStatus(hIMC);
	    // this code set the VK_KANA state correctly.
	    ImmSetOpenStatus(hIMC, !status);
	    ImmSetOpenStatus(hIMC, status);
	    ImmReleaseContext(cwps.hwnd, hIMC);
	  }
	}
	notifySetFocus();
	notifyLockState(5);
	break;
      case WM_IME_STARTCOMPOSITION:
	g.m_isImeCompositioning = true;
	notifyLockState(6);
	break;
      case WM_IME_ENDCOMPOSITION:
	g.m_isImeCompositioning = false;
	notifyLockState(7);
	break;
      case WM_IME_NOTIFY:
	if (cwps.wParam == IMN_SETOPENSTATUS)
	  if (HIMC hIMC = ImmGetContext(cwps.hwnd))
	  {
	    g.m_isImeLock = !!ImmGetOpenStatus(hIMC);
	    ImmReleaseContext(cwps.hwnd, hIMC);
	    notifyLockState(8);
	  }
	break;
    }
  }
  return CallNextHookEx(s_hookDataArch->m_hHookCallWndProc, i_nCode,
			i_wParam, i_lParam);
}


static LRESULT CALLBACK lowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
  MSLLHOOKSTRUCT *pMsll = (MSLLHOOKSTRUCT*)lParam;
  LONG dx = pMsll->pt.x - g_hookData->m_mousePos.x;
  LONG dy = pMsll->pt.y - g_hookData->m_mousePos.y;
  HWND target = reinterpret_cast<HWND>(g_hookData->m_hwndMouseHookTarget);

  if (!g.m_isInitialized)
    initialize();

  if (!g_hookData || nCode < 0 || wParam != WM_MOUSEMOVE)
    goto through;

  switch (g_hookData->m_mouseHookType)
  {
    case MouseHookType_Wheel:
      // For this type, g_hookData->m_mouseHookParam means
      // translate rate mouse move to wheel.
      mouse_event(MOUSEEVENTF_WHEEL, 0, 0,
		  g_hookData->m_mouseHookParam * dy, 0);
      return 1;
      break;
    case MouseHookType_WindowMove:
    {
      RECT curRect;

      if (!GetWindowRect(target, &curRect))
	goto through;

      // g_hookData->m_mouseHookParam < 0 means
      // target window to move is MDI.
      if (g_hookData->m_mouseHookParam < 0)
      {
	HWND parent = GetParent(target);
	POINT p = {curRect.left, curRect.top};

	if (parent == NULL || !ScreenToClient(parent, &p))
	  goto through;

	curRect.left = p.x;
	curRect.top = p.y;
      }
	
      SetWindowPos(target, NULL,
		   curRect.left + dx,
		   curRect.top + dy,
		   0, 0,
		   SWP_ASYNCWINDOWPOS | SWP_NOACTIVATE |
		   SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
      g_hookData->m_mousePos = pMsll->pt;
      goto through;
      break;
    }
    case MouseHookType_None:
    default:
      goto through;
      break;
  }
    
 through:
  return CallNextHookEx(g.m_hHookMouseProc,
			nCode, wParam, lParam);
}


#ifdef NO_DRIVER
static LRESULT CALLBACK lowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
  KBDLLHOOKSTRUCT *pKbll = (KBDLLHOOKSTRUCT*)lParam;

  if (!g.m_isInitialized)
    initialize();

  if (!g_hookData || nCode < 0)
    goto through;

  if (g.m_keyboardDetour && g.m_engine)
  {
    unsigned int result;
    result = g.m_keyboardDetour(g.m_engine, pKbll);
    if (result)
    {
      return 1;
    }
  }
 through:
  return CallNextHookEx(g.m_hHookKeyboardProc,
			nCode, wParam, lParam);
}
#endif // NO_DRIVER


/// install hooks
DllExport int installHooks(KEYBOARD_DETOUR i_keyboardDetour, Engine *i_engine)
{
  if (!g.m_isInitialized)
    initialize();

  g.m_hwndTaskTray = g_hookData->m_hwndTaskTray;
  s_hookDataArch->m_hHookGetMessage =
    SetWindowsHookEx(WH_GETMESSAGE, (HOOKPROC)getMessageProc,
		     g.m_hInstDLL, 0);
  s_hookDataArch->m_hHookCallWndProc =
    SetWindowsHookEx(WH_CALLWNDPROC, (HOOKPROC)callWndProc, g.m_hInstDLL, 0);
  g_hookData->m_mouseHookType = MouseHookType_None;
  if (i_engine != NULL)
  {
#ifdef NO_DRIVER
    g.m_keyboardDetour = i_keyboardDetour;
    g.m_engine = i_engine;
    g.m_hHookKeyboardProc =
      SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)lowLevelKeyboardProc,
		       g.m_hInstDLL, 0);
#endif // NO_DRIVER
    g.m_hHookMouseProc =
      SetWindowsHookEx(WH_MOUSE_LL, (HOOKPROC)lowLevelMouseProc,
		       g.m_hInstDLL, 0);
  }
  return 0;
}


/// uninstall hooks
DllExport int uninstallHooks()
{
  if (s_hookDataArch->m_hHookGetMessage)
    UnhookWindowsHookEx(s_hookDataArch->m_hHookGetMessage);
  s_hookDataArch->m_hHookGetMessage = NULL;
  if (s_hookDataArch->m_hHookCallWndProc)
    UnhookWindowsHookEx(s_hookDataArch->m_hHookCallWndProc);
  s_hookDataArch->m_hHookCallWndProc = NULL;
  if (g.m_hHookMouseProc)
    UnhookWindowsHookEx(g.m_hHookMouseProc);
  g.m_hHookMouseProc = NULL;
#ifdef NO_DRIVER
  if (g.m_hHookKeyboardProc)
    UnhookWindowsHookEx(g.m_hHookKeyboardProc);
  g.m_hHookKeyboardProc = NULL;
#endif // NO_DRIVER
  g.m_hwndTaskTray = 0;
  return 0;
}
