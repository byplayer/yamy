//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// dlginvestigate.cpp


#include "misc.h"
#include "engine.h"
#include "focus.h"
#include "hook.h"
#include "mayurc.h"
#include "stringtool.h"
#include "target.h"
#include "windowstool.h"
#include "vkeytable.h"
#include "dlginvestigate.h"
#include <iomanip>


///
class DlgInvestigate
{
  HWND m_hwnd;					///
  UINT m_WM_MAYU_MESSAGE;			///
  DlgInvestigateData m_data;			/// 
  
public:
  ///
  DlgInvestigate(HWND i_hwnd)
    : m_hwnd(i_hwnd),
      m_WM_MAYU_MESSAGE(RegisterWindowMessage(
			    addSessionId(WM_MAYU_MESSAGE_NAME).c_str()))
  {
    m_data.m_engine = NULL;
    m_data.m_hwndLog = NULL;
  }
  
  /// WM_INITDIALOG
  BOOL wmInitDialog(HWND /* i_focus */, LPARAM i_lParam)
  {
    m_data = *reinterpret_cast<DlgInvestigateData *>(i_lParam);
    setSmallIcon(m_hwnd, IDI_ICON_mayu);
    setBigIcon(m_hwnd, IDI_ICON_mayu);
    return TRUE;
  }
  
  /// WM_DESTROY
  BOOL wmDestroy()
  {
    unsetSmallIcon(m_hwnd);
    unsetBigIcon(m_hwnd);
    return TRUE;
  }
  
  /// WM_CLOSE
  BOOL wmClose()
  {
    ShowWindow(m_hwnd, SW_HIDE);
    return TRUE;
  }

  /// WM_COMMAND
  BOOL wmCommand(int /* i_notifyCode */, int i_id, HWND /* i_hwndControl */)
  {
    switch (i_id)
    {
      case IDOK:
      {
	ShowWindow(m_hwnd, SW_HIDE);
	return TRUE;
      }
    }
    return FALSE;
  }

  /// WM_focusNotify
  BOOL wmFocusNotify(bool i_isFocused, HWND i_hwndFocus)
  {
    if (m_data.m_engine &&
	i_hwndFocus == GetDlgItem(m_hwnd, IDC_CUSTOM_scancode))
      m_data.m_engine->enableLogMode(i_isFocused);
    return TRUE;
  }
  
  /// WM_targetNotify
  BOOL wmTargetNotify(HWND i_hwndTarget)
  {
    _TCHAR className[GANA_MAX_ATOM_LENGTH];
    bool ok = false;
    if (GetClassName(i_hwndTarget, className, NUMBER_OF(className)))
    {
      if (_tcsicmp(className, _T("ConsoleWindowClass")) == 0)
      {
	_TCHAR titleName[1024];
	if (GetWindowText(i_hwndTarget, titleName, NUMBER_OF(titleName)) == 0)
	  titleName[0] = _T('\0');
	{
	  Acquire a(&m_data.m_engine->m_log, 1);
	  m_data.m_engine->m_log << _T("HWND:\t") << std::hex
				 << reinterpret_cast<int>(i_hwndTarget)
				 << std::dec << std::endl;
	}
	Acquire a(&m_data.m_engine->m_log, 0);
	m_data.m_engine->m_log << _T("CLASS:\t") << className << std::endl;
	m_data.m_engine->m_log << _T("TITLE:\t") << titleName << std::endl;
	ok = true;
      }
    }
    if (!ok)
      CHECK_TRUE( PostMessage(i_hwndTarget, m_WM_MAYU_MESSAGE,
			      MayuMessage_notifyName, 0) );
    return TRUE;
  }
  
  /// WM_vkeyNotify
  BOOL wmVkeyNotify(int i_nVirtKey, int /* i_repeatCount */,
		    BYTE /* i_scanCode */, bool i_isExtended,
		    bool /* i_isAltDown */, bool i_isKeyup)
  {
    Acquire a(&m_data.m_engine->m_log, 0);
    m_data.m_engine->m_log
      << (i_isExtended ? _T(" E-") : _T("   "))
      << _T("0x") << std::hex << std::setw(2) << std::setfill(_T('0'))
      << i_nVirtKey << std::dec << _T("  &VK( ")
      << (i_isExtended ? _T("E-") : _T("  "))
      << (i_isKeyup ? _T("U-") : _T("D-"));
    
    for (const VKeyTable *vkt = g_vkeyTable; vkt->m_name; ++ vkt)
    {
      if (vkt->m_code == i_nVirtKey)
      {
	m_data.m_engine->m_log << vkt->m_name << _T(" )") << std::endl;
	return TRUE;
      }
    }
    m_data.m_engine->m_log << _T("0x") << std::hex << std::setw(2)
			   << std::setfill(_T('0')) << i_nVirtKey << std::dec
			   << _T(" )") << std::endl;
    return TRUE;
  }

  BOOL wmMove(int /* i_x */, int /* i_y */)
  {
    RECT rc1, rc2;
    GetWindowRect(m_hwnd, &rc1);
    GetWindowRect(m_data.m_hwndLog, &rc2);
    
    MoveWindow(m_data.m_hwndLog, rc1.left, rc1.bottom,
	       rcWidth(&rc2), rcHeight(&rc2), TRUE);
    
    return TRUE;
  }
};


//
BOOL CALLBACK dlgInvestigate_dlgProc(HWND i_hwnd, UINT i_message,
				     WPARAM i_wParam, LPARAM i_lParam)
{
  DlgInvestigate *wc;
  getUserData(i_hwnd, &wc);
  if (!wc)
    switch (i_message)
    {
      case WM_INITDIALOG:
	wc = setUserData(i_hwnd, new DlgInvestigate(i_hwnd));
	return wc->wmInitDialog(reinterpret_cast<HWND>(i_wParam), i_lParam);
    }
  else
    switch (i_message)
    {
      case WM_MOVE:
	return wc->wmMove(static_cast<short>(LOWORD(i_lParam)),
			  static_cast<short>(HIWORD(i_lParam)));
      case WM_COMMAND:
	return wc->wmCommand(HIWORD(i_wParam), LOWORD(i_wParam),
			     reinterpret_cast<HWND>(i_lParam));
      case WM_CLOSE:
	return wc->wmClose();
      case WM_DESTROY:
	return wc->wmDestroy();
      case WM_APP_notifyFocus:
	return wc->wmFocusNotify(!!i_wParam,
				 reinterpret_cast<HWND>(i_lParam));
      case WM_APP_targetNotify:
	return wc->wmTargetNotify(reinterpret_cast<HWND>(i_lParam));
      case WM_APP_notifyVKey:
	return wc->wmVkeyNotify(
	  static_cast<int>(i_wParam), static_cast<int>(i_lParam & 0xffff),
	  static_cast<BYTE>((i_lParam >> 16) & 0xff),
	  !!(i_lParam & (1 << 24)),
	  !!(i_lParam & (1 << 29)),
	  !!(i_lParam & (1 << 31)));
      case WM_NCDESTROY:
	delete wc;
	return TRUE;
    }
  return FALSE;
}
