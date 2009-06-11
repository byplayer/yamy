#include <windows.h>
#include <process.h>
#include <tchar.h>
#include "../driver.h"
#ifdef STS4MAYU
#include "SynKit.h"
#pragma comment(lib, "SynCom.lib")
#endif /* STS4MAYU */
#ifdef CTS4MAYU
#include "Touchpad.h"
#pragma comment(lib, "TouchPad.lib")
#endif /* CTS4MAYU */

static HANDLE s_instance;
static UINT s_engineThreadId;

#ifdef STS4MAYU
static ISynAPI *s_synAPI;
static ISynDevice *s_synDevice;
static HANDLE s_notifyEvent;

static int s_terminated;
static HANDLE s_loopThread;
static unsigned int s_loopThreadId;
#endif /* STS4MAYU */
#ifdef CTS4MAYU
static HTOUCHPAD s_hTP[16];
static int s_devNum;
#endif /* CTS4MAYU */

static void changeTouch(int i_isBreak)
{
  PostThreadMessage(s_engineThreadId, WM_APP + 201, i_isBreak ? 0 : 1, 0);
}

static void postEvent(WPARAM wParam, LPARAM lParam)
{
  PostThreadMessage(s_engineThreadId, WM_APP + 201, wParam, lParam);
}

#ifdef STS4MAYU
static unsigned int WINAPI loop(void *dummy)
{
  HRESULT result;
  SynPacket packet;
  int isTouched = 0;

  while (s_terminated == 0) {
    WaitForSingleObject(s_notifyEvent, INFINITE);
    if (s_terminated) {
      break;
    }

    for (;;) {
      long value;

      result = s_synAPI->GetEventParameter(&value);
      if (result != SYN_OK) {
	break;
      }
      if (value == SE_Configuration_Changed) {
	s_synDevice->SetEventNotification(s_notifyEvent);
      }
    }

    for (;;) {
      result = s_synDevice->LoadPacket(packet);
      if (result == SYNE_FAIL) {
	break;
      }

      if (isTouched) {
	if (!(packet.FingerState() & SF_FingerTouch)) {
	  changeTouch(1);
	  isTouched = 0;
	}
      } else {
	if (packet.FingerState() & SF_FingerTouch) {
	  changeTouch(0);
	  isTouched = 1;
	}
      }
    }
  }
  _endthreadex(0);
  return 0;
}
#endif /* STS4MAYU */
#ifdef CTS4MAYU
static void CALLBACK TouchpadFunc(HTOUCHPAD hTP, LPFEEDHDR lpFeedHdr, LPARAM i_lParam)
{
	LPRAWFEED lpRawFeed;
	static int isTouched = 0;
	static WPARAM s_wParam;
	static LPARAM s_lParam;
	WPARAM wParam;
	LPARAM lParam;

	lpRawFeed = (LPRAWFEED)(lpFeedHdr + 1);
#if 1
	wParam = lpRawFeed->wPressure;
	lParam = lpRawFeed->y << 16 | lpRawFeed->x;
	if (wParam != s_wParam || lParam != s_lParam) {
	  postEvent(wParam, lParam);
	  s_wParam = wParam;
	  s_lParam = lParam;
	}
#else
	if (isTouched) {
	  if (!lpRawFeed->wPressure) {
	    changeTouch(1);
	    isTouched = 0;
	  }
	} else {
	  if (lpRawFeed->wPressure) {
	    changeTouch(0);
	    isTouched = 1;
	  }
	}
#endif
	EnableWindowsCursor(hTP, TRUE);
}

static BOOL CALLBACK DevicesFunc(LPGENERICDEVICE device, LPARAM lParam)
{
	HTOUCHPAD hTP = NULL;
	BOOL ret = FALSE;

	s_hTP[s_devNum] = GetPad(device->devicePort);
	CreateCallback(s_hTP[s_devNum], TouchpadFunc,
		       TPF_RAW | TPF_POSTMESSAGE, NULL);
	StartFeed(s_hTP[s_devNum]);
	++s_devNum;
	return TRUE;
}
#endif /* CTS4MAYU */

bool WINAPI ts4mayuInit(UINT i_engineThreadId)
{
  s_engineThreadId = i_engineThreadId;

#ifdef STS4MAYU
  HRESULT result;
  long hdl;

  s_synAPI = NULL;
  s_synDevice = NULL;
  s_notifyEvent = NULL;

  s_terminated = 0;

  result = SynCreateAPI(&s_synAPI);
  if (result != SYN_OK) {
    goto error_on_init;
  }

  hdl = -1;
  result = s_synAPI->FindDevice(SE_ConnectionAny, SE_DeviceTouchPad, &hdl);
  if (result != SYN_OK) {
    goto error_on_init;
  }

  result = s_synAPI->CreateDevice(hdl, &s_synDevice);
  if (result != SYN_OK) {
    goto error_on_init;
  }

  s_notifyEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
  if (s_notifyEvent == NULL) {
    goto error_on_init;
  }

  s_synAPI->SetEventNotification(s_notifyEvent);
  s_synDevice->SetEventNotification(s_notifyEvent);

  s_loopThread =
    (HANDLE)_beginthreadex(NULL, 0, loop, NULL, 0, &s_loopThreadId);
  if (s_loopThread == 0) {
    goto error_on_init;
  }

  return true;

error_on_init:
  if (s_notifyEvent) {
    CloseHandle(s_notifyEvent);
  }

  if (s_synDevice) {
    s_synDevice->Release();
  }

  if (s_synAPI) {
    s_synAPI->Release();
  }

  return false;
#endif /* STS4MAYU */
#ifdef CTS4MAYU
  // enumerate devices
  EnumDevices(DevicesFunc, NULL);
  return true;
#endif /* CTS4MAYU */
}


bool WINAPI ts4mayuTerm()
{
#ifdef STS4MAYU
  s_terminated = 1;

  if (s_loopThread) {
    SetEvent(s_notifyEvent);
    WaitForSingleObject(s_loopThread, INFINITE);
    CloseHandle(s_loopThread);
  }

  if (s_notifyEvent) {
    CloseHandle(s_notifyEvent);
  }

  if (s_synDevice) {
    s_synDevice->Release();
  }

  if (s_synAPI) {
    s_synAPI->Release();
  }

  return true;
#endif /* STS4MAYU */
#ifdef CTS4MAYU
  for (int i = 0; i < s_devNum; i++) {
    StopFeed(s_hTP[i]);
  }
  return false;
#endif /* CTS4MAYU */
}


BOOL WINAPI DllMain(HANDLE module, DWORD reason, LPVOID reserve)
{
    s_instance = (HINSTANCE)module;

    return TRUE;
}
