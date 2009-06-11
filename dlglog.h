//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// dlglog.h


#ifndef _DLGLOG_H
#  define _DLGLOG_H

#  include <windows.h>
#  include "msgstream.h"


//
BOOL CALLBACK dlgLog_dlgProc(
  HWND i_hwnd, UINT i_message, WPARAM i_wParam, LPARAM i_lParam);

enum
{
  ///
  WM_APP_dlglogNotify = WM_APP + 115,
};

enum DlgLogNotify
{
  DlgLogNotify_logCleared,			///
};

/// parameters for "Investigate" dialog box
class DlgLogData
{
public:
  tomsgstream *m_log;				/// log stream
  HWND m_hwndTaskTray;				/// tasktray window
};

#endif // !_DLGLOG_H
