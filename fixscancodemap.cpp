#include "fixscancodemap.h"
#include "registry.h"
#include "misc.h"
#include <tchar.h>
#include <tlhelp32.h>

typedef HMODULE (WINAPI *FpGetModuleHandleW)(LPCWSTR);
typedef FARPROC (WINAPI *FpGetProcAddress)(HMODULE, LPCSTR);
typedef BOOL (WINAPI *FpUpdatePerUserSystemParameters)(DWORD, BOOL);
typedef HANDLE (WINAPI *FpOpenProcess)(DWORD, BOOL, DWORD);
typedef BOOL (WINAPI *FpOpenProcessToken)(HANDLE, DWORD, PHANDLE);
typedef BOOL (WINAPI *FpImpersonateLoggedOnUser)(HANDLE);
typedef BOOL (WINAPI *FpRevertToSelf)(VOID);
typedef BOOL (WINAPI *FpCloseHandle)(HANDLE);

typedef BOOL (WINAPI *FpRegisterShellHook)(HWND, DWORD);

typedef struct {
	DWORD retval_;
	DWORD pid_;
	TCHAR advapi32_[64];
	CHAR impersonateLoggedOnUser_[32];
	CHAR revertToSelf_[32];
	CHAR openProcessToken_[32];
	FpGetModuleHandleW pGetModuleHandle;
	FpGetProcAddress pGetProcAddress;
	FpUpdatePerUserSystemParameters pUpdate;
	FpOpenProcess pOpenProcess;
	FpCloseHandle pCloseHandle;
} InjectInfo;

#pragma runtime_checks( "", off )
static DWORD invokeFunc(InjectInfo *info)
{
	BOOL ret;
	HANDLE hToken;
	HMODULE hAdvapi32;
	FpImpersonateLoggedOnUser pImpersonateLoggedOnUser;
	FpRevertToSelf pRevertToSelf;
	FpOpenProcessToken pOpenProcessToken;

	info->retval_ = 0;

	hAdvapi32 = info->pGetModuleHandle(info->advapi32_);

	pImpersonateLoggedOnUser = (FpImpersonateLoggedOnUser)info->pGetProcAddress(hAdvapi32, info->impersonateLoggedOnUser_);
	pRevertToSelf = (FpRevertToSelf)info->pGetProcAddress(hAdvapi32, info->revertToSelf_);
	pOpenProcessToken = (FpOpenProcessToken)info->pGetProcAddress(hAdvapi32, info->openProcessToken_);

	HANDLE hProcess = info->pOpenProcess(PROCESS_QUERY_INFORMATION, FALSE, info->pid_);
	if (hProcess == NULL) {
		info->retval_ = 1;
		return 0;
	}

	ret = pOpenProcessToken(hProcess, TOKEN_QUERY | TOKEN_DUPLICATE , &hToken);
	if (ret == FALSE) {
		info->retval_ = 2;
		return 0;
	}

	ret = pImpersonateLoggedOnUser(hToken);
	if (ret == FALSE) {
		info->retval_ = 3;
		return 0;
	}

	info->pUpdate(0, 1);

	ret = pRevertToSelf();
	if (ret == FALSE) {
		info->retval_ = 4;
		return 0;
	}

	info->pCloseHandle(hToken);
	info->pCloseHandle(hProcess);
	return 0;
}
static void afterFunc(void){}
#pragma runtime_checks( "", restore )

int FixScancodeMap::acquirePrivileges()
{
	int ret = 0;
	HANDLE hToken = NULL;

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken)) {
		ret = 1;
		goto exit;
	}

	LUID luid;
	if (!LookupPrivilegeValue(NULL, _T("SeDebugPrivilege"), &luid)) {
		ret = 2;
		goto exit;
	}

	TOKEN_PRIVILEGES tk_priv;
	tk_priv.PrivilegeCount = 1;
	tk_priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	tk_priv.Privileges[0].Luid = luid;

	if (!AdjustTokenPrivileges(hToken, FALSE, &tk_priv, 0, NULL, NULL)) {
		ret = 3;
		goto exit;
	}

exit:
	if (hToken != NULL) {
	    CloseHandle(hToken);
	}
	return ret;
}


