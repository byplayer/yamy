//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// mayuipc.h - mayu inter process communication

#ifndef _MAYUIPC_H
#  define _MAYUIPC_H

#  include <windows.h>

#  ifdef __cplusplus
extern "C" {
#  endif // __cplusplus

/// 
#  define WM_MayuIPC_NAME _T("MayuIPC{46269F4D-D560-40f9-B38B-DB5E280FEF47}")

enum MayuIPCCommand
{
	// enable or disable Mayu
	MayuIPCCommand_Enable = 1,
};

BOOL MayuIPC_PostMessage(MayuIPCCommand i_wParam, LPARAM i_lParam);
BOOL MayuIPC_Enable(BOOL i_isEnabled);

#  ifdef _MAYUIPC_H_DEFINE_FUNCTIONS

BOOL MayuIPC_PostMessage(MayuIPCCommand i_command, LPARAM i_lParam)
{
	static UINT WM_MayuIPC;
	HWND hwnd;
	
	if (WM_MayuIPC == 0)
	{
		WM_MayuIPC = RegisterWindowMessage(WM_MayuIPC_NAME);
		if (WM_MayuIPC == 0)
		{
			return FALSE;
		}
	}
	
	hwnd = FindWindow(_T("mayuTasktray"), NULL);
	if (hwnd == NULL)
	{
		return FALSE;
	}
	PostMessage(hwnd, WM_MayuIPC, i_command, i_lParam);
	return TRUE;
}

BOOL MayuIPC_Enable(BOOL i_isEnabled)
{
	return MayuIPC_PostMessage(MayuIPCCommand_Enable, i_isEnabled);
}

#  endif // _MAYUIPC_H_DEFINE_FUNCTIONS

#  ifdef __cplusplus
}
#  endif // __cplusplus
#endif // !_MAYUIPC_H
