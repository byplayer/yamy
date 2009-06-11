//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// mayu.cpp


#define APSTUDIO_INVOKED

#include "misc.h"
#include "compiler_specific_func.h"
#include "dlginvestigate.h"
#include "dlglog.h"
#include "dlgsetting.h"
#include "dlgversion.h"
#include "engine.h"
#include "errormessage.h"
#include "focus.h"
#include "function.h"
#include "hook.h"
#include "mayu.h"
#include "mayuipc.h"
#include "mayurc.h"
#include "msgstream.h"
#include "multithread.h"
#include "registry.h"
#include "setting.h"
#include "target.h"
#include "windowstool.h"
#include <process.h>
#include <time.h>
#include <commctrl.h>
#include <wtsapi32.h>


///
#define ID_MENUITEM_reloadBegin _APS_NEXT_COMMAND_VALUE


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Mayu


///
class Mayu
{
  HWND m_hwndTaskTray;				/// tasktray window
  HWND m_hwndLog;				/// log dialog
  HWND m_hwndInvestigate;			/// investigate dialog
  HWND m_hwndVersion;				/// version dialog
  
  UINT m_WM_TaskbarRestart;			/** window message sent when
                                                    taskber restarts */
  UINT m_WM_MayuIPC;				/** IPC message sent from
						    other applications */
  NOTIFYICONDATA m_ni;				/// taskbar icon data
  HICON m_tasktrayIcon[2];			/// taskbar icon
  bool m_canUseTasktrayBaloon;			/// 

  tomsgstream m_log;				/** log stream (output to log
						    dialog's edit) */

  HMENU m_hMenuTaskTray;			/// tasktray menu
  
  Setting *m_setting;				/// current setting
  bool m_isSettingDialogOpened;			/// is setting dialog opened ?
  
  Engine m_engine;				/// engine

  bool m_usingSN;		   /// using WTSRegisterSessionNotification() ?
  time_t m_startTime;				/// mayu started at ...

  enum
  { 
    WM_APP_taskTrayNotify = WM_APP + 101,	///
    WM_APP_msgStreamNotify = WM_APP + 102,	///
    ID_TaskTrayIcon = 1,			///
  };

private:
  /// register class for tasktray
  ATOM Register_tasktray()
  {
    WNDCLASS wc;
    wc.style         = 0;
    wc.lpfnWndProc   = tasktray_wndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = sizeof(Mayu *);
    wc.hInstance     = g_hInst;
    wc.hIcon         = NULL;
    wc.hCursor       = NULL;
    wc.hbrBackground = NULL;
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = _T("mayuTasktray");
    return RegisterClass(&wc);
  }