DWORD FixScancodeMap::getWinLogonPid()
{
    DWORD pid = 0;
	DWORD mySessionId = 0;

	if (ProcessIdToSessionId(GetCurrentProcessId(), &mySessionId) == FALSE) {
		return 0;
	}

	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnap == INVALID_HANDLE_VALUE) {
		return 0;
	}

	PROCESSENTRY32 pe;
	pe.dwSize = sizeof(pe);

    BOOL bResult = Process32First(hSnap, &pe);
	while (bResult){
		if (!_tcscmp(pe.szExeFile, _T("winlogon.exe"))) {
			DWORD sessionId;

			if (ProcessIdToSessionId(pe.th32ProcessID, &sessionId) == FALSE) {
				pid = 0;
				break;
			}

			if (sessionId == mySessionId) {
				pid = pe.th32ProcessID;
				break;
			}
		}
		bResult = Process32Next(hSnap, &pe);
	}

	CloseHandle(hSnap);
	return pid;
}


int FixScancodeMap::injectThread(DWORD dwPID)
{
	int ret = 0;
	DWORD err = 0;
	BOOL wFlag;

	HANDLE hProcess = NULL;
	LPVOID remoteMem = NULL;
	LPVOID remoteInfo = NULL;
	DWORD invokeFuncAddr = (DWORD)invokeFunc;
	DWORD afterFuncAddr = (DWORD)afterFunc;
	DWORD memSize =  afterFuncAddr - invokeFuncAddr;
	InjectInfo info;
	HMODULE hMod;

	info.pid_ = GetCurrentProcessId();

	memcpy(&info.advapi32_, _T("advapi32.dll"), sizeof(info.advapi32_));
	memcpy(&info.impersonateLoggedOnUser_, "ImpersonateLoggedOnUser", sizeof(info.impersonateLoggedOnUser_));
	memcpy(&info.revertToSelf_, "RevertToSelf", sizeof(info.revertToSelf_));
	memcpy(&info.openProcessToken_, "OpenProcessToken", sizeof(info.openProcessToken_));

	hMod = GetModuleHandle(_T("user32.dll"));
	if (hMod != NULL) {
		info.pUpdate = (FpUpdatePerUserSystemParameters)GetProcAddress(hMod, "UpdatePerUserSystemParameters");
		if (info.pUpdate == NULL) {
			return 1;
		}
	}

	hMod = GetModuleHandle(_T("kernel32.dll"));
	if (hMod != NULL) {
		info.pGetModuleHandle = (FpGetModuleHandleW)GetProcAddress(hMod, "GetModuleHandleW");
		if (info.pGetModuleHandle == NULL) {
			return 1;
		}

		info.pGetProcAddress = (FpGetProcAddress)GetProcAddress(hMod, "GetProcAddress");
		if (info.pGetProcAddress == NULL) {
			return 1;
		}

		info.pOpenProcess = (FpOpenProcess)GetProcAddress(hMod, "OpenProcess");
		if (info.pOpenProcess == NULL) {
			return 1;
		}

		info.pCloseHandle = (FpCloseHandle)GetProcAddress(hMod, "CloseHandle");
		if (info.pCloseHandle == NULL) {
			return 1;
		}
	}

	if ((hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID)) == NULL) {
		ret = 1;
		goto exit;
	}

	remoteMem = VirtualAllocEx(hProcess, NULL, memSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (remoteMem == NULL) {
		ret = 2;
		err = GetLastError();
		goto exit;
	}

	wFlag = WriteProcessMemory(hProcess, remoteMem, (char*)invokeFunc, memSize, (SIZE_T*)0);
	if (wFlag == FALSE) {
		ret = 3;
		goto exit;
	}

	remoteInfo = VirtualAllocEx(hProcess, NULL, sizeof(info), MEM_COMMIT, PAGE_READWRITE);
	if (remoteInfo == NULL) {
		ret = 2;
		err = GetLastError();
		goto exit;
	}

	wFlag = WriteProcessMemory(hProcess, remoteInfo, (char*)&info, sizeof(info), (SIZE_T*)0);
	if (wFlag == FALSE) {
		ret = 3;
		goto exit;
	}

#if 0
	TCHAR buf[1024];

	_stprintf_s(buf, sizeof(buf)/sizeof(buf[0]),
		_T("execute UpdatePerUserSystemParameters(), inject code to winlogon.exe?\r\n")
		_T("invokeFunc=0x%p\r\n")
		_T("afterFunc=0x%p\r\n")
		_T("afterFunc - invokeFunc=%d\r\n")
		_T("remoteMem=0x%p\r\n")
		_T("remoteInfo=0x%p(size: %d)\r\n"),
		invokeFunc, afterFunc, memSize, remoteMem, remoteInfo, sizeof(info));
	if (MessageBox((HWND)NULL, buf, _T("upusp"), MB_OKCANCEL | MB_ICONSTOP) == IDCANCEL) {
		(info.pUpdate)(0, 1);
		goto exit;
	}
#endif

	HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, 
		(LPTHREAD_START_ROUTINE)remoteMem, remoteInfo, 0, NULL);
	if (hThread == NULL) {
		ret = 4;
		goto exit;
	}

	if (WaitForSingleObject(hThread, 5000) == WAIT_TIMEOUT) {
		ret = 5;
		goto exit;
	}
	CloseHandle(hThread);

