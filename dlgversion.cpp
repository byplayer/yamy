//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// dlgversion.cpp


#include "misc.h"

#include "mayu.h"
#include "mayurc.h"
#include "windowstool.h"
#include "compiler_specific_func.h"
#include "layoutmanager.h"

#include <cstdio>
#include <windowsx.h>


///
class DlgVersion : public LayoutManager
{
  HWND m_hwnd;		///
  
public:
  ///
  DlgVersion(HWND i_hwnd)
    : LayoutManager(i_hwnd),
      m_hwnd(i_hwnd)
  {
  }
  
  /// WM_INITDIALOG
  BOOL wmInitDialog(HWND /* i_focus */, LPARAM i_lParam)
  {
    TCHAR *mayudVersion = (TCHAR*)i_lParam;
    setSmallIcon(m_hwnd, IDI_ICON_mayu);
    setBigIcon(m_hwnd, IDI_ICON_mayu);
    
    _TCHAR modulebuf[1024];
    CHECK_TRUE( GetModuleFileName(g_hInst, modulebuf,
				  NUMBER_OF(modulebuf)) );
    
    _TCHAR buf[1024];
    _sntprintf(buf, NUMBER_OF(buf), loadString(IDS_version).c_str(),
	       _T(VERSION)
#ifndef NDEBUG
	       _T(" (DEBUG)")
#endif // !NDEBUG
#ifdef _UNICODE
	       _T(" (UNICODE)")
#endif // !_UNICODE
	       ,
	       mayudVersion,
	       loadString(IDS_homepage).c_str(),
	       (_T(LOGNAME) _T("@") + toLower(_T(COMPUTERNAME))).c_str(),
	       _T(__DATE__) _T(" ") _T(__TIME__),
	       getCompilerVersionString().c_str(),
	       modulebuf);
    
    
    Edit_SetText(GetDlgItem(m_hwnd, IDC_EDIT_builtBy), buf);
    
    // set layout manager
    typedef LayoutManager LM;

    addItem(GetDlgItem(m_hwnd, IDC_STATIC_mayuIcon),
	    LM::ORIGIN_LEFT_EDGE, LM::ORIGIN_TOP_EDGE,
	    LM::ORIGIN_LEFT_EDGE, LM::ORIGIN_TOP_EDGE);
    addItem(GetDlgItem(m_hwnd, IDC_EDIT_builtBy),
	    LM::ORIGIN_LEFT_EDGE, LM::ORIGIN_TOP_EDGE,
	    LM::ORIGIN_RIGHT_EDGE, LM::ORIGIN_BOTTOM_EDGE);
    addItem(GetDlgItem(m_hwnd, IDC_BUTTON_download),
	    LM::ORIGIN_CENTER, LM::ORIGIN_BOTTOM_EDGE,
	    LM::ORIGIN_CENTER, LM::ORIGIN_BOTTOM_EDGE);
    addItem(GetDlgItem(m_hwnd, IDOK),
	    LM::ORIGIN_CENTER, LM::ORIGIN_BOTTOM_EDGE,
	    LM::ORIGIN_CENTER, LM::ORIGIN_BOTTOM_EDGE);
    restrictSmallestSize();
    
    return TRUE;
  }
  
  /// WM_CLOSE
  BOOL wmClose()
  {
    CHECK_TRUE( EndDialog(m_hwnd, 0) );
    return TRUE;
  }

  /// WM_COMMAND
  BOOL wmCommand(int /* i_notifyCode */, int i_id, HWND /* i_hwndControl */)
  {
    switch (i_id)
    {
      case IDOK:
      {
	CHECK_TRUE( EndDialog(m_hwnd, 0) );
	return TRUE;
      }
      case IDC_BUTTON_download:
      {
	ShellExecute(NULL, NULL, loadString(IDS_homepage).c_str(),
		     NULL, NULL, SW_SHOWNORMAL);
	CHECK_TRUE( EndDialog(m_hwnd, 0) );
	return TRUE;
      }
    }
    return FALSE;
  }
};


//
BOOL CALLBACK dlgVersion_dlgProc(
  HWND i_hwnd, UINT i_message, WPARAM i_wParam, LPARAM i_lParam)
{
  DlgVersion *wc;
  getUserData(i_hwnd, &wc);
  if (!wc)
    switch (i_message)
    {
      case WM_INITDIALOG:
	wc = setUserData(i_hwnd, new DlgVersion(i_hwnd));
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
      default:
	return wc->defaultWMHandler(i_message, i_wParam, i_lParam);
    }
  return FALSE;
}