  /// notify handler
  BOOL notifyHandler(COPYDATASTRUCT *cd)
  {
      switch (cd->dwData)
      {
	case Notify::Type_setFocus:
	case Notify::Type_name:
	{
	  NotifySetFocus *n = (NotifySetFocus *)cd->lpData;
	  n->m_className[NUMBER_OF(n->m_className) - 1] = _T('\0');
	  n->m_titleName[NUMBER_OF(n->m_titleName) - 1] = _T('\0');
	  
	  if (n->m_type == Notify::Type_setFocus)
	    m_engine.setFocus(n->m_hwnd, n->m_threadId,
			      n->m_className, n->m_titleName, false);

	  {
	    Acquire a(&m_log, 1);
	    m_log << _T("HWND:\t") << std::hex
		  << reinterpret_cast<int>(n->m_hwnd)
		  << std::dec << std::endl;
	    m_log << _T("THREADID:") << static_cast<int>(n->m_threadId)
		  << std::endl;
	  }
	  Acquire a(&m_log, (n->m_type == Notify::Type_name) ? 0 : 1);
	  m_log << _T("CLASS:\t") << n->m_className << std::endl;
	  m_log << _T("TITLE:\t") << n->m_titleName << std::endl;
	  
	  bool isMDI = true;
	  HWND hwnd = getToplevelWindow(n->m_hwnd, &isMDI);
	  RECT rc;
	  if (isMDI)
	  {
	    getChildWindowRect(hwnd, &rc);
	    m_log << _T("MDI Window Position/Size: (")
		  << rc.left << _T(", ") << rc.top << _T(") / (")
		  << rcWidth(&rc) << _T("x") << rcHeight(&rc) << _T(")")
		  << std::endl;
	    hwnd = getToplevelWindow(n->m_hwnd, NULL);
	  }
	  
	  GetWindowRect(hwnd, &rc);
	  m_log << _T("Toplevel Window Position/Size: (")
		<< rc.left << _T(", ") << rc.top << _T(") / (")
		<< rcWidth(&rc) << _T("x") << rcHeight(&rc) << _T(")")
		<< std::endl;

	  SystemParametersInfo(SPI_GETWORKAREA, 0, (void *)&rc, FALSE);
	  m_log << _T("Desktop Window Position/Size: (")
		<< rc.left << _T(", ") << rc.top << _T(") / (")
		<< rcWidth(&rc) << _T("x") << rcHeight(&rc) << _T(")")
		<< std::endl;
	  
	  m_log << std::endl;
	  break;
	}
	
	case Notify::Type_lockState:
	{
	  NotifyLockState *n = (NotifyLockState *)cd->lpData;
	  m_engine.setLockState(n->m_isNumLockToggled,
				n->m_isCapsLockToggled,
				n->m_isScrollLockToggled,
				n->m_isKanaLockToggled,
				n->m_isImeLockToggled,
				n->m_isImeCompToggled);
#if 0
	  Acquire a(&m_log, 0);
	  if (n->m_isKanaLockToggled) {
	    m_log << _T("Notify::Type_lockState Kana on  : ");
	  } else {
	    m_log << _T("Notify::Type_lockState Kana off : ");
	  }
	  m_log << n->m_debugParam << ", "
		<< g_hookData->m_correctKanaLockHandling << std::endl;
#endif
	  break;
	}

	case Notify::Type_sync:
	{
	  m_engine.syncNotify();
	  break;
	}

	case Notify::Type_threadDetach:
	{
	  NotifyThreadDetach *n = (NotifyThreadDetach *)cd->lpData;
	  m_engine.threadDetachNotify(n->m_threadId);
	  break;
	}

	case Notify::Type_command:
	{
	  NotifyCommand *n = (NotifyCommand *)cd->lpData;
	  m_engine.commandNotify(n->m_hwnd, n->m_message,
				 n->m_wParam, n->m_lParam);
	  break;
	}

	case Notify::Type_show:
	{
	  NotifyShow *n = (NotifyShow *)cd->lpData;
	  switch (n->m_show)
	  {
	    case NotifyShow::Show_Maximized:
	      m_engine.setShow(true, false, n->m_isMDI);
	      break;
	    case NotifyShow::Show_Minimized:
	      m_engine.setShow(false, true, n->m_isMDI);
	      break;
	    case NotifyShow::Show_Normal:
	    default:
	      m_engine.setShow(false, false, n->m_isMDI);
	      break;
	  }	  
	  break;
	}

	case Notify::Type_log:
	{
	  Acquire a(&m_log, 1);
	  NotifyLog *n = (NotifyLog *)cd->lpData;
	  m_log << _T("hook log: ") << n->m_msg << std::endl;
	  break;
	}
      }
      return true;
  }

