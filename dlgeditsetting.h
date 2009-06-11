//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// dlgeditsetting.h


#ifndef _DLGEDITSETTING_H
#  define _DLGEDITSETTING_H

#  include "stringtool.h"


/// dialog procedure of "Edit Setting" dialog box
BOOL CALLBACK dlgEditSetting_dlgProc(
  HWND i_hwnd, UINT i_message, WPARAM i_wParam, LPARAM i_lParam);

/// parameters for "Edit Setting" dialog box
class DlgEditSettingData
{
public:
  tstringi m_name;				/// setting name
  tstringi m_filename;				/// filename of setting
  tstringi m_symbols;		/// symbol list (-Dsymbol1;-Dsymbol2;-D...)
};


#endif // !_DLGEDITSETTING_H
