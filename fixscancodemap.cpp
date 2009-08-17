#include "fixscancodemap.h"
#include "misc.h"
#include "windowstool.h"
#include <tchar.h>
#include <tlhelp32.h>

#pragma runtime_checks( "", off )
static DWORD invokeFunc(InjectInfo *info)
{
	BOOL ret;
	HANDLE hToken;
	HMODULE hAdvapi32;
	FpImpersonateLoggedOnUser pImpersonateLoggedOnUser;
	FpRevertToSelf pRevertToSelf;
	FpOpenProcessToken pOpenProcessToken;

	hAdvapi32 = info->pGetModuleHandle(info->advapi32_);

	pImpersonateLoggedOnUser = (FpImpersonateLoggedOnUser)info->pGetProcAddress(hAdvapi32, info->impersonateLoggedOnUser_);
	pRevertToSelf = (FpRevertToSelf)info->pGetProcAddress(hAdvapi32, info->revertToSelf_);
	pOpenProcessToken = (FpOpenProcessToken)info->pGetProcAddress(hAdvapi32, info->openProcessToken_);

	HANDLE hProcess = info->pOpenProcess(PROCESS_QUERY_INFORMATION, FALSE, info->pid_);
	if (hProcess == NULL) {
		return 1;
	}

	ret = pOpenProcessToken(hProcess, TOKEN_QUERY | TOKEN_DUPLICATE , &hToken);
	if (ret == FALSE) {
		return 2;
	}

	ret = pImpersonateLoggedOnUser(hToken);
	if (ret == FALSE) {
		return 3;
	}

	info->pUpdate(0, 1);

	ret = pRevertToSelf();
	if (ret == FALSE) {
		return 4;
	}

	info->pCloseHandle(hToken);
	info->pCloseHandle(hProcess);
	return 0;
}
static int afterFunc(int arg)
{
	// dummy operation
	// if this function empty, optimizer unify this with other empty functions.
	// following code avoid it.
	arg *= 710810; // non-sense operation
	return arg;
}
#pragma runtime_checks( "", restore )

const DWORD FixScancodeMap::s_fixEntryNum = 4;
const DWORD FixScancodeMap::s_fixEntry[] = {
	0x003ae03a,
	0x0029e029,
	0x0070e070,
	0x007be07b,
};

int FixScancodeMap::acquirePrivileges()
{
	int ret = 0;
	HANDLE hToken = NULL;

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken)) {
		ret = 5;
		goto exit;
	}

	LUID luid;
	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid)) {
		ret = 6;
		goto exit;
	}

	TOKEN_PRIVILEGES tk_priv;
	tk_priv.PrivilegeCount = 1;
	tk_priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	tk_priv.Privileges[0].Luid = luid;

	if (!AdjustTokenPrivileges(hToken, FALSE, &tk_priv, 0, NULL, NULL)) {
		ret = 7;
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

			if (ProcessIdToSessionId(pe.th32ProcessID, &sessionId) != FALSE) {
				if (sessionId == mySessionId) {
					pid = pe.th32ProcessID;
					break;
				}
			}
		}
		bResult = Process32Next(hSnap, &pe);
	}

	CloseHandle(hSnap);
	return pid;
}


int FixScancodeMap::clean()
{
	int ret = 0;

	if (WaitForSingleObject(m_hThread, 5000) == WAIT_TIMEOUT) {
		ret = 15;
		goto dirty_exit;
	}
	DWORD result = -1;
	GetExitCodeThread(m_hThread, &result);
	ret = result;
	CloseHandle(m_hThread);
	m_hThread = NULL;

	if (m_remoteMem != NULL && m_hProcess != NULL) {
		VirtualFreeEx(m_hProcess, m_remoteMem, 0, MEM_RELEASE);
		m_remoteMem = NULL;
	}

	if (m_remoteInfo != NULL && m_hProcess != NULL) {
		VirtualFreeEx(m_hProcess, m_remoteInfo, 0, MEM_RELEASE);
		m_remoteInfo = NULL;
	}

	if (m_hProcess != NULL) {
		CloseHandle(m_hProcess);
		m_hProcess = NULL;
	}

dirty_exit:
	return ret;
}