  /// window procedure for tasktray
  static LRESULT CALLBACK
  tasktray_wndProc(HWND i_hwnd, UINT i_message,
		   WPARAM i_wParam, LPARAM i_lParam)
  {
    Mayu *This = reinterpret_cast<Mayu *>(GetWindowLong(i_hwnd, 0));

    if (!This)
      switch (i_message)
      {
	case WM_CREATE:
	  This = reinterpret_cast<Mayu *>(
	    reinterpret_cast<CREATESTRUCT *>(i_lParam)->lpCreateParams);
	  SetWindowLong(i_hwnd, 0, (long)This);
	  return 0;
      }
    else
      switch (i_message)
      {
        case WM_COPYDATA:
	{
	  COPYDATASTRUCT *cd;
	  cd = reinterpret_cast<COPYDATASTRUCT *>(i_lParam);
	  return This->notifyHandler(cd);
	}
        case WM_QUERYENDSESSION:
	  This->m_engine.prepairQuit();
	  PostQuitMessage(0);
	  return TRUE;

#ifndef WM_WTSSESSION_CHANGE			// WinUser.h
#  define WM_WTSSESSION_CHANGE            0x02B1
#endif
	case WM_WTSSESSION_CHANGE:
	{
	  const char *m = "";
	  switch (i_wParam)
	  {
#ifndef WTS_CONSOLE_CONNECT			// WinUser.h
#  define WTS_CONSOLE_CONNECT                0x1
#  define WTS_CONSOLE_DISCONNECT             0x2
#  define WTS_REMOTE_CONNECT                 0x3
#  define WTS_REMOTE_DISCONNECT              0x4
#  define WTS_SESSION_LOGON                  0x5
#  define WTS_SESSION_LOGOFF                 0x6
#  define WTS_SESSION_LOCK                   0x7
#  define WTS_SESSION_UNLOCK                 0x8
#endif
	    case WTS_CONSOLE_CONNECT:
	      m = "WTS_CONSOLE_CONNECT";
	      if (!This->m_engine.resume()) {
		This->m_engine.prepairQuit();
		PostQuitMessage(0);
	      }
	      break;
	    case WTS_CONSOLE_DISCONNECT:
	      m = "WTS_CONSOLE_DISCONNECT";
	      This->m_engine.pause();
	      break;
	    case WTS_REMOTE_CONNECT: m = "WTS_REMOTE_CONNECT"; break;
	    case WTS_REMOTE_DISCONNECT: m = "WTS_REMOTE_DISCONNECT"; break;
	    case WTS_SESSION_LOGON:  m = "WTS_SESSION_LOGON"; break;
	    case WTS_SESSION_LOGOFF: m = "WTS_SESSION_LOGOFF"; break;
	    case WTS_SESSION_LOCK: m = "WTS_SESSION_LOCK"; break;
	    case WTS_SESSION_UNLOCK: m = "WTS_SESSION_UNLOCK"; break;
	      //case WTS_SESSION_REMOTE_CONTROL: m = "WTS_SESSION_REMOTE_CONTROL"; break;
	  }
	  This->m_log << _T("WM_WTSESSION_CHANGE(")
		      << i_wParam << ", " << i_lParam << "): "
		      << m << std::endl;
	  return TRUE;
	}
	case WM_APP_msgStreamNotify:
	{
	  tomsgstream::StreamBuf *log =
	    reinterpret_cast<tomsgstream::StreamBuf *>(i_lParam);
	  const tstring &str = log->acquireString();
	  editInsertTextAtLast(GetDlgItem(This->m_hwndLog, IDC_EDIT_log),
			       str, 65000);
	  log->releaseString();
	  return 0;
	}
	
	case WM_APP_taskTrayNotify:
	{
	  if (i_wParam == ID_TaskTrayIcon)
	    switch (i_lParam)
	    {
	      case WM_RBUTTONUP:
	      {
		POINT p;
		CHECK_TRUE( GetCursorPos(&p) );
		SetForegroundWindow(i_hwnd);
		HMENU hMenuSub = GetSubMenu(This->m_hMenuTaskTray, 0);
		if (This->m_engine.getIsEnabled())
		  CheckMenuItem(hMenuSub, ID_MENUITEM_disable,
				MF_UNCHECKED | MF_BYCOMMAND);
		else
		  CheckMenuItem(hMenuSub, ID_MENUITEM_disable,
				MF_CHECKED | MF_BYCOMMAND);
		CHECK_TRUE( SetMenuDefaultItem(hMenuSub,
					  ID_MENUITEM_investigate, FALSE) );

		// create reload menu
		HMENU hMenuSubSub = GetSubMenu(hMenuSub, 1);
		Registry reg(MAYU_REGISTRY_ROOT);
		int mayuIndex;
		reg.read(_T(".mayuIndex"), &mayuIndex, 0);
		while (DeleteMenu(hMenuSubSub, 0, MF_BYPOSITION))
		  ;
		tregex getName(_T("^([^;]*);"));
		for (int index = 0; ; index ++)
		{
		  _TCHAR buf[100];
		  _sntprintf(buf, NUMBER_OF(buf), _T(".mayu%d"), index);
		  tstringi dot_mayu;
		  if (!reg.read(buf, &dot_mayu))
		    break;
		  tsmatch what;
		  if (boost::regex_search(dot_mayu, what, getName))
		  {
		    MENUITEMINFO mii;
		    std::memset(&mii, 0, sizeof(mii));
		    mii.cbSize = sizeof(mii);
		    mii.fMask = MIIM_ID | MIIM_STATE | MIIM_TYPE;
		    mii.fType = MFT_STRING;
		    mii.fState =
		      MFS_ENABLED | ((mayuIndex == index) ? MFS_CHECKED : 0);
		    mii.wID = ID_MENUITEM_reloadBegin + index;
		    tstringi name(what.str(1));
		    mii.dwTypeData = const_cast<_TCHAR *>(name.c_str());
		    mii.cch = name.size();
		    
		    InsertMenuItem(hMenuSubSub, index, TRUE, &mii);
		  }
		}

		// show popup menu
		TrackPopupMenu(hMenuSub, TPM_LEFTALIGN,
			       p.x, p.y, 0, i_hwnd, NULL);
		// TrackPopupMenu may fail (ERROR_POPUP_ALREADY_ACTIVE)
		break;
	      }
	      
	      case WM_LBUTTONDBLCLK:
		SendMessage(i_hwnd, WM_COMMAND,
			    MAKELONG(ID_MENUITEM_investigate, 0), 0);
		break;
	    }
	  return 0;
	}
      
	case WM_COMMAND:
	{
	  int notify_code = HIWORD(i_wParam);
	  int id = LOWORD(i_wParam);
	  if (notify_code == 0) // menu
	    switch (id)
	    {
	      default:
		if (ID_MENUITEM_reloadBegin <= id)
		{
		  Registry reg(MAYU_REGISTRY_ROOT);
		  reg.write(_T(".mayuIndex"), id - ID_MENUITEM_reloadBegin);
		  This->load();
		}
		break;
	      case ID_MENUITEM_reload:
		This->load();
		break;
	      case ID_MENUITEM_investigate:
	      {
		ShowWindow(This->m_hwndLog, SW_SHOW);
		ShowWindow(This->m_hwndInvestigate, SW_SHOW);
		
		RECT rc1, rc2;
		GetWindowRect(This->m_hwndInvestigate, &rc1);
		GetWindowRect(This->m_hwndLog, &rc2);

		MoveWindow(This->m_hwndLog, rc1.left, rc1.bottom,
			   rcWidth(&rc1), rcHeight(&rc2), TRUE);
		
		SetForegroundWindow(This->m_hwndLog);
		SetForegroundWindow(This->m_hwndInvestigate);
		break;
	      }
	      case ID_MENUITEM_setting:
		if (!This->m_isSettingDialogOpened)
		{
		  This->m_isSettingDialogOpened = true;
		  if (DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_setting),
				NULL, dlgSetting_dlgProc))
		    This->load();
		  This->m_isSettingDialogOpened = false;
		}
		break;
	      case ID_MENUITEM_log:
		ShowWindow(This->m_hwndLog, SW_SHOW);
		SetForegroundWindow(This->m_hwndLog);
		break;
	      case ID_MENUITEM_version:
		ShowWindow(This->m_hwndVersion, SW_SHOW);
		SetForegroundWindow(This->m_hwndVersion);
		break;
	      case ID_MENUITEM_help:
	      {
		_TCHAR buf[GANA_MAX_PATH];
		CHECK_TRUE( GetModuleFileName(g_hInst, buf, NUMBER_OF(buf)) );
		tstringi helpFilename = pathRemoveFileSpec(buf);
		helpFilename += _T("\\");
		helpFilename += loadString(IDS_helpFilename);
		ShellExecute(NULL, _T("open"), helpFilename.c_str(),
			     NULL, NULL, SW_SHOWNORMAL);
		break;
	      }
	      case ID_MENUITEM_disable:
		This->m_engine.enable(!This->m_engine.getIsEnabled());
		This->showTasktrayIcon();
		break;
	      case ID_MENUITEM_quit:
		This->m_engine.prepairQuit();
		PostQuitMessage(0);
		break;
	    }
	  return 0;
	}

