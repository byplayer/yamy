#include <windows.h>

class FixScancodeMap {
	typedef struct {
		DWORD header1;
		DWORD header2;
		DWORD count;
		DWORD entry[1];
	} ScancodeMap;

	static const DWORD s_fixEntryNum;
	static const DWORD s_fixEntry[];

	int acquirePrivileges();
	DWORD getWinLogonPid();
	int injectThread(DWORD dwPID);
	int update();

public:
	FixScancodeMap() {}
	~FixScancodeMap() {}

	int fix();
	int restore();
};
