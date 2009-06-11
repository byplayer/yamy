//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// hook.h


#ifndef _HOOK_H
#  define _HOOK_H

#  include "misc.h"
#  include <tchar.h>

///
#  define HOOK_PIPE_NAME \
 _T("\\\\.\\pipe\\GANAware\\mayu\\{4B22D464-7A4E-494b-982A-C2B2BBAAF9F3}") _T(VERSION)
///
#  define WM_MAYU_MESSAGE_NAME _T("GANAware\\mayu\\WM_MAYU_MESSAGE")

///
enum MayuMessage
{
  MayuMessage_notifyName,
  MayuMessage_funcRecenter,
  MayuMessage_funcSetImeStatus,
  MayuMessage_funcSetImeString,
};


///
struct Notify
{
  ///
  enum Type
  {
    Type_setFocus,				/// NotifySetFocus
    Type_name,					/// NotifySetFocus
    Type_lockState,				/// NotifyLockState
    Type_sync,					/// Notify
    Type_threadDetach,				/// NotifyThreadDetach
    Type_command,				/// NotifyThreadDetach
    Type_show,					/// NotifyShow
    Type_log,					/// NotifyLog
  };
  Type m_type;					///
  DWORD m_debugParam;				/// (for debug)
};


///
struct NotifySetFocus : public Notify
{
  DWORD m_threadId;				///
  HWND m_hwnd;					///
  _TCHAR m_className[GANA_MAX_PATH];		///
  _TCHAR m_titleName[GANA_MAX_PATH];		///
};


///
struct NotifyLockState : public Notify
{
  bool m_isNumLockToggled;			///
  bool m_isCapsLockToggled;			///
  bool m_isScrollLockToggled;			///
  bool m_isKanaLockToggled;			///
  bool m_isImeLockToggled;			///
  bool m_isImeCompToggled;			///
};


///
struct NotifyThreadDetach : public Notify
{
  DWORD m_threadId;				///
};


///
struct NotifyCommand : public Notify
{
  HWND m_hwnd;					///
  UINT m_message;				///
  WPARAM m_wParam;				///
  LPARAM m_lParam;				///
};


enum
{
  NOTIFY_MESSAGE_SIZE = sizeof(NotifySetFocus),	///
};


///
struct NotifyShow : public Notify
{
  ///
  enum Show
  {
    Show_Normal,
    Show_Maximized,
    Show_Minimized,
  };
  Show m_show;					///
  bool m_isMDI;					///
};


///
struct NotifyLog : public Notify
{
  _TCHAR m_msg[GANA_MAX_PATH];			///
};


///
enum MouseHookType
{
  MouseHookType_None = 0,				/// none
  MouseHookType_Wheel = 1 << 0,			/// wheel
  MouseHookType_WindowMove = 1 << 1,		/// window move
};

///
class HookData
{
public:
  HHOOK m_hHookGetMessage;			///
  HHOOK m_hHookCallWndProc;			///
  HHOOK m_hHookMouseProc;			///
  USHORT m_syncKey;				///
  bool m_syncKeyIsExtended;			///
  bool m_doesNotifyCommand;			///
  HWND m_hwndTaskTray;				///
  bool m_correctKanaLockHandling;		/// does use KL- ?
  MouseHookType m_mouseHookType;		///
  int m_mouseHookParam;			///
  HWND m_hwndMouseHookTarget;		///
  POINT m_mousePos;				///
};


///
#  define DllExport __declspec(dllexport)
///
#  define DllImport __declspec(dllimport)


#  ifndef _HOOK_CPP
extern DllImport HookData *g_hookData;
extern DllImport int installHooks();
extern DllImport int uninstallHooks();
extern DllImport bool notify(void *data, size_t sizeof_data);
extern DllImport void notifyLockState();
#  endif // !_HOOK_CPP


#endif // !_HOOK_H