	case WM_APP_engineNotify:
	{
	  switch (i_wParam)
	  {
	    case EngineNotify_shellExecute:
	      This->m_engine.shellExecute();
	      break;
	    case EngineNotify_loadSetting:
	      This->load();
	      break;
	    case EngineNotify_helpMessage:
	      This->showHelpMessage(false);
	      if (i_lParam)
		This->showHelpMessage(true);
	      break;
	    case EngineNotify_showDlg:
	    {
	      // show investigate/log window
	      int sw = (i_lParam & ~MayuDialogType_mask);
	      HWND hwnd = NULL;
	      switch (static_cast<MayuDialogType>(
		i_lParam & MayuDialogType_mask))
	      {
		case MayuDialogType_investigate:
		  hwnd = This->m_hwndInvestigate;
		  break;
		case MayuDialogType_log:
		  hwnd = This->m_hwndLog;
		  break;
	      }
	      if (hwnd)
	      {
		ShowWindow(hwnd, sw);
		switch (sw)
		{
		  case SW_SHOWNORMAL:
		  case SW_SHOWMAXIMIZED:
		  case SW_SHOW:
		  case SW_RESTORE:
		  case SW_SHOWDEFAULT:
		    SetForegroundWindow(hwnd);
		    break;
		}
	      }
	      break;
	    }
	    case EngineNotify_setForegroundWindow:
	      // FIXME: completely useless. why ?
	      setForegroundWindow(reinterpret_cast<HWND>(i_lParam));
	      {
		Acquire a(&This->m_log, 1);
		This->m_log << _T("setForegroundWindow(0x")
			    << std::hex << i_lParam << std::dec << _T(")")
			    << std::endl;
	      }
	      break;
	    case EngineNotify_clearLog:
	      SendMessage(This->m_hwndLog, WM_COMMAND,
			  MAKELONG(IDC_BUTTON_clearLog, 0), 0);
	      break;
	    default:
	      break;
	  }
	  return 0;
	}
	
