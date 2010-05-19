#include <list>
#include <windows.h>
#include "registry.h"

typedef HMODULE (WINAPI *FpGetModuleHandleW)(LPCWSTR);
typedef FARPROC (WINAPI *FpGetProcAddress)(HMODULE, LPCSTR);
typedef BOOL (WINAPI *FpUpdatePerUserSystemParameters4)(BOOL);
typedef BOOL (WINAPI *FpUpdatePerUserSystemParameters8)(DWORD, BOOL);
typedef HANDLE (WINAPI *FpOpenProcess)(DWORD, BOOL, DWORD);
typedef BOOL (WINAPI *FpOpenProcessToken)(HANDLE, DWORD, PHANDLE);
typedef BOOL (WINAPI *FpImpersonateLoggedOnUser)(HANDLE);
typedef BOOL (WINAPI *FpRevertToSelf)(VOID);
typedef BOOL (WINAPI *FpCloseHandle)(HANDLE);

typedef struct {
	DWORD isVistaOrLater_;
	DWORD pid_;
	TCHAR advapi32_[64];
	CHAR impersonateLoggedOnUser_[32];
	CHAR revertToSelf_[32];
	CHAR openProcessToken_[32];
	FpGetModuleHandleW pGetModuleHandle;
	FpGetProcAddress pGetProcAddress;
	FpUpdatePerUserSystemParameters4 pUpdate4;
	FpUpdatePerUserSystemParameters8 pUpdate8;
	FpOpenProcess pOpenProcess;
	FpCloseHandle pCloseHandle;
} InjectInfo;

class FixScancodeMap {
private:
	typedef struct {
		DWORD header1;
		DWORD header2;
		DWORD count;
		DWORD entry[1];
	} ScancodeMap;

	typedef struct {
		HANDLE m_hProcess;
		LPVOID m_remoteMem;
		LPVOID m_remoteInfo;
		HANDLE m_hThread;
	} WlInfo;

private:
	static const DWORD s_fixEntryNum;
	static const DWORD s_fixEntry[];

private:
	HWND m_hwnd;
	UINT m_messageOnFail;
	int m_errorOnConstruct;
	DWORD m_winlogonPid;
	std::list<WlInfo> m_wlTrash;
	InjectInfo m_info;
	Registry m_regHKCU;
	Registry m_regHKLM;
	Registry *m_pReg;
	HANDLE m_hFixEvent;
	HANDLE m_hRestoreEvent;
	HANDLE m_hQuitEvent;
	HANDLE m_hThread;
	unsigned m_threadId;

private:
	int acquirePrivileges();
	DWORD getWinLogonPid();
	static bool clean(WlInfo wl);
	int injectThread(DWORD dwPID);
	int update();
	int fix();
	int restore();
	static unsigned int WINAPI threadLoop(void *i_this);

public:
	FixScancodeMap();
	~FixScancodeMap();

	int init(HWND i_hwnd, UINT i_messageOnFail);
	int escape(bool i_escape);
};
