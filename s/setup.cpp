///////////////////////////////////////////////////////////////////////////////
// setup.cpp


#include "../misc.h"
#include "../registry.h"
#include "../stringtool.h"
#include "../windowstool.h"
#include "../mayu.h"
#include "setuprc.h"
#include "installer.h"

#include <windowsx.h>
#include <shlobj.h>


using namespace Installer;


///////////////////////////////////////////////////////////////////////////////
// Registry


#define DIR_REGISTRY_ROOT			\
	HKEY_LOCAL_MACHINE,			\
	_T("Software\\GANAware\\mayu")


///////////////////////////////////////////////////////////////////////////////
// Globals


enum
{
  Flag_Usb = 1 << 1,
};
u_int32 g_flags = SetupFile::Normal;


using namespace SetupFile;
const SetupFile::Data g_setupFiles[] =
{
  // same name
#define SN(i_kind, i_os, i_from, i_destination)			\
  { i_kind, i_os, Normal|Flag_Usb, _T(i_from), i_destination, _T(i_from) }
  // different name
#define DN(i_kind, i_os, i_from, i_destination, i_to)	\
  { i_kind, i_os, Normal|Flag_Usb, _T(i_from), i_destination, _T(i_to) }
  
  // executables
  SN(Dll , ALL, "mayu.dll"	     , ToDest),
  SN(File, ALL, "mayu.exe"	     , ToDest),
  SN(File, ALL, "setup.exe"	     , ToDest),
				    
  // drivers
#if defined(_WINNT)
  SN(File, NT , "mayud.sys"	     , ToDest),
  SN(File, NT , "mayudnt4.sys"	     , ToDest),
  SN(File, W2k, "mayudrsc.sys"    , ToDest),
  SN(File, W2k, "mayud.sys"	     , ToDriver),
  DN(File, NT4, "mayudnt4.sys"	     , ToDriver, "mayud.sys"),
  SN(File, W2k, "mayudrsc.sys"    , ToDriver),
#elif defined(_WIN95)
  SN(File, W9x, "mayud.vxd"	     , ToDest),
#else
#  error
#endif

  // setting files		    
  SN(File, ALL, "104.mayu"	     , ToDest),
  SN(File, ALL, "104on109.mayu"	     , ToDest),
  SN(File, ALL, "109.mayu"	     , ToDest),
  SN(File, ALL, "109on104.mayu"	     , ToDest),
  SN(File, ALL, "default.mayu"	     , ToDest),
  SN(File, ALL, "dot.mayu"	     , ToDest),
  SN(File, ALL, "emacsedit.mayu"     , ToDest),
				    
  // documents				    
  SN(Dir , ALL, "doc"	 	     , ToDest), // mkdir
  DN(File, ALL, "banner-ja.gif"	     , ToDest, "doc\\banner-ja.gif"	 ),
  DN(File, ALL, "edit-setting-ja.png", ToDest, "doc\\edit-setting-ja.png"),
  DN(File, ALL, "investigate-ja.png" , ToDest, "doc\\investigate-ja.png" ),
  DN(File, ALL, "log-ja.png"	     , ToDest, "doc\\log-ja.png"	 ),
  DN(File, ALL, "menu-ja.png"	     , ToDest, "doc\\menu-ja.png"	 ),
  DN(File, ALL, "pause-ja.png"	     , ToDest, "doc\\pause-ja.png"	 ),
  DN(File, ALL, "setting-ja.png"     , ToDest, "doc\\setting-ja.png"	 ),
  DN(File, ALL, "target.png"	     , ToDest, "doc\\target.png"	 ),
  DN(File, ALL, "version-ja.png"     , ToDest, "doc\\version-ja.png"	 ),
  DN(File, ALL, "CONTENTS-ja.html"   , ToDest, "doc\\CONTENTS-ja.html"	 ),
  DN(File, ALL, "CUSTOMIZE-ja.html"  , ToDest, "doc\\CUSTOMIZE-ja.html"	 ),
  DN(File, ALL, "MANUAL-ja.html"     , ToDest, "doc\\MANUAL-ja.html"	 ),
  DN(File, ALL, "README-ja.html"     , ToDest, "doc\\README-ja.html"	 ),
  DN(File, ALL, "README.css"	     , ToDest, "doc\\README.css"	 ),
  DN(File, ALL, "syntax.txt"	     , ToDest, "doc\\syntax.txt"	 ),
  
  SN(File, ALL, "mayu-mode.el"	     , ToDest),
				    
  SN(Dir , ALL, "contrib"	     , ToDest), // mkdir
  DN(File, ALL, "mayu-settings.txt"  , ToDest, "contrib\\mayu-settings.txt"),
  DN(File, ALL, "dvorak.mayu"	     , ToDest, "contrib\\dvorak.mayu"      ),
  DN(File, ALL, "DVORAKon109.mayu"   , ToDest, "contrib\\DVORAKon109.mayu" ),
  DN(File, ALL, "keitai.mayu"	     , ToDest, "contrib\\keitai.mayu"      ),
  DN(File, ALL, "ax.mayu"	     , ToDest, "contrib\\ax.mayu"          ),
  DN(File, ALL, "98x1.mayu"	     , ToDest, "contrib\\98x1.mayu"        ),
  DN(File, ALL, "109onAX.mayu"	     , ToDest, "contrib\\109onAX.mayu"     ),

  SN(Dir , ALL, "Plugins"	     , ToDest), // mkdir
};