	case WM_APP_dlglogNotify:
	{
	  switch (i_wParam)
	  {
	    case DlgLogNotify_logCleared:
	      This->showBanner(true);
	      break;
	    default:
	      break;
	  }
	  return 0;
	}

	case WM_DESTROY:
	  if (This->m_usingSN)
	  {
	    wtsUnRegisterSessionNotification(i_hwnd);
	    This->m_usingSN = false;
	  }
	  return 0;
	
	default:
	  if (i_message == This->m_WM_TaskbarRestart)
	  {
	    if (This->showTasktrayIcon(true))
	    {
	      Acquire a(&This->m_log, 0);
	      This->m_log << _T("Tasktray icon is updated.") << std::endl;
	    }
	    else
	    {
	      Acquire a(&This->m_log, 1);
	      This->m_log << _T("Tasktray icon already exists.") << std::endl;
	    }
	    return 0;
	  }
	  else if (i_message == This->m_WM_MayuIPC)
	  {
	    switch (static_cast<MayuIPCCommand>(i_wParam))
	    {
	      case MayuIPCCommand_Enable:
		This->m_engine.enable(!!i_lParam);
		This->showTasktrayIcon();
		if (i_lParam)
		{
		  Acquire a(&This->m_log, 1);
		  This->m_log << _T("Enabled by another application.")
			      << std::endl;
		}
		else
		{
		  Acquire a(&This->m_log, 1);
		  This->m_log << _T("Disabled by another application.")
			      << std::endl;
		}
		break;
	    }
	  }
      }
    return DefWindowProc(i_hwnd, i_message, i_wParam, i_lParam);
  }

  /// load setting
  void load()
  {
    Setting *newSetting = new Setting;

    // set symbol
    for (int i = 1; i < __argc; ++ i)
    {
      if (__targv[i][0] == _T('-') && __targv[i][1] == _T('D'))
	newSetting->m_symbols.insert(__targv[i] + 2);
    }

    if (!SettingLoader(&m_log, &m_log).load(newSetting))
    {
      ShowWindow(m_hwndLog, SW_SHOW);
      SetForegroundWindow(m_hwndLog);
      delete newSetting;
      Acquire a(&m_log, 0);
      m_log << _T("error: failed to load.") << std::endl;
      return;
    }
    m_log << _T("successfully loaded.") << std::endl;
    while (!m_engine.setSetting(newSetting))
      Sleep(1000);
    delete m_setting;
    m_setting = newSetting;
  }

  // show message (a baloon from the task tray icon)
  void showHelpMessage(bool i_doesShow = true)
  {
    if (m_canUseTasktrayBaloon)
    {
      if (i_doesShow)
      {
	tstring helpMessage, helpTitle;
	m_engine.getHelpMessages(&helpMessage, &helpTitle);
	tcslcpy(m_ni.szInfo, helpMessage.c_str(), NUMBER_OF(m_ni.szInfo));
	tcslcpy(m_ni.szInfoTitle, helpTitle.c_str(),
		NUMBER_OF(m_ni.szInfoTitle));
	m_ni.dwInfoFlags = NIIF_INFO;
      }
      else
	m_ni.szInfo[0] = m_ni.szInfoTitle[0] = _T('\0');
      CHECK_TRUE( Shell_NotifyIcon(NIM_MODIFY, &m_ni) );
    }
  }
  
  // change the task tray icon
  bool showTasktrayIcon(bool i_doesAdd = false)
  {
    m_ni.hIcon  = m_tasktrayIcon[m_engine.getIsEnabled() ? 1 : 0];
    m_ni.szInfo[0] = m_ni.szInfoTitle[0] = _T('\0');
    if (i_doesAdd) {
      // http://support.microsoft.com/kb/418138/JA/
      int guard = 60;
      for (; !Shell_NotifyIcon(NIM_ADD, &m_ni) && 0 < guard; -- guard) {
	if (Shell_NotifyIcon(NIM_MODIFY, &m_ni)) {
	  return true;
	}
	Sleep(1000);				// 1sec
      }
      return 0 < guard;
    } else {
      return !!Shell_NotifyIcon(NIM_MODIFY, &m_ni);
    }
  }

  void showBanner(bool i_isCleared)
  {
    time_t now;
    time(&now);
      
    _TCHAR starttimebuf[1024];
    _TCHAR timebuf[1024];

#ifdef __BORLANDC__
#pragma message("\t\t****\tAfter std::ostream() is called,  ")
#pragma message("\t\t****\tstrftime(... \"%%#c\" ...) fails.")
#pragma message("\t\t****\tWhy ? Bug of Borland C++ 5.5.1 ? ")
#pragma message("\t\t****\t                         - nayuta")
    _tcsftime(timebuf, NUMBER_OF(timebuf), _T("%Y/%m/%d %H:%M:%S"),
	      localtime(&now));
    _tcsftime(starttimebuf, NUMBER_OF(starttimebuf), _T("%Y/%m/%d %H:%M:%S"),
	      localtime(&m_startTime));
#else
    _tcsftime(timebuf, NUMBER_OF(timebuf), _T("%#c"), localtime(&now));
    _tcsftime(starttimebuf, NUMBER_OF(starttimebuf), _T("%#c"),
	      localtime(&m_startTime));
#endif
      
    Acquire a(&m_log, 0);
    m_log << _T("------------------------------------------------------------") << std::endl;
    m_log << loadString(IDS_mayu) << _T(" ") _T(VERSION);
#ifndef NDEBUG
    m_log << _T(" (DEBUG)");
#endif
#ifdef _UNICODE
    m_log << _T(" (UNICODE)");
#endif
    m_log << std::endl;
    m_log << _T("  built by ")
	  << _T(LOGNAME) << _T("@") << toLower(_T(COMPUTERNAME))
	  << _T(" (") << _T(__DATE__) <<  _T(" ")
	  << _T(__TIME__) << _T(", ")
	  << getCompilerVersionString() << _T(")") << std::endl;
    _TCHAR modulebuf[1024];
    CHECK_TRUE( GetModuleFileName(g_hInst, modulebuf,
				  NUMBER_OF(modulebuf)) );
    m_log << _T("started at ") << starttimebuf << std::endl;
    m_log << modulebuf << std::endl;
    m_log << _T("------------------------------------------------------------") << std::endl;

    if (i_isCleared) {
      m_log << _T("log was cleared at ") << timebuf << std::endl;
    } else {
      m_log << _T("log begins at ") << timebuf << std::endl;
    }
  }

