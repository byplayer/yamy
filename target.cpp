//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// target.cpp


#include "misc.h"

#include "mayurc.h"
#include "target.h"
#include "windowstool.h"


///
class Target
{
  HWND m_hwnd;					///
  HWND m_preHwnd;				///
  HICON m_hCursor;				///

  ///
  static void invertFrame(HWND i_hwnd)
  {
    HDC hdc = GetWindowDC(i_hwnd);
    ASSERT(hdc);
    int rop2 = SetROP2(hdc, R2_XORPEN);
    if (rop2)
    {
      RECT rc;
      CHECK_TRUE( GetWindowRect(i_hwnd, &rc) );
      int width = rcWidth(&rc);
      int height = rcHeight(&rc);
    
      HANDLE hpen = SelectObject(hdc, GetStockObject(WHITE_PEN));
      HANDLE hbr  = SelectObject(hdc, GetStockObject(NULL_BRUSH));
      CHECK_TRUE( Rectangle(hdc, 0, 0, width    , height    ) );
      CHECK_TRUE( Rectangle(hdc, 1, 1, width - 1, height - 1) );
      CHECK_TRUE( Rectangle(hdc, 2, 2, width - 2, height - 2) );
      SelectObject(hdc, hpen);
      SelectObject(hdc, hbr);
      // no need to DeleteObject StockObject
      SetROP2(hdc, rop2);
    }
    CHECK_TRUE( ReleaseDC(i_hwnd, hdc) );
  }
  
  ///
  Target(HWND i_hwnd)
    : m_hwnd(i_hwnd),
      m_preHwnd(NULL),
      m_hCursor(NULL)
  {
  }

  /// WM_CREATE
  int wmCreate(CREATESTRUCT * /* i_cs */)
  {
    CHECK_TRUE( m_hCursor =
		LoadCursor(g_hInst, MAKEINTRESOURCE(IDC_CURSOR_target)) );
    return 0;
  }

  /// WM_PAINT
  int wmPaint()
  {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(m_hwnd, &ps);
    ASSERT(hdc);

    if (GetCapture() != m_hwnd)
    {
      RECT rc;
      CHECK_TRUE( GetClientRect(m_hwnd, &rc) );
      CHECK_TRUE(
	DrawIcon(hdc, (rcWidth(&rc) - GetSystemMetrics(SM_CXICON)) / 2,
		 (rcHeight(&rc) - GetSystemMetrics(SM_CYICON)) / 2,
		 m_hCursor) );
    }
    
    EndPaint(m_hwnd, &ps);
    return 0;
  }

  ///
  struct PointWindow
  {
    POINT m_p;					///
    HWND m_hwnd;				///
    RECT m_rc;					///
  };
  
  ///
  static BOOL CALLBACK childWindowFromPoint(HWND i_hwnd, LPARAM i_lParam)
  {
    if (IsWindowVisible(i_hwnd))
    {
      PointWindow &pw = *(PointWindow *)i_lParam;
      RECT rc;
      CHECK_TRUE( GetWindowRect(i_hwnd, &rc) );
      if (PtInRect(&rc, pw.m_p))
	if (isRectInRect(&rc, &pw.m_rc))
	{
	  pw.m_hwnd = i_hwnd;
	  pw.m_rc = rc;
	}
    }
    return TRUE;
  }
  
  ///
  static BOOL CALLBACK windowFromPoint(HWND i_hwnd, LPARAM i_lParam)
  {
    if (IsWindowVisible(i_hwnd))
    {
      PointWindow &pw = *(PointWindow *)i_lParam;
      RECT rc;
      CHECK_TRUE( GetWindowRect(i_hwnd, &rc) );
      if (PtInRect(&rc, pw.m_p))
      {
	pw.m_hwnd = i_hwnd;
	pw.m_rc = rc;
	return FALSE;
      }
    }
    return TRUE;
  }

  /// WM_MOUSEMOVE
  int wmMouseMove(WORD /* i_keys */, int /* i_x */, int /* i_y */)
  {
    if (GetCapture() == m_hwnd)
    {
      PointWindow pw;
      CHECK_TRUE( GetCursorPos(&pw.m_p) );
      pw.m_hwnd = 0;
      CHECK_TRUE( GetWindowRect(GetDesktopWindow(), &pw.m_rc) );
      EnumWindows(windowFromPoint, (LPARAM)&pw);
      while (1)
      {
	HWND hwndParent = pw.m_hwnd;
	if (!EnumChildWindows(pw.m_hwnd, childWindowFromPoint, (LPARAM)&pw))
	  break;
	if (hwndParent == pw.m_hwnd)
	  break;
      }
      if (pw.m_hwnd != m_preHwnd)
      {
	if (m_preHwnd)
	  invertFrame(m_preHwnd);
	m_preHwnd = pw.m_hwnd;
	invertFrame(m_preHwnd);
	SendMessage(GetParent(m_hwnd), WM_APP_targetNotify, 0,
		    (LPARAM)m_preHwnd);
      }
      SetCursor(m_hCursor);
    }
    return 0;
  }

  /// WM_LBUTTONDOWN
  int wmLButtonDown(WORD /* i_keys */, int /* i_x */, int /* i_y */)
  {
    SetCapture(m_hwnd);
    SetCursor(m_hCursor);
    CHECK_TRUE( InvalidateRect(m_hwnd, NULL, TRUE) );
    CHECK_TRUE( UpdateWindow(m_hwnd) );
    return 0;
  }

  /// WM_LBUTTONUP
  int wmLButtonUp(WORD /* i_keys */, int /* i_x */, int /* i_y */)
  {
    if (m_preHwnd)
      invertFrame(m_preHwnd);
    m_preHwnd = NULL;
    ReleaseCapture();
    CHECK_TRUE( InvalidateRect(m_hwnd, NULL, TRUE) );
    CHECK_TRUE( UpdateWindow(m_hwnd) );
    return 0;
  }

public:
  ///
  static LRESULT CALLBACK WndProc(HWND i_hwnd, UINT i_message,
				  WPARAM i_wParam, LPARAM i_lParam)
  {
    Target *wc;
    getUserData(i_hwnd, &wc);
    if (!wc)
      switch (i_message)
      {
	case WM_CREATE:
	  wc = setUserData(i_hwnd, new Target(i_hwnd));
	  return wc->wmCreate((CREATESTRUCT *)i_lParam);
      }
    else 
      switch (i_message)
      {
	case WM_PAINT:
	  return wc->wmPaint();
	case WM_LBUTTONDOWN:
	  return wc->wmLButtonDown((WORD)i_wParam, (short)LOWORD(i_lParam),
				   (short)HIWORD(i_lParam));
	case WM_LBUTTONUP:
	  return wc->wmLButtonUp((WORD)i_wParam, (short)LOWORD(i_lParam),
				 (short)HIWORD(i_lParam));
	case WM_MOUSEMOVE:
	  return wc->wmMouseMove((WORD)i_wParam, (short)LOWORD(i_lParam),
				 (short)HIWORD(i_lParam));
	case WM_NCDESTROY:
	  delete wc;
	  return 0;
      }
    return DefWindowProc(i_hwnd, i_message, i_wParam, i_lParam);
  }
};
  

//
ATOM Register_target()
{
  WNDCLASS wc;
  wc.style         = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc   = Target::WndProc;
  wc.cbClsExtra    = 0;
  wc.cbWndExtra    = 0;
  wc.hInstance     = g_hInst;
  wc.hIcon         = NULL;
  wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  wc.lpszMenuName  = NULL;
  wc.lpszClassName = _T("mayuTarget");
  return RegisterClass(&wc);
}
