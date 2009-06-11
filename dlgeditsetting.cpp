//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// dlgeditsetting.cpp


#include "misc.h"
#include "mayurc.h"
#include "windowstool.h"
#include "dlgeditsetting.h"
#include "layoutmanager.h"
#include <windowsx.h>


///
class DlgEditSetting : public LayoutManager
{
  HWND m_hwndMayuPathName;			///
  HWND m_hwndMayuPath;				///
  HWND m_hwndSymbols;				///

  DlgEditSettingData *m_data;			///

public:
  ///
  DlgEditSetting(HWND i_hwnd)
    : LayoutManager(i_hwnd),
      m_hwndMayuPathName(NULL),
      m_hwndMayuPath(NULL),
      m_hwndSymbols(NULL),
      m_data(NULL)
  {
  }
  
  /// WM_INITDIALOG
  BOOL wmInitDialog(HWND /* focus */, LPARAM i_lParam)
  {
    m_data = reinterpret_cast<DlgEditSettingData *>(i_lParam);
    
    setSmallIcon(m_hwnd, IDI_ICON_mayu);
    setBigIcon(m_hwnd, IDI_ICON_mayu);
    
    CHECK_TRUE( m_hwndMayuPathName
		= GetDlgItem(m_hwnd, IDC_EDIT_mayuPathName) );
    CHECK_TRUE( m_hwndMayuPath = GetDlgItem(m_hwnd, IDC_EDIT_mayuPath) );
    CHECK_TRUE( m_hwndSymbols = GetDlgItem(m_hwnd, IDC_EDIT_symbols) );

    SetWindowText(m_hwndMayuPathName, m_data->m_name.c_str());
    SetWindowText(m_hwndMayuPath, m_data->m_filename.c_str());
    SetWindowText(m_hwndSymbols, m_data->m_symbols.c_str());
    
    restrictSmallestSize();
    
    // set layout manager
    typedef LayoutManager LM;

    addItem(GetDlgItem(m_hwnd, IDC_STATIC_mayuPathName));
    addItem(GetDlgItem(m_hwnd, IDC_EDIT_mayuPathName),
	    LM::ORIGIN_LEFT_EDGE, LM::ORIGIN_TOP_EDGE,
	    LM::ORIGIN_RIGHT_EDGE, LM::ORIGIN_TOP_EDGE);
    addItem(GetDlgItem(m_hwnd, IDC_STATIC_mayuPathNameComment),
	    LM::ORIGIN_RIGHT_EDGE, LM::ORIGIN_TOP_EDGE,
	    LM::ORIGIN_RIGHT_EDGE, LM::ORIGIN_TOP_EDGE);

    addItem(GetDlgItem(m_hwnd, IDC_STATIC_mayuPath));
    addItem(GetDlgItem(m_hwnd, IDC_EDIT_mayuPath),
	    LM::ORIGIN_LEFT_EDGE, LM::ORIGIN_TOP_EDGE,
	    LM::ORIGIN_RIGHT_EDGE, LM::ORIGIN_TOP_EDGE);
    addItem(GetDlgItem(m_hwnd, IDC_BUTTON_browse),
	    LM::ORIGIN_RIGHT_EDGE, LM::ORIGIN_TOP_EDGE,
	    LM::ORIGIN_RIGHT_EDGE, LM::ORIGIN_TOP_EDGE);

    addItem(GetDlgItem(m_hwnd, IDC_STATIC_symbols));
    addItem(GetDlgItem(m_hwnd, IDC_EDIT_symbols),
	    LM::ORIGIN_LEFT_EDGE, LM::ORIGIN_TOP_EDGE,
	    LM::ORIGIN_RIGHT_EDGE, LM::ORIGIN_TOP_EDGE);
    addItem(GetDlgItem(m_hwnd, IDC_STATIC_symbolsComment),
	    LM::ORIGIN_RIGHT_EDGE, LM::ORIGIN_TOP_EDGE,
	    LM::ORIGIN_RIGHT_EDGE, LM::ORIGIN_TOP_EDGE);
    
    addItem(GetDlgItem(m_hwnd, IDOK),
	    LM::ORIGIN_CENTER, LM::ORIGIN_TOP_EDGE,
	    LM::ORIGIN_CENTER, LM::ORIGIN_TOP_EDGE);
    addItem(GetDlgItem(m_hwnd, IDCANCEL),
	    LM::ORIGIN_CENTER, LM::ORIGIN_TOP_EDGE,
	    LM::ORIGIN_CENTER, LM::ORIGIN_TOP_EDGE);

    restrictSmallestSize(LM::RESTRICT_BOTH);
    restrictLargestSize(LM::RESTRICT_VERTICALLY);
    
    return TRUE;
  }
  
  /// WM_CLOSE
  BOOL wmClose()
  {
    CHECK_TRUE( EndDialog(m_hwnd, 0) );
    return TRUE;
  }

  /// WM_COMMAND
  BOOL wmCommand(int /* i_notify_code */, int i_id, HWND /* i_hwnd_control */)
  {
    _TCHAR buf[GANA_MAX_PATH];
    switch (i_id)
    {
      case IDC_BUTTON_browse:
      {
	tstring title = loadString(IDS_openMayu);
	tstring filter = loadString(IDS_openMayuFilter);
	for (size_t i = 0; i < filter.size(); ++ i)
	  if (filter[i] == _T('|'))
	    filter[i] = _T('\0');

	_tcscpy(buf, _T(".mayu"));
	OPENFILENAME of;
	memset(&of, 0, sizeof(of));
	of.lStructSize = sizeof(of);
	of.hwndOwner = m_hwnd;
	of.lpstrFilter = filter.c_str();
	of.nFilterIndex = 1;
	of.lpstrFile = buf;
	of.nMaxFile = NUMBER_OF(buf);
	of.lpstrTitle = title.c_str();
	of.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST |
	  OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
	if (GetOpenFileName(&of))
	  SetWindowText(m_hwndMayuPath, buf);
	return TRUE;
      }
      
      case IDOK:
      {
	GetWindowText(m_hwndMayuPathName, buf, NUMBER_OF(buf));
	m_data->m_name = buf;
	GetWindowText(m_hwndMayuPath, buf, NUMBER_OF(buf));
	m_data->m_filename = buf;
	GetWindowText(m_hwndSymbols, buf, NUMBER_OF(buf));
	m_data->m_symbols = buf;
	CHECK_TRUE( EndDialog(m_hwnd, 1) );
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
};


//
BOOL CALLBACK dlgEditSetting_dlgProc(HWND i_hwnd, UINT i_message,
				     WPARAM i_wParam, LPARAM i_lParam)
{
  DlgEditSetting *wc;
  getUserData(i_hwnd, &wc);
  if (!wc)
    switch (i_message)
    {
      case WM_INITDIALOG:
	wc = setUserData(i_hwnd, new DlgEditSetting(i_hwnd));
	return wc->wmInitDialog(
	  reinterpret_cast<HWND>(i_wParam), i_lParam);
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
