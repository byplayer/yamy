//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// hook.h


#ifndef _HOOK_H
#  define _HOOK_H

#  include "misc.h"
#  include <tchar.h>
#ifdef NO_DRIVER
#  include <windows.h>
#endif // NO_DRIVER

///
#  define HOOK_PIPE_NAME \
 _T("\\\\.\\pipe\\GANAware\\mayu\\{4B22D464-7A4E-494b-982A-C2B2BBAAF9F3}") _T(VERSION)
///
#ifdef USE_MAILSLOT
#  define NOTIFY_MAILSLOT_NAME \
_T("\\\\.\\mailslot\\GANAware\\mayu\\{330F7914-EB5B-49be-ACCE-D2B8DF585B32}") _T(VERSION)
///
#endif // USE_MAILSLOT
#  define WM_MAYU_MESSAGE_NAME _T("GANAware\\mayu\\WM_MAYU_MESSAGE")

///
enum MayuMessage {
	MayuMessage_notifyName,
	MayuMessage_funcRecenter,
	MayuMessage_funcSetImeStatus,
	MayuMessage_funcSetImeString,
};


///
struct Notify {
	///
	enum Type {
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
struct NotifySetFocus : public Notify {
	DWORD m_threadId;				///
	DWORD m_hwnd;				///
	_TCHAR m_className[GANA_MAX_PATH];		///
	_TCHAR m_titleName[GANA_MAX_PATH];		///
};


///
struct NotifyLockState : public Notify {
	bool m_isNumLockToggled;			///
	bool m_isCapsLockToggled;			///
	bool m_isScrollLockToggled;			///
	bool m_isKanaLockToggled;			///
	bool m_isImeLockToggled;			///
	bool m_isImeCompToggled;			///
};


///
struct NotifyThreadDetach : public Notify {
	DWORD m_threadId;				///
};


///
struct NotifyCommand : public Notify {
	HWND m_hwnd;					///
	UINT m_message;				///
	WPARAM m_wParam;				///
	LPARAM m_lParam;				///
};


enum {
	NOTIFY_MESSAGE_SIZE = sizeof(NotifySetFocus),	///
};


///
struct NotifyShow : public Notify {
	///
	enum Show {
		Show_Normal,
		Show_Maximized,
		Show_Minimized,
	};
	Show m_show;					///
	bool m_isMDI;					///
};


///
struct NotifyLog : public Notify {
	_TCHAR m_msg[GANA_MAX_PATH];			///
};


///
enum MouseHookType {
	MouseHookType_None = 0,				/// none
	MouseHookType_Wheel = 1 << 0,			/// wheel
	MouseHookType_WindowMove = 1 << 1,		/// window move
};

#ifdef NO_DRIVER
class Engine;
typedef unsigned int (WINAPI *KEYBOARD_DETOUR)(Engine *i_engine, KBDLLHOOKSTRUCT *i_kid);
#endif // NO_DRIVER

///
class HookData
{
public:
	USHORT m_syncKey;				///
	bool m_syncKeyIsExtended;			///
	bool m_doesNotifyCommand;			///
	DWORD m_hwndTaskTray;				///
	bool m_correctKanaLockHandling;		/// does use KL- ?
	MouseHookType m_mouseHookType;		///
	int m_mouseHookParam;			///
	DWORD m_hwndMouseHookTarget;		///
	POINT m_mousePos;				///
};


///
#  define DllExport __declspec(dllexport)
///
#  define DllImport __declspec(dllimport)


#  ifndef _HOOK_CPP
extern DllImport HookData *g_hookData;
extern DllImport int installMessageHook();
extern DllImport int uninstallMessageHook();
extern DllImport int installKeyboardHook(KEYBOARD_DETOUR i_keyboardDetour, Engine *i_engine, bool i_install);
extern DllImport int installMouseHook(KEYBOARD_DETOUR i_keyboardDetour, Engine *i_engine, bool i_install);
extern DllImport bool notify(void *data, size_t sizeof_data);
extern DllImport void notifyLockState();
#  endif // !_HOOK_CPP


#endif // !_HOOK_H