int FixScancodeMap::injectThread(DWORD dwPID)
{
	int ret = 0;
	DWORD err = 0;
	BOOL wFlag;

	DWORD invokeFuncAddr = (DWORD)invokeFunc;
	DWORD afterFuncAddr = (DWORD)afterFunc;
	DWORD memSize =  afterFuncAddr - invokeFuncAddr;

	if (m_hThread != NULL) {
		ret = clean();
		if (ret) {
			return ret;
		}
	}

	if ((m_hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID)) == NULL) {
		ret = 8;
		goto exit;
	}

	m_remoteMem = VirtualAllocEx(m_hProcess, NULL, memSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (m_remoteMem == NULL) {
		ret = 9;
		err = GetLastError();
		goto exit;
	}

	wFlag = WriteProcessMemory(m_hProcess, m_remoteMem, (char*)invokeFunc, memSize, (SIZE_T*)0);
	if (wFlag == FALSE) {
		ret = 10;
		goto exit;
	}

	m_remoteInfo = VirtualAllocEx(m_hProcess, NULL, sizeof(m_info), MEM_COMMIT, PAGE_READWRITE);
	if (m_remoteInfo == NULL) {
		ret = 11;
		err = GetLastError();
		goto exit;
	}

	wFlag = WriteProcessMemory(m_hProcess, m_remoteInfo, (char*)&m_info, sizeof(m_info), (SIZE_T*)0);
	if (wFlag == FALSE) {
		ret = 12;
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
		invokeFunc, afterFunc, memSize, m_remoteMem, m_remoteInfo, sizeof(m_info));
	if (MessageBox((HWND)NULL, buf, _T("upusp"), MB_OKCANCEL | MB_ICONSTOP) == IDCANCEL) {
		(m_info.pUpdate)(0, 1);
		goto exit;
	}
#endif

	m_hThread = CreateRemoteThread(m_hProcess, NULL, 0, 
		(LPTHREAD_START_ROUTINE)m_remoteMem, m_remoteInfo, 0, NULL);
	if (m_hThread == NULL) {
		ret = 13;
		goto exit;
	}

	if (WaitForSingleObject(m_hThread, 5000) == WAIT_TIMEOUT) {
		ret = 14;
		goto dirty_exit;
	}
	DWORD result = -1;
	GetExitCodeThread(m_hThread, &result);
	ret = result;
	CloseHandle(m_hThread);
	m_hThread = NULL;

exit:
	if (m_remoteMem != NULL && m_hProcess != NULL) {
		VirtualFreeEx(m_hProcess, m_remoteMem, 0, MEM_RELEASE);
		m_remoteMem = NULL;
	}

	if (m_remoteInfo != NULL && m_hProcess != NULL) {
		VirtualFreeEx(m_hProcess, m_remoteInfo, 0, MEM_RELEASE);
		m_remoteInfo = NULL;
	}

	if (m_hProcess != NULL) {
		CloseHandle(m_hProcess);
		m_hProcess = NULL;
	}

dirty_exit:
	return ret;
}

int FixScancodeMap::update()
{
	MINIMIZEDMETRICS mm;
	int result = 0;

	result = acquirePrivileges();
	if (result) {
		goto exit;
	}

	DWORD dwPID;
	if ((dwPID = getWinLogonPid()) == 0) {
		result = 15;
		goto exit;
	}

	memset(&mm, 0, sizeof(mm));
	mm.cbSize = sizeof(mm);
	SystemParametersInfo(SPI_GETMINIMIZEDMETRICS, sizeof(mm), &mm, 0);

	result = injectThread(dwPID);
	if (result && m_hThread == NULL) {
		goto exit;
	}

	SystemParametersInfo(SPI_SETMINIMIZEDMETRICS, sizeof(mm), &mm, 0);

exit:
	return result;
}