public:
  ///
  Mayu()
    : m_hwndTaskTray(NULL),
      m_hwndLog(NULL),
      m_WM_TaskbarRestart(RegisterWindowMessage(_T("TaskbarCreated"))),
      m_WM_MayuIPC(RegisterWindowMessage(WM_MayuIPC_NAME)),
      m_canUseTasktrayBaloon(
	PACKVERSION(5, 0) <= getDllVersion(_T("shlwapi.dll"))),
      m_log(WM_APP_msgStreamNotify),
      m_setting(NULL),
      m_isSettingDialogOpened(false),
      m_engine(m_log)
  {
    time(&m_startTime);

    CHECK_TRUE( Register_focus() );
    CHECK_TRUE( Register_target() );
    CHECK_TRUE( Register_tasktray() );

    // change dir
#if 0
    HomeDirectories pathes;
    getHomeDirectories(&pathes);
    for (HomeDirectories::iterator i = pathes.begin(); i != pathes.end(); ++ i)
      if (SetCurrentDirectory(i->c_str()))
	break;
#endif
    
    // create windows, dialogs
    tstringi title = loadString(IDS_mayu);
    m_hwndTaskTray = CreateWindow(_T("mayuTasktray"), title.c_str(),
				  WS_OVERLAPPEDWINDOW,
				  CW_USEDEFAULT, CW_USEDEFAULT, 
				  CW_USEDEFAULT, CW_USEDEFAULT, 
				  NULL, NULL, g_hInst, this);
    CHECK_TRUE( m_hwndTaskTray );
    
    // set window handle of tasktray to hooks
    g_hookData->m_hwndTaskTray = m_hwndTaskTray;
    m_usingSN = wtsRegisterSessionNotification(m_hwndTaskTray,
					       NOTIFY_FOR_THIS_SESSION);

    DlgLogData dld;
    dld.m_log = &m_log;
    dld.m_hwndTaskTray = m_hwndTaskTray;
    m_hwndLog =
      CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_log), NULL,
			dlgLog_dlgProc, (LPARAM)&dld);
    CHECK_TRUE( m_hwndLog );

    DlgInvestigateData did;
    did.m_engine = &m_engine;
    did.m_hwndLog = m_hwndLog;
    m_hwndInvestigate =
      CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_investigate), NULL,
			dlgInvestigate_dlgProc, (LPARAM)&did);
    CHECK_TRUE( m_hwndInvestigate );

    m_hwndVersion =
      CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_version),
			NULL, dlgVersion_dlgProc,
			(LPARAM)m_engine.getMayudVersion().c_str());
    CHECK_TRUE( m_hwndVersion );

    // attach log
    SendMessage(GetDlgItem(m_hwndLog, IDC_EDIT_log), EM_SETLIMITTEXT, 0, 0);
    m_log.attach(m_hwndTaskTray);

    // start keyboard handler thread
    m_engine.setAssociatedWndow(m_hwndTaskTray);
    m_engine.start();
    
    // show tasktray icon
    m_tasktrayIcon[0] = loadSmallIcon(IDI_ICON_mayu_disabled);
    m_tasktrayIcon[1] = loadSmallIcon(IDI_ICON_mayu);
    std::memset(&m_ni, 0, sizeof(m_ni));
    m_ni.uID    = ID_TaskTrayIcon;
    m_ni.hWnd   = m_hwndTaskTray;
    m_ni.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    m_ni.hIcon  = m_tasktrayIcon[1];
    m_ni.uCallbackMessage = WM_APP_taskTrayNotify;
    tstring tip = loadString(IDS_mayu);
    tcslcpy(m_ni.szTip, tip.c_str(), NUMBER_OF(m_ni.szTip));
    if (m_canUseTasktrayBaloon)
    {
      m_ni.cbSize = sizeof(m_ni);
      m_ni.uFlags |= NIF_INFO;
    }
    else
      m_ni.cbSize = NOTIFYICONDATA_V1_SIZE;
    showTasktrayIcon(true);

    // create menu
    m_hMenuTaskTray = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_MENU_tasktray));
    ASSERT(m_hMenuTaskTray);
    
    // set initial lock state
    notifyLockState();
  }

  ///
  ~Mayu()
  {
    // first, detach log from edit control to avoid deadlock
    m_log.detach();
   
    // stop notify from mayu.dll
    g_hookData->m_hwndTaskTray = NULL;
    
    // destroy windows
    CHECK_TRUE( DestroyWindow(m_hwndVersion) );
    CHECK_TRUE( DestroyWindow(m_hwndInvestigate) );
    CHECK_TRUE( DestroyWindow(m_hwndLog) );
    CHECK_TRUE( DestroyWindow(m_hwndTaskTray) );
    
    // destroy menu
    DestroyMenu(m_hMenuTaskTray);
    
    // delete tasktray icon
    CHECK_TRUE( Shell_NotifyIcon(NIM_DELETE, &m_ni) );
    CHECK_TRUE( DestroyIcon(m_tasktrayIcon[1]) );
    CHECK_TRUE( DestroyIcon(m_tasktrayIcon[0]) );

    // stop keyboard handler thread
    m_engine.stop();
    
    // remove setting;
    delete m_setting;
  }

  /// message loop
  WPARAM messageLoop()
  {
    showBanner(false);
    load();
    
    MSG msg;
    while (0 < GetMessage(&msg, NULL, 0, 0))
    {
      if (IsDialogMessage(m_hwndLog, &msg))
	continue;
      if (IsDialogMessage(m_hwndInvestigate, &msg))
	continue;
      if (IsDialogMessage(m_hwndVersion, &msg))
	continue;
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
    return msg.wParam;
  }  
};


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Functions