enum KeyboardKind
{
  KEYBOARD_KIND_109,
  KEYBOARD_KIND_104,
} g_keyboardKind;


static const StringResource g_strres[] =
{
#include "strres.h"
};


bool g_wasExecutedBySFX = false;	// Was setup executed by cab32 SFX ?
Resource *g_resource;			// resource information
tstringi g_destDir;			// destination directory


///////////////////////////////////////////////////////////////////////////////
// functions


// show message
int message(int i_id, int i_flag, HWND i_hwnd = NULL)
{
  return MessageBox(i_hwnd, g_resource->loadString(i_id),
		    g_resource->loadString(IDS_mayuSetup), i_flag);
}


// driver service error
void driverServiceError(DWORD i_err)
{
  switch (i_err)
  {
    case ERROR_ACCESS_DENIED:
      message(IDS_notAdministrator, MB_OK | MB_ICONSTOP);
      break;
    case ERROR_SERVICE_MARKED_FOR_DELETE:
      message(IDS_alreadyUninstalled, MB_OK | MB_ICONSTOP);
      break;
    default:
    {
      TCHAR *errmsg;
      int err = int(i_err);
      if (err < 0) {
	i_err = -err;
      }
      if (FormatMessage(
	    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
	    NULL, i_err, 0, (LPTSTR)&errmsg, 0, NULL)) {
	TCHAR buf[1024];
	_sntprintf(buf, NUMBER_OF(buf), _T("%s: %d: %s\n"),
		   g_resource->loadString(IDS_error),
		   err, errmsg);
	LocalFree(errmsg);
	MessageBox(NULL, buf, g_resource->loadString(IDS_mayuSetup),
		   MB_OK | MB_ICONSTOP);
      } else {
	message(IDS_error, MB_OK | MB_ICONSTOP);
      }
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// dialogue


// dialog box
class DlgMain
{
  HWND m_hwnd;
  bool m_doRegisterToStartMenu;	// if register to the start menu
  bool m_doRegisterToStartUp;	// if register to the start up

private:
  // install
  int install()
  {
    Registry reg(DIR_REGISTRY_ROOT);
    CHECK_TRUE( reg.write(_T("dir"), g_destDir) );
    tstringi srcDir = getModuleDirectory();

    if (!installFiles(g_setupFiles, NUMBER_OF(g_setupFiles), g_flags, srcDir,
		      g_destDir))
    {
      removeFiles(g_setupFiles, NUMBER_OF(g_setupFiles), g_flags, g_destDir);
      if (g_wasExecutedBySFX)
	removeSrcFiles(g_setupFiles, NUMBER_OF(g_setupFiles), g_flags, srcDir);
      return 1;
    }
    if (g_wasExecutedBySFX)
      removeSrcFiles(g_setupFiles, NUMBER_OF(g_setupFiles), g_flags, srcDir);

#if defined(_WINNT)
    DWORD err =
      createDriverService(_T("mayud"),
			  g_resource->loadString(IDS_mayud),
			  getDriverDirectory() + _T("\\mayud.sys"),
			  _T("+Keyboard Class\0"),
			  g_flags & Flag_Usb ? true : false);

    if (err != ERROR_SUCCESS)
    {
      driverServiceError(err);
      removeFiles(g_setupFiles, NUMBER_OF(g_setupFiles), g_flags, g_destDir);
      return 1;
    }

    if (g_flags == Flag_Usb)
      CHECK_TRUE( reg.write(_T("isUsbDriver"), DWORD(1)) );
#endif // _WINNT
    
    // create shortcut
    if (m_doRegisterToStartMenu)
    {
      tstringi shortcut = getStartMenuName(loadString(IDS_shortcutName));
      if (!shortcut.empty())
	createLink((g_destDir + _T("\\mayu.exe")).c_str(), shortcut.c_str(),
		   g_resource->loadString(IDS_shortcutName),
		   g_destDir.c_str());
    }
    if (m_doRegisterToStartUp)
    {
      tstringi shortcut = getStartUpName(loadString(IDS_shortcutName));
      if (!shortcut.empty())
	createLink((g_destDir + _T("\\mayu.exe")).c_str(), shortcut.c_str(),
		   g_resource->loadString(IDS_shortcutName),
		   g_destDir.c_str());
    }

    // set registry
    reg.write(_T("layout"),
	      (g_keyboardKind == KEYBOARD_KIND_109) ? _T("109") : _T("104"));

    // file extension
    createFileExtension(_T(".mayu"), _T("text/plain"),
			_T("mayu file"), g_resource->loadString(IDS_mayuFile),
			g_destDir + _T("\\mayu.exe,1"),
			g_resource->loadString(IDS_mayuShellOpen));
    
    // uninstall
    createUninstallInformation(_T("mayu"), g_resource->loadString(IDS_mayu),
			       g_destDir + _T("\\setup.exe -u"));

    if (g_flags == Flag_Usb)
    {
      if (message(IDS_copyFinishUsb, MB_YESNO | MB_ICONQUESTION, m_hwnd)
	  == IDYES)
      {
	// reboot ...
	HANDLE hToken; 
	// Get a token for this process. 
	if (!OpenProcessToken(GetCurrentProcess(), 
			      TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	{
	  message(IDS_failedToReboot, MB_OK | MB_ICONSTOP);
	  return 0;
	}
	// Get the LUID for the shutdown privilege.
	TOKEN_PRIVILEGES tkp;
	LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);
	tkp.PrivilegeCount = 1;  // one privilege to set
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	// Get the shutdown privilege for this process. 
	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0,
			      (PTOKEN_PRIVILEGES)NULL, 0);
	// Cannot test the return value of AdjustTokenPrivileges. 
	if (GetLastError() != ERROR_SUCCESS)
	{
	  message(IDS_failedToReboot, MB_OK | MB_ICONSTOP);
	  return 0;
	}
	// Shut down the system and force all applications to close. 
	if (!ExitWindowsEx(EWX_REBOOT | EWX_FORCE, 0))
	{
	  message(IDS_failedToReboot, MB_OK | MB_ICONSTOP);
	  return 0;
	}
      }
    }
    else
    {
      if (message(IDS_copyFinish, MB_YESNO | MB_ICONQUESTION, m_hwnd)
	  == IDYES)
	ExitWindows(0, 0);			// logoff
    }
    return 0;
  }
  
private:
  // WM_INITDIALOG
  BOOL wmInitDialog(HWND /* focus */, LPARAM /* lParam */)
  {
    setSmallIcon(m_hwnd, IDI_ICON_mayu);
    setBigIcon(m_hwnd, IDI_ICON_mayu);
    Edit_SetText(GetDlgItem(m_hwnd, IDC_EDIT_path), g_destDir.c_str());
    HWND hwndCombo = GetDlgItem(m_hwnd, IDC_COMBO_keyboard);
#if 0
    if (checkOs(SetupFile::W2k))
#endif
    {
      ComboBox_AddString(hwndCombo,
			 g_resource->loadString(IDS_keyboard109usb));
      ComboBox_AddString(hwndCombo,
			 g_resource->loadString(IDS_keyboard104usb));
    }
#if 0
    ComboBox_AddString(hwndCombo, g_resource->loadString(IDS_keyboard109));
    ComboBox_AddString(hwndCombo, g_resource->loadString(IDS_keyboard104));
#endif
    ComboBox_SetCurSel(hwndCombo,
		       (g_keyboardKind == KEYBOARD_KIND_109) ? 0 : 1);
    tstring note;
    for (int i = IDS_note01; i <= IDS_note13; ++ i) {
      note += g_resource->loadString(i);
    }
    Edit_SetText(GetDlgItem(m_hwnd, IDC_EDIT_note), note.c_str());
    return TRUE;
  }
  
  // WM_CLOSE
  BOOL wmClose()
  {
    EndDialog(m_hwnd, 0);
    return TRUE;
  }
  
  // WM_COMMAND
  BOOL wmCommand(int /* notify_code */, int i_id, HWND /* hwnd_control */)
  {
    switch (i_id)
    {
      case IDC_BUTTON_browse:
      {
	_TCHAR folder[GANA_MAX_PATH];
	
	BROWSEINFO bi;
	ZeroMemory(&bi, sizeof(bi));
	bi.hwndOwner      = m_hwnd;
	bi.pidlRoot       = NULL;
	bi.pszDisplayName = folder;
	bi.lpszTitle      = g_resource->loadString(IDS_selectDir);
	ITEMIDLIST *browse = SHBrowseForFolder(&bi);
	if (browse != NULL)
	{
	  if (SHGetPathFromIDList(browse, folder))
	  {
	    if (createDirectories(folder))
	      Edit_SetText(GetDlgItem(m_hwnd, IDC_EDIT_path), folder);
	  }
	  IMalloc *imalloc = NULL;
	  if (SHGetMalloc(&imalloc) == NOERROR)
	    imalloc->Free((void *)browse);
	}
	return TRUE;
      }
      
      case IDOK:
      {
	_TCHAR buf[GANA_MAX_PATH];
	Edit_GetText(GetDlgItem(m_hwnd, IDC_EDIT_path), buf, NUMBER_OF(buf));
	if (buf[0])
	{
	  g_destDir = normalizePath(buf);
	  m_doRegisterToStartMenu =
	    (IsDlgButtonChecked(m_hwnd, IDC_CHECK_registerStartMenu) ==
	     BST_CHECKED);
	  m_doRegisterToStartUp =
	    (IsDlgButtonChecked(m_hwnd, IDC_CHECK_registerStartUp) ==
	     BST_CHECKED);

	  int curSel =
	    ComboBox_GetCurSel(GetDlgItem(m_hwnd, IDC_COMBO_keyboard));
	  g_flags = SetupFile::Normal;
#if 0
	  if (checkOs(SetupFile::W2k))
#endif
	  {
	    switch (curSel)
	    {
	      case 0:
		g_keyboardKind = KEYBOARD_KIND_109;
		g_flags = Flag_Usb;
		break;
	      case 1:
		g_keyboardKind = KEYBOARD_KIND_104;
		g_flags = Flag_Usb;
		break;
#if 0
	      case 2: g_keyboardKind = KEYBOARD_KIND_109; break;
	      case 3: g_keyboardKind = KEYBOARD_KIND_104; break;
#endif
	    };
	  }
#if 0
	  else
	  {
	    switch (curSel)
	    {
	      case 0: g_keyboardKind = KEYBOARD_KIND_109; break;
	      case 1: g_keyboardKind = KEYBOARD_KIND_104; break;
	    };
	  }
#endif

#if 0
	  if (g_flags == Flag_Usb)
	    if (message(IDS_usbWarning, MB_OKCANCEL | MB_ICONWARNING, m_hwnd)
		== IDCANCEL)
	      return TRUE;
#endif
	  
	  if (createDirectories(g_destDir.c_str()))
	    EndDialog(m_hwnd, install());
	  else
	    message(IDS_invalidDirectory, MB_OK | MB_ICONSTOP, m_hwnd);
	}
	else
	  message(IDS_mayuEmpty, MB_OK, m_hwnd);
	return TRUE;
      }
      
      case IDCANCEL:
      {
	CHECK_TRUE( EndDialog(m_hwnd, 0) );
	return TRUE;
      }
    }
    return FALSE;
  }

public:
  DlgMain(HWND i_hwnd)
    : m_hwnd(i_hwnd),
      m_doRegisterToStartMenu(false),
      m_doRegisterToStartUp(false)
  {
  }

  static BOOL CALLBACK dlgProc(HWND i_hwnd, UINT i_message,
			       WPARAM i_wParam, LPARAM i_lParam)
  {
    DlgMain *wc;
    getUserData(i_hwnd, &wc);
    if (!wc)
      switch (i_message)
      {
	case WM_INITDIALOG:
	  wc = setUserData(i_hwnd, new DlgMain(i_hwnd));
	  return wc->wmInitDialog(reinterpret_cast<HWND>(i_wParam), i_lParam);
      }
    else
      switch (i_message)
      {
	case WM_COMMAND:
	  return wc->wmCommand(HIWORD(i_wParam), LOWORD(i_wParam),
			       reinterpret_cast<HWND>(i_lParam));
	case WM_CLOSE:
	  return wc->wmClose();
	case WM_NCDESTROY:
	  delete wc;
	  return TRUE;
      }
    return FALSE;
  }
};


// uninstall
// (in this function, we cannot use any resource, so we use strres[])
int uninstall()
{
  if (IDYES != message(IDS_removeOk, MB_YESNO | MB_ICONQUESTION))
    return 1;

#if defined(_WINNT)
  DWORD err = removeDriverService(_T("mayud"));
  if (err != ERROR_SUCCESS)
  {
    driverServiceError(err);
    return 1;
  }
#endif // _WINNT

  DeleteFile(getStartMenuName(
    g_resource->loadString(IDS_shortcutName)).c_str());
  DeleteFile(getStartUpName(
    g_resource->loadString(IDS_shortcutName)).c_str());

  removeFiles(g_setupFiles, NUMBER_OF(g_setupFiles), g_flags, g_destDir);
  removeFileExtension(_T(".mayu"), _T("mayu file"));
  removeUninstallInformation(_T("mayu"));
  
  Registry::remove(DIR_REGISTRY_ROOT);
  Registry::remove(HKEY_CURRENT_USER, _T("Software\\GANAware\\mayu"));
  
  message(IDS_removeFinish, MB_OK | MB_ICONINFORMATION);
  return 0;
}


int WINAPI _tWinMain(HINSTANCE i_hInstance, HINSTANCE /* hPrevInstance */,
		     LPTSTR /* lpszCmdLine */, int /* nCmdShow */)
{
  CoInitialize(NULL);
  
  g_hInst = i_hInstance;
  Resource resource(g_strres);
  g_resource = &resource;

  // check OS
  if (
#if defined(_WINNT)
      !checkOs(SetupFile::NT)
#elif defined(_WIN95)
      !checkOs(SetupFile::W9x)
#else
#  error
#endif
    )
  {
    message(IDS_invalidOS, MB_OK | MB_ICONSTOP);
    return 1;
  }

  // keyboard kind
  g_keyboardKind =
    (resource.getLocale() == LOCALE_Japanese_Japan_932) ?
    KEYBOARD_KIND_109 : KEYBOARD_KIND_104;

  // read registry
  tstringi programFiles;			// "Program Files" directory
  Registry::read(HKEY_LOCAL_MACHINE,
		 _T("Software\\Microsoft\\Windows\\CurrentVersion"),
		 _T("ProgramFilesDir"), &programFiles);
  Registry::read(DIR_REGISTRY_ROOT, _T("dir"), &g_destDir,
		 programFiles + _T("\\mayu"));

  int retval = 1;
  
  if (__argc == 2 && _tcsicmp(__targv[1], _T("-u")) == 0)
    retval = uninstallStep1(_T("-u"));
  else
  {
    HANDLE mutexPrevVer = CreateMutex(
      (SECURITY_ATTRIBUTES *)NULL, TRUE,
      MUTEX_MAYU_EXCLUSIVE_RUNNING);
    if (GetLastError() == ERROR_ALREADY_EXISTS) { // mayu is running
      message(IDS_mayuRunning, MB_OK | MB_ICONSTOP);
    } else {
      // is mayu running ?
      HANDLE mutex = CreateMutex(
	(SECURITY_ATTRIBUTES *)NULL, TRUE,
	addSessionId(MUTEX_MAYU_EXCLUSIVE_RUNNING).c_str());
      if (GetLastError() == ERROR_ALREADY_EXISTS) { // mayu is running
	message(IDS_mayuRunning, MB_OK | MB_ICONSTOP);
      } else if (__argc == 3 && _tcsicmp(__targv[1], _T("-u")) == 0) {
	uninstallStep2(__targv[2]);
	retval = uninstall();
      } else if (__argc == 2 && _tcsicmp(__targv[1], _T("-s")) == 0) {
	g_wasExecutedBySFX = true;
	retval = DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_main), NULL,
			   DlgMain::dlgProc);
      } else if (__argc == 1) {
	retval = DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_main), NULL,
			   DlgMain::dlgProc);
      }
      CloseHandle(mutex);
    }
    CloseHandle(mutexPrevVer);
  }
  
  return retval;
}
