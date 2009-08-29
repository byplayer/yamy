//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// yamyd.cpp

#include "mayu.h"
#include "hook.h"

/// main
int WINAPI _tWinMain(HINSTANCE /* i_hInstance */, HINSTANCE /* i_hPrevInstance */,
					 LPTSTR /* i_lpszCmdLine */, int /* i_nCmdShow */)
{
	HANDLE mutex = OpenMutex(SYNCHRONIZE, FALSE, MUTEX_YAMYD_BLOCKER);
	if (mutex != NULL) {
		CHECK_FALSE( installMessageHook() );

		// wait for master process exit
		WaitForSingleObject(mutex, INFINITE);
		ReleaseMutex(mutex);

		CHECK_FALSE( uninstallMessageHook() );
	}

	return 0;
}