/// convert registry
void convertRegistry()
{
  Registry reg(MAYU_REGISTRY_ROOT);
  tstringi dot_mayu;
  bool doesAdd = false;
  DWORD index;
  if (reg.read(_T(".mayu"), &dot_mayu))
  {
    reg.write(_T(".mayu0"), _T(";") + dot_mayu + _T(";"));
    reg.remove(_T(".mayu"));
    doesAdd = true;
    index = 0;
  }
  else if (!reg.read(_T(".mayu0"), &dot_mayu))
  {
    reg.write(_T(".mayu0"), loadString(IDS_readFromHomeDirectory) + _T(";;"));
    doesAdd = true;
    index = 1;
  }
  if (doesAdd)
  {
    Registry commonreg(HKEY_LOCAL_MACHINE, _T("Software\\GANAware\\mayu"));
    tstringi dir, layout;
    if (commonreg.read(_T("dir"), &dir) &&
	commonreg.read(_T("layout"), &layout))
    {
      tstringi tmp = _T(";") + dir + _T("\\dot.mayu");
      if (layout == _T("109"))
      {
	reg.write(_T(".mayu1"), loadString(IDS_109Emacs) + tmp
		  + _T(";-DUSE109") _T(";-DUSEdefault"));
	reg.write(_T(".mayu2"), loadString(IDS_104on109Emacs) + tmp
		  + _T(";-DUSE109") _T(";-DUSEdefault") _T(";-DUSE104on109"));
	reg.write(_T(".mayu3"), loadString(IDS_109) + tmp
		  + _T(";-DUSE109"));
	reg.write(_T(".mayu4"), loadString(IDS_104on109) + tmp
		  + _T(";-DUSE109") _T(";-DUSE104on109"));
      }
      else
      {
	reg.write(_T(".mayu1"), loadString(IDS_104Emacs) + tmp
		  + _T(";-DUSE104") _T(";-DUSEdefault"));
	reg.write(_T(".mayu2"), loadString(IDS_109on104Emacs) + tmp
		  + _T(";-DUSE104") _T(";-DUSEdefault") _T(";-DUSE109on104"));
	reg.write(_T(".mayu3"), loadString(IDS_104) + tmp
		  + _T(";-DUSE104"));
	reg.write(_T(".mayu4"), loadString(IDS_109on104) + tmp
		  + _T(";-DUSE104") _T(";-DUSE109on104"));
      }
      reg.write(_T(".mayuIndex"), index);
    }
  }
}


