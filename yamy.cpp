//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// yamy.cpp

#include <windows.h>
#include <tchar.h>
#include "stringtool.h"
#include "mayurc.h"

/// main
int WINAPI _tWinMain(HINSTANCE i_hInstance, HINSTANCE /* i_hPrevInstance */,
					 LPTSTR /* i_lpszCmdLine */, int /* i_nCmdShow */)
{
	typedef BOOL (WINAPI* ISWOW64PROCESS)(HANDLE hProcess, PBOOL Wow64Process);
	BOOL isWow64;
	ISWOW64PROCESS pIsWow64Process;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	BOOL result;
	tstring yamyPath;
	_TCHAR exePath[GANA_MAX_PATH];
	_TCHAR exeDrive[GANA_MAX_PATH];
	_TCHAR exeDir[GANA_MAX_PATH];

	GetModuleFileName(NULL, exePath, GANA_MAX_PATH);
	_tsplitpath_s(exePath, exeDrive, GANA_MAX_PATH, exeDir, GANA_MAX_PATH, NULL, 0, NULL, 0);
	yamyPath = exeDrive;
	yamyPath += exeDir;

	pIsWow64Process =
		(ISWOW64PROCESS)::GetProcAddress(::GetModuleHandle(_T("kernel32.dll")),
										 "IsWow64Process");
	ZeroMemory(&pi,sizeof(pi));
	ZeroMemory(&si,sizeof(si));
	si.cb=sizeof(si);

	if (pIsWow64Process) {
		result = pIsWow64Process(::GetCurrentProcess(), &isWow64);
		if (result != FALSE && isWow64 == TRUE) {
			yamyPath += _T("yamy64");
		} else {
			yamyPath += _T("yamy32");
		}
	} else {
		yamyPath += _T("yamy32");
	}

	result = CreateProcess(yamyPath.c_str(), NULL, NULL, NULL, FALSE,
						   NORMAL_PRIORITY_CLASS, 0, NULL, &si, &pi);

	if (result == FALSE) {
		TCHAR buf[1024];
		TCHAR text[1024];
		TCHAR title[1024];

		LoadString(i_hInstance, IDS_cannotInvoke,
				   text, sizeof(text)/sizeof(text[0]));
		LoadString(i_hInstance, IDS_mayu,
				   title, sizeof(title)/sizeof(title[0]));
		_stprintf_s(buf, sizeof(buf)/sizeof(buf[0]),
					text, yamyPath, GetLastError());
		MessageBox((HWND)NULL, buf, title, MB_OK | MB_ICONSTOP);
	} else {
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	}

	return 0;
}