int FixScancodeMap::fix()
{
	ScancodeMap *origMap, *fixMap;
	DWORD origSize, fixSize;
	bool ret;
	int result = 0;

	// save original Scancode Map
	ret = m_pReg->read(_T("Scancode Map"), NULL, &origSize, NULL, 0);
	if (ret) {
		origMap = reinterpret_cast<ScancodeMap*>(malloc(origSize));
		if (origMap == NULL) {
			result = 16;
			goto exit;
		}

		ret = m_pReg->read(_T("Scancode Map"), reinterpret_cast<BYTE*>(origMap), &origSize, NULL, 0);
		if (ret == false) {
			result = 17;
			goto exit;
		}

		fixSize = origSize;
		fixMap = reinterpret_cast<ScancodeMap*>(malloc(origSize + s_fixEntryNum * sizeof(s_fixEntry[0])));
		if (fixMap == NULL) {
			result = 18;
			goto exit;
		}

		memcpy_s(fixMap, origSize + s_fixEntryNum, origMap, origSize);
	} else {
		origSize = 0;
		origMap = NULL;

		fixSize = sizeof(ScancodeMap);
		fixMap = reinterpret_cast<ScancodeMap*>(malloc(sizeof(ScancodeMap) + s_fixEntryNum * sizeof(s_fixEntry[0])));
		if (fixMap == NULL) {
			result = 19;
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

	ret = m_pReg->write(_T("Scancode Map"), reinterpret_cast<BYTE*>(fixMap), fixSize);
	if (ret == false) {
		result = 20;
		goto exit;
	}

	result = update();

	if (origMap) {
		ret = m_pReg->write(_T("Scancode Map"), reinterpret_cast<BYTE*>(origMap), origSize);
	} else {
		ret = m_pReg->remove(_T("Scancode Map"));
	}
	if (ret == false) {
		result = 21;
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

FixScancodeMap::FixScancodeMap() :
	m_hProcess(NULL),
	m_remoteMem(NULL),
	m_remoteInfo(NULL),
	m_hThread(NULL),
	m_regHKCU(HKEY_CURRENT_USER, _T("Keyboard Layout")),
	m_regHKLM(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Control\\Keyboard Layout")),
	m_pReg(NULL)
{
	HMODULE hMod;

	m_info.pid_ = GetCurrentProcessId();

	memcpy(&m_info.advapi32_, _T("advapi32.dll"), sizeof(m_info.advapi32_));
	memcpy(&m_info.impersonateLoggedOnUser_, "ImpersonateLoggedOnUser", sizeof(m_info.impersonateLoggedOnUser_));
	memcpy(&m_info.revertToSelf_, "RevertToSelf", sizeof(m_info.revertToSelf_));
	memcpy(&m_info.openProcessToken_, "OpenProcessToken", sizeof(m_info.openProcessToken_));

	hMod = GetModuleHandle(_T("user32.dll"));
	if (hMod != NULL) {
		m_info.pUpdate = (FpUpdatePerUserSystemParameters)GetProcAddress(hMod, "UpdatePerUserSystemParameters");
		if (m_info.pUpdate == NULL) {
			return;
		}
	}

	hMod = GetModuleHandle(_T("kernel32.dll"));
	if (hMod != NULL) {
		m_info.pGetModuleHandle = (FpGetModuleHandleW)GetProcAddress(hMod, "GetModuleHandleW");
		if (m_info.pGetModuleHandle == NULL) {
			return;
		}

		m_info.pGetProcAddress = (FpGetProcAddress)GetProcAddress(hMod, "GetProcAddress");
		if (m_info.pGetProcAddress == NULL) {
			return;
		}

		m_info.pOpenProcess = (FpOpenProcess)GetProcAddress(hMod, "OpenProcess");
		if (m_info.pOpenProcess == NULL) {
			return;
		}

		m_info.pCloseHandle = (FpCloseHandle)GetProcAddress(hMod, "CloseHandle");
		if (m_info.pCloseHandle == NULL) {
			return;
		}
	}

	// Windows7 RC not support Scancode Map on HKCU?
	if (checkWindowsVersion(6, 1) == FALSE) {
		m_pReg = &m_regHKCU; // Vista or earlier
	} else {
		m_pReg = &m_regHKLM; // Windows7 or later
	}
}

FixScancodeMap::~FixScancodeMap()
{
}