/// main
int WINAPI _tWinMain(HINSTANCE i_hInstance, HINSTANCE /* i_hPrevInstance */,
		     LPTSTR /* i_lpszCmdLine */, int /* i_nCmdShow */)
{
  g_hInst = i_hInstance;

  // set locale
  CHECK_TRUE( _tsetlocale(LC_ALL, _T("")) );

  // common controls
#if defined(_WIN95)
  InitCommonControls();
#else
  INITCOMMONCONTROLSEX icc;
  icc.dwSize = sizeof(icc);
  icc.dwICC = ICC_LISTVIEW_CLASSES;
  CHECK_TRUE( InitCommonControlsEx(&icc) );
#endif

  // convert old registry to new registry
  convertRegistry();
  
  // is another mayu running ?
  HANDLE mutex = CreateMutex(
      (SECURITY_ATTRIBUTES *)NULL, TRUE,
      addSessionId(MUTEX_MAYU_EXCLUSIVE_RUNNING).c_str());
  if (GetLastError() == ERROR_ALREADY_EXISTS)
  {
    // another mayu already running
    tstring text = loadString(IDS_mayuAlreadyExists);
    tstring title = loadString(IDS_mayu);
    if (g_hookData) {
	UINT WM_TaskbarRestart = RegisterWindowMessage(_T("TaskbarCreated"));
	PostMessage(g_hookData->m_hwndTaskTray, WM_TaskbarRestart, 0, 0);
    }
    MessageBox((HWND)NULL, text.c_str(), title.c_str(), MB_OK | MB_ICONSTOP);
    return 1;
  }

  // check remote desktop
  DWORD sessionId;
  if (!ProcessIdToSessionId(GetCurrentProcessId(), &sessionId) ||
      wtsGetActiveConsoleSessionId() != sessionId)
  {
    tstring text = loadString(IDS_executedInRemoteDesktop);
    tstring title = loadString(IDS_mayu);
    MessageBox((HWND)NULL, text.c_str(), title.c_str(), MB_OK | MB_ICONSTOP);
    return 1;
  }
  
  CHECK_FALSE( installHooks() );
  try
  {
    Mayu().messageLoop();
  }
  catch (ErrorMessage &i_e)
  {
    tstring title = loadString(IDS_mayu);
    MessageBox((HWND)NULL, i_e.getMessage().c_str(), title.c_str(),
	       MB_OK | MB_ICONSTOP);
  }
  CHECK_FALSE( uninstallHooks() );
  
  CHECK_TRUE( CloseHandle(mutex) );
  return 0;
}