exit:
	if (remoteMem != NULL) {
		VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
	}

	if (remoteInfo != NULL) {
		VirtualFreeEx(hProcess, remoteInfo, 0, MEM_RELEASE);
	}

	if (hProcess != NULL) {
		CloseHandle(hProcess);
	}

	return ret;
}

int FixScancodeMap::update()
{
	MINIMIZEDMETRICS mm;
	int result = 0;

	if (acquirePrivileges()) {
		result = 1;
		goto exit;
	}

	DWORD dwPID;
	if ((dwPID = getWinLogonPid()) == 0) {
		result = 1;
		goto exit;
	}

	memset(&mm, 0, sizeof(mm));
	mm.cbSize = sizeof(mm);
	SystemParametersInfo(SPI_GETMINIMIZEDMETRICS, sizeof(mm), &mm, 0);

	if (injectThread(dwPID)) {
		result = 1;
		goto exit;
	}

	SystemParametersInfo(SPI_SETMINIMIZEDMETRICS, sizeof(mm), &mm, 0);

exit:
	return result;
}

int FixScancodeMap::fix()
{
	ScancodeMap *origMap, *fixMap;
	Registry reg(HKEY_CURRENT_USER, _T("Keyboard Layout"));
	DWORD origSize, fixSize;
	bool ret;
	int result = 0;

	// save original Scancode Map
	ret = reg.read(_T("Scancode Map"), NULL, &origSize, NULL, 0);
	if (ret) {
		origMap = reinterpret_cast<ScancodeMap*>(malloc(origSize));
		if (origMap == NULL) {
			result = 1;
			goto exit;
		}

		ret = reg.read(_T("Scancode Map"), reinterpret_cast<BYTE*>(origMap), &origSize, NULL, 0);
		if (ret == false) {
			result = 1;
			goto exit;
		}

		fixSize = origSize;
		fixMap = reinterpret_cast<ScancodeMap*>(malloc(origSize + s_fixEntryNum * sizeof(s_fixEntry[0])));
		if (fixMap == NULL) {
			result = 1;
			goto exit;
		}

		memcpy_s(fixMap, origSize + s_fixEntryNum, origMap, origSize);
	} else {
		origSize = 0;
		origMap = NULL;

		fixSize = sizeof(ScancodeMap);
		fixMap = reinterpret_cast<ScancodeMap*>(malloc(sizeof(ScancodeMap) + s_fixEntryNum * sizeof(s_fixEntry[0])));
		if (fixMap == NULL) {
			result = 1;
			goto exit;
		}

		fixMap->header1 = 0;
		fixMap->header2 = 0;
		fixMap->count = 1;
		fixMap->entry[0] = 0;
	}

	for (DWORD i = 0; i < s_fixEntryNum; i++) {
		bool skip = false;

		if (origMap) {
			for (DWORD j = 0; j < origMap->count; j++) {
				if (HIWORD(s_fixEntry[i]) == HIWORD(origMap->entry[j])) {
					skip = true;
				}
			}
		}

		if (skip) {
			// s_fixEntry[i] found in original Scancode Map, so don't fix it
			continue;
		}

		// add fix entry to fixMap
		fixMap->entry[fixMap->count - 1] = s_fixEntry[i];
		fixMap->entry[fixMap->count] = 0;
		fixMap->count++;
		fixSize += 4;
	}

	ret = reg.write(_T("Scancode Map"), reinterpret_cast<BYTE*>(fixMap), fixSize);
	if (ret == false) {
		result = 1;
		goto exit;
	}

	result = update();

	if (origMap) {
		ret = reg.write(_T("Scancode Map"), reinterpret_cast<BYTE*>(origMap), origSize);
	} else {
		ret = reg.remove(_T("Scancode Map"));
	}
	if (ret == false) {
		result = 1;
		goto exit;
	}

exit:
	free(origMap);
	origMap = NULL;

	free(fixMap);
	fixMap = NULL;

	return result;
}

int FixScancodeMap::restore()
{
	return update();
}

const DWORD FixScancodeMap::s_fixEntryNum = 4;
const DWORD FixScancodeMap::s_fixEntry[] = {
	0x003ae03a,
	0x0029e029,
	0x0070e070,
	0x003b001e,
};

