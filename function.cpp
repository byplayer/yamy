//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// function.cpp


#include "engine.h"
#include "hook.h"
#include "mayu.h"
#include "mayurc.h"
#include "misc.h"
#include "registry.h"
#include "vkeytable.h"
#include "windowstool.h"
#include <algorithm>
#include <process.h>

#define FUNCTION_DATA
#include "functions.h"
#undef FUNCTION_DATA


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// TypeTable


template <class T> class TypeTable
{
public:
  T m_type;
  const _TCHAR *m_name;
};


template <class T> static inline
bool getTypeName(tstring *o_name, T i_type,
		 const TypeTable<T> *i_table, size_t i_n)
{
  for (size_t i = 0; i < i_n; ++ i)
    if (i_table[i].m_type == i_type)
    {
      *o_name = i_table[i].m_name;
      return true;
    }
  return false;
}

template <class T> static inline
bool getTypeValue(T *o_type, const tstringi &i_name,
		  const TypeTable<T> *i_table, size_t i_n)
{
  for (size_t i = 0; i < i_n; ++ i)
    if (i_table[i].m_name == i_name)
    {
      *o_type = i_table[i].m_type;
      return true;
    }
  return false;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// VKey


// stream output
tostream &operator<<(tostream &i_ost, VKey i_data)
{
  if (i_data & VKey_extended)
    i_ost << _T("E-");
  if (i_data & VKey_released)
    i_ost << _T("U-");
  if (i_data & VKey_pressed)
    i_ost << _T("D-");

  u_int8 code = i_data & ~(VKey_extended | VKey_released | VKey_pressed);
  const VKeyTable *vkt;
  for (vkt = g_vkeyTable; vkt->m_name; ++ vkt)
    if (vkt->m_code == code)
      break;
  if (vkt->m_name)
    i_ost << vkt->m_name;
  else
    i_ost << _T("0x") << std::hex << code << std::dec;
  return i_ost;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ToWindowType


// ToWindowType table
typedef TypeTable<ToWindowType> TypeTable_ToWindowType;
static const TypeTable_ToWindowType g_toWindowTypeTable[] =
{
  { ToWindowType_toOverlappedWindow, _T("toOverlappedWindow") },
  { ToWindowType_toMainWindow,       _T("toMainWindow")       },
  { ToWindowType_toItself,           _T("toItself")           },
  { ToWindowType_toParentWindow,     _T("toParentWindow")     },
};


// stream output
tostream &operator<<(tostream &i_ost, ToWindowType i_data)
{
  tstring name;
  if (getTypeName(&name, i_data,
		  g_toWindowTypeTable, NUMBER_OF(g_toWindowTypeTable)))
    i_ost << name;
  else
    i_ost << static_cast<int>(i_data);
  return i_ost;
}


// get value of ToWindowType
bool getTypeValue(ToWindowType *o_type, const tstring &i_name)
{
  return getTypeValue(o_type, i_name,
		      g_toWindowTypeTable, NUMBER_OF(g_toWindowTypeTable));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// GravityType


// GravityType table
typedef TypeTable<GravityType> TypeTable_GravityType;
static const TypeTable_GravityType g_gravityTypeTable[] =
{
  { GravityType_C,  _T("C")  },
  { GravityType_N,  _T("N")  },
  { GravityType_E,  _T("E")  },
  { GravityType_W,  _T("W")  },
  { GravityType_S,  _T("S")  },
  { GravityType_NW, _T("NW") },
  { GravityType_NW, _T("WN") },
  { GravityType_NE, _T("NE") },
  { GravityType_NE, _T("EN") },
  { GravityType_SW, _T("SW") },
  { GravityType_SW, _T("WS") },
  { GravityType_SE, _T("SE") },
  { GravityType_SE, _T("ES") },
};


// stream output
tostream &operator<<(tostream &i_ost, GravityType i_data)
{
  tstring name;
  if (getTypeName(&name, i_data,
		  g_gravityTypeTable, NUMBER_OF(g_gravityTypeTable)))
    i_ost << name;
  else
    i_ost << _T("(GravityType internal error)");
  return i_ost;
}


// get value of GravityType
bool getTypeValue(GravityType *o_type, const tstring &i_name)
{
  return getTypeValue(o_type, i_name,
		      g_gravityTypeTable, NUMBER_OF(g_gravityTypeTable));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// MouseHookType


// MouseHookType table
typedef TypeTable<MouseHookType> TypeTable_MouseHookType;
static const TypeTable_MouseHookType g_mouseHookTypeTable[] =
{
  { MouseHookType_None,  _T("None")  },
  { MouseHookType_Wheel,  _T("Wheel")  },
  { MouseHookType_WindowMove,  _T("WindowMove")  },
};


// stream output
tostream &operator<<(tostream &i_ost, MouseHookType i_data)
{
  tstring name;
  if (getTypeName(&name, i_data,
		  g_mouseHookTypeTable, NUMBER_OF(g_mouseHookTypeTable)))
    i_ost << name;
  else
    i_ost << _T("(MouseHookType internal error)");
  return i_ost;
}


// get value of MouseHookType
bool getTypeValue(MouseHookType *o_type, const tstring &i_name)
{
  return getTypeValue(o_type, i_name, g_mouseHookTypeTable,
		      NUMBER_OF(g_mouseHookTypeTable));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// MayuDialogType


// ModifierLockType table
typedef TypeTable<MayuDialogType> TypeTable_MayuDialogType;
static const TypeTable_MayuDialogType g_mayuDialogTypeTable[] =
{
  { MayuDialogType_investigate, _T("investigate")  },
  { MayuDialogType_log,         _T("log")          },
};


// stream output
tostream &operator<<(tostream &i_ost, MayuDialogType i_data)
{
  tstring name;
  if (getTypeName(&name, i_data,
		  g_mayuDialogTypeTable, NUMBER_OF(g_mayuDialogTypeTable)))
    i_ost << name;
  else
    i_ost << _T("(MayuDialogType internal error)");
  return i_ost;
}


// get value of MayuDialogType
bool getTypeValue(MayuDialogType *o_type, const tstring &i_name)
{
  return getTypeValue(o_type, i_name, g_mayuDialogTypeTable,
		      NUMBER_OF(g_mayuDialogTypeTable));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ToggleType


// ToggleType table
typedef TypeTable<ToggleType> TypeTable_ToggleType;
static const TypeTable_ToggleType g_toggleType[] =
{
  { ToggleType_toggle, _T("toggle") },
  { ToggleType_off, _T("off") },
  { ToggleType_off, _T("false") },
  { ToggleType_off, _T("released") },
  { ToggleType_on,  _T("on")  },
  { ToggleType_on,  _T("true")  },
  { ToggleType_on,  _T("pressed")  },
};


// stream output
tostream &operator<<(tostream &i_ost, ToggleType i_data)
{
  tstring name;
  if (getTypeName(&name, i_data, g_toggleType, NUMBER_OF(g_toggleType)))
    i_ost << name;
  else
    i_ost << _T("(ToggleType internal error)");
  return i_ost;
}


// get value of ToggleType
bool getTypeValue(ToggleType *o_type, const tstring &i_name)
{
  return getTypeValue(o_type, i_name, g_toggleType,
		      NUMBER_OF(g_toggleType));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ModifierLockType


// ModifierLockType table
typedef TypeTable<ModifierLockType> TypeTable_ModifierLockType;
static const TypeTable_ModifierLockType g_modifierLockTypeTable[] =
{
  { ModifierLockType_Lock0, _T("lock0") },
  { ModifierLockType_Lock1, _T("lock1") },
  { ModifierLockType_Lock2, _T("lock2") },
  { ModifierLockType_Lock3, _T("lock3") },
  { ModifierLockType_Lock4, _T("lock4") },
  { ModifierLockType_Lock5, _T("lock5") },
  { ModifierLockType_Lock6, _T("lock6") },
  { ModifierLockType_Lock7, _T("lock7") },
  { ModifierLockType_Lock8, _T("lock8") },
  { ModifierLockType_Lock9, _T("lock9") },
};


// stream output
tostream &operator<<(tostream &i_ost, ModifierLockType i_data)
{
  tstring name;
  if (getTypeName(&name, i_data,
		  g_modifierLockTypeTable, NUMBER_OF(g_modifierLockTypeTable)))
    i_ost << name;
  else
    i_ost << _T("(ModifierLockType internal error)");
  return i_ost;
}


// get value of ModifierLockType
bool getTypeValue(ModifierLockType *o_type, const tstring &i_name)
{
  return getTypeValue(o_type, i_name, g_modifierLockTypeTable,
		      NUMBER_OF(g_modifierLockTypeTable));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ShowCommandType


// ShowCommandType table
typedef TypeTable<ShowCommandType> TypeTable_ShowCommandType;
static const TypeTable_ShowCommandType g_showCommandTypeTable[] =
{
  { ShowCommandType_hide,            _T("hide")            },
  { ShowCommandType_maximize,        _T("maximize")        },
  { ShowCommandType_minimize,        _T("minimize")        },
  { ShowCommandType_restore,         _T("restore")         },
  { ShowCommandType_show,            _T("show")            },
  { ShowCommandType_showDefault,     _T("showDefault")     },
  { ShowCommandType_showMaximized,   _T("showMaximized")   },
  { ShowCommandType_showMinimized,   _T("showMinimized")   },
  { ShowCommandType_showMinNoActive, _T("showMinNoActive") },
  { ShowCommandType_showNA,          _T("showNA")          },
  { ShowCommandType_showNoActivate,  _T("showNoActivate")  },
  { ShowCommandType_showNormal,      _T("showNormal")      },
};


// stream output
tostream &operator<<(tostream &i_ost, ShowCommandType i_data)
{
  tstring name;
  if (getTypeName(&name, i_data,
		  g_showCommandTypeTable, NUMBER_OF(g_showCommandTypeTable)))
    i_ost << name;
  else
    i_ost << _T("(ShowCommandType internal error)");
  return i_ost;
}


// get value of ShowCommandType
bool getTypeValue(ShowCommandType *o_type, const tstring &i_name)
{
  return getTypeValue(o_type, i_name, g_showCommandTypeTable,
		      NUMBER_OF(g_showCommandTypeTable));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// TargetWindowType


// ModifierLockType table
typedef TypeTable<TargetWindowType> TypeTable_TargetWindowType;
static const TypeTable_TargetWindowType g_targetWindowType[] =
{
  { TargetWindowType_overlapped, _T("overlapped") },
  { TargetWindowType_mdi,        _T("mdi")        },
};


// stream output
tostream &operator<<(tostream &i_ost, TargetWindowType i_data)
{
  tstring name;
  if (getTypeName(&name, i_data,
		  g_targetWindowType, NUMBER_OF(g_targetWindowType)))
    i_ost << name;
  else
    i_ost << _T("(TargetWindowType internal error)");
  return i_ost;
}


// get value of TargetWindowType
bool getTypeValue(TargetWindowType *o_type, const tstring &i_name)
{
  return getTypeValue(o_type, i_name, g_targetWindowType,
		      NUMBER_OF(g_targetWindowType));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// BooleanType


// BooleanType table
typedef TypeTable<BooleanType> TypeTable_BooleanType;
static const TypeTable_BooleanType g_booleanType[] =
{
  { BooleanType_false, _T("false") },
  { BooleanType_true,  _T("true")  },
};


// stream output
tostream &operator<<(tostream &i_ost, BooleanType i_data)
{
  tstring name;
  if (getTypeName(&name, i_data, g_booleanType, NUMBER_OF(g_booleanType)))
    i_ost << name;
  else
    i_ost << _T("(BooleanType internal error)");
  return i_ost;
}


// get value of BooleanType
bool getTypeValue(BooleanType *o_type, const tstring &i_name)
{
  return getTypeValue(o_type, i_name, g_booleanType,
		      NUMBER_OF(g_booleanType));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LogicalOperatorType


// LogicalOperatorType table
typedef TypeTable<LogicalOperatorType> TypeTable_LogicalOperatorType;
static const TypeTable_LogicalOperatorType g_logicalOperatorType[] =
{
  { LogicalOperatorType_or, _T("||") },
  { LogicalOperatorType_and,  _T("&&")  },
};


// stream output
tostream &operator<<(tostream &i_ost, LogicalOperatorType i_data)
{
  tstring name;
  if (getTypeName(&name, i_data, g_logicalOperatorType,
		  NUMBER_OF(g_logicalOperatorType)))
    i_ost << name;
  else
    i_ost << _T("(LogicalOperatorType internal error)");
  return i_ost;
}


// get value of LogicalOperatorType
bool getTypeValue(LogicalOperatorType *o_type, const tstring &i_name)
{
  return getTypeValue(o_type, i_name, g_logicalOperatorType,
		      NUMBER_OF(g_logicalOperatorType));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// WindowMonitorFromType


// WindowMonitorFromType table
typedef TypeTable<WindowMonitorFromType> TypeTable_WindowMonitorFromType;
static const TypeTable_WindowMonitorFromType g_windowMonitorFromType[] =
{
  { WindowMonitorFromType_primary, _T("primary") },
  { WindowMonitorFromType_current, _T("current") },
};


// stream output
tostream &operator<<(tostream &i_ost, WindowMonitorFromType i_data)
{
  tstring name;
  if(getTypeName(&name, i_data, g_windowMonitorFromType,
                 NUMBER_OF(g_windowMonitorFromType)))
    i_ost << name;
  else
    i_ost << _T("(WindowMonitorFromType internal error)");
  return i_ost;
}


// get value of WindowMonitorFromType
bool getTypeValue(WindowMonitorFromType *o_type, const tstring &i_name)
{
  return getTypeValue(o_type, i_name, g_windowMonitorFromType,
                      NUMBER_OF(g_windowMonitorFromType));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// std::list<tstringq>


/// stream output
tostream &operator<<(tostream &i_ost, const std::list<tstringq> &i_data)
{
  for (std::list<tstringq>::const_iterator
	 i = i_data.begin(); i != i_data.end(); ++ i)
  {
    i_ost << *i << _T(", ");
  }
  return i_ost;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// FunctionData


//
FunctionData::~FunctionData()
{
}


// stream output
tostream &operator<<(tostream &i_ost, const FunctionData *i_data)
{
  return i_data->output(i_ost);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// FunctionCreator


///
class FunctionCreator
{
public:
  typedef FunctionData *(*Creator)();		/// 
  
public:
  const _TCHAR *m_name;				/// function name
  Creator m_creator;				/// function data creator
};


// create function
FunctionData *createFunctionData(const tstring &i_name)
{
  static 
#define FUNCTION_CREATOR
#include "functions.h"
#undef FUNCTION_CREATOR
    ;

  for (size_t i = 0; i != NUMBER_OF(functionCreators); ++ i)
    if (i_name == functionCreators[i].m_name)
      return functionCreators[i].m_creator();
  return NULL;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// misc. functions


//
bool getSuitableWindow(FunctionParam *i_param, HWND *o_hwnd)
{
  if (!i_param->m_isPressed)
    return false;
  *o_hwnd = getToplevelWindow(i_param->m_hwnd, NULL);
  if (!*o_hwnd)
    return false;
  return true;
}

//
bool getSuitableMdiWindow(FunctionParam *i_param, HWND *o_hwnd,
			  TargetWindowType *io_twt,
			  RECT *o_rcWindow = NULL, RECT *o_rcParent = NULL)
{
  if (!i_param->m_isPressed)
    return false;
  bool isMdi = *io_twt == TargetWindowType_mdi;
  *o_hwnd = getToplevelWindow(i_param->m_hwnd, &isMdi);
  *io_twt = isMdi ? TargetWindowType_mdi : TargetWindowType_overlapped;
  if (!*o_hwnd)
    return false;
  switch (*io_twt)
  {
    case TargetWindowType_overlapped:
      if (o_rcWindow)
	GetWindowRect(*o_hwnd, o_rcWindow);
      if (o_rcParent) {
        HMONITOR hm = monitorFromWindow(i_param->m_hwnd,
                                        MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi;
        mi.cbSize = sizeof(mi);
        getMonitorInfo(hm, &mi);
        *o_rcParent = mi.rcWork;
      }
      break;
    case TargetWindowType_mdi:
      if (o_rcWindow)
	getChildWindowRect(*o_hwnd, o_rcWindow);
      if (o_rcParent)
	GetClientRect(GetParent(*o_hwnd), o_rcParent);
      break;
  }
  return true;
}

// get clipboard text (you must call closeClopboard())
static const _TCHAR *getTextFromClipboard(HGLOBAL *o_hdata)
{
  *o_hdata = NULL;
  
  if (!OpenClipboard(NULL))
    return NULL;

#ifdef UNICODE
  *o_hdata = GetClipboardData(CF_UNICODETEXT);
#else
  *o_hdata = GetClipboardData(CF_TEXT);
#endif
  if (!*o_hdata)
    return NULL;
  
  _TCHAR *data = reinterpret_cast<_TCHAR *>(GlobalLock(*o_hdata));
  if (!data)
    return NULL;
  return data;
}

// close clipboard that opend by getTextFromClipboard()
static void closeClipboard(HGLOBAL i_hdata, HGLOBAL i_hdataNew = NULL)
{
  if (i_hdata)
    GlobalUnlock(i_hdata);
  if (i_hdataNew)
  {
    EmptyClipboard();
#ifdef UNICODE
    SetClipboardData(CF_UNICODETEXT, i_hdataNew);
#else
    SetClipboardData(CF_TEXT, i_hdataNew);
#endif
  }
  CloseClipboard();
}


// EmacsEditKillLineFunc.
// clear the contents of the clopboard
// at that time, confirm if it is the result of the previous kill-line
void Engine::EmacsEditKillLine::func()
{
  if (!m_buf.empty())
  {
    HGLOBAL g;
    const _TCHAR *text = getTextFromClipboard(&g);
    if (text == NULL || m_buf != text)
      reset();
    closeClipboard(g);
  }
  if (OpenClipboard(NULL))
  {
    EmptyClipboard();
    CloseClipboard();
  }
}


/** if the text of the clipboard is
@doc
<pre>
1: EDIT Control (at EOL C-K): ""            =&gt; buf + "\r\n", Delete   
0: EDIT Control (other  C-K): "(.+)"        =&gt; buf + "\1"             
0: IE FORM TEXTAREA (at EOL C-K): "\r\n"    =&gt; buf + "\r\n"           
2: IE FORM TEXTAREA (other C-K): "(.+)\r\n" =&gt; buf + "\1", Return Left
^retval
</pre>
*/
HGLOBAL Engine::EmacsEditKillLine::makeNewKillLineBuf(
  const _TCHAR *i_data, int *o_retval)
{
  size_t len = m_buf.size();
  len += _tcslen(i_data) + 3;
  
  HGLOBAL hdata = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE,
			      len * sizeof(_TCHAR));
  if (!hdata)
    return NULL;
  _TCHAR *dataNew = reinterpret_cast<_TCHAR *>(GlobalLock(hdata));
  *dataNew = _T('\0');
  if (!m_buf.empty())
    _tcscpy(dataNew, m_buf.c_str());
  
  len = _tcslen(i_data);
  if (3 <= len &&
      i_data[len - 2] == _T('\r') && i_data[len - 1] == _T('\n'))
  {
    _tcscat(dataNew, i_data);
    len = _tcslen(dataNew);
    dataNew[len - 2] = _T('\0'); // chomp
    *o_retval = 2;
  }
  else if (len == 0)
  {
    _tcscat(dataNew, _T("\r\n"));
    *o_retval = 1;
  }
  else
  {
    _tcscat(dataNew, i_data);
    *o_retval = 0;
  }
  
  m_buf = dataNew;
  
  GlobalUnlock(hdata);
  return hdata;
}


// EmacsEditKillLinePred
int Engine::EmacsEditKillLine::pred()
{
  HGLOBAL g;
  const _TCHAR *text = getTextFromClipboard(&g);
  int retval;
  HGLOBAL hdata = makeNewKillLineBuf(text ? text : _T(""), &retval);
  closeClipboard(g, hdata);
  return retval;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// functions


// send a default key to Windows
void Engine::funcDefault(FunctionParam *i_param)
{
  {
    Acquire a(&m_log, 1);
    m_log << std::endl;
    i_param->m_doesNeedEndl = false;
  }
  if (i_param->m_isPressed)
    generateModifierEvents(i_param->m_c.m_mkey.m_modifier);
  generateKeyEvent(i_param->m_c.m_mkey.m_key, i_param->m_isPressed, true);
}

// use a corresponding key of a parent keymap
void Engine::funcKeymapParent(FunctionParam *i_param)
{
  Current c(i_param->m_c);
  c.m_keymap = c.m_keymap->getParentKeymap();
  if (!c.m_keymap)
  {
    funcDefault(i_param);
    return;
  }
  
  {
    Acquire a(&m_log, 1);
    m_log << _T("(") << c.m_keymap->getName() << _T(")") << std::endl;
  }
  i_param->m_doesNeedEndl = false;
  generateKeyboardEvents(c);
}

// use a corresponding key of a current window
void Engine::funcKeymapWindow(FunctionParam *i_param)
{
  Current c(i_param->m_c);
  c.m_keymap = m_currentFocusOfThread->m_keymaps.front();
  c.m_i = m_currentFocusOfThread->m_keymaps.begin();
  generateKeyboardEvents(c);
}

// use a corresponding key of the previous prefixed keymap
void Engine::funcKeymapPrevPrefix(FunctionParam *i_param, int i_previous)
{
  Current c(i_param->m_c);
  if (0 < i_previous && 0 <= m_keymapPrefixHistory.size() - i_previous)
  {
    int n = i_previous - 1;
    KeymapPtrList::reverse_iterator i = m_keymapPrefixHistory.rbegin();
    while (0 < n && i != m_keymapPrefixHistory.rend())
      --n, ++i;
    c.m_keymap = *i;
    generateKeyboardEvents(c);
  }
}

// use a corresponding key of an other window class, or use a default key
void Engine::funcOtherWindowClass(FunctionParam *i_param)
{
  Current c(i_param->m_c);
  ++ c.m_i;
  if (c.m_i == m_currentFocusOfThread->m_keymaps.end())
  {
    funcDefault(i_param);
    return;
  }
  
  c.m_keymap = *c.m_i;
  {
    Acquire a(&m_log, 1);
    m_log << _T("(") << c.m_keymap->getName() << _T(")") << std::endl;
  }
  i_param->m_doesNeedEndl = false;
  generateKeyboardEvents(c);
}

// prefix key
void Engine::funcPrefix(FunctionParam *i_param, const Keymap *i_keymap,
			BooleanType i_doesIgnoreModifiers)
{
  if (!i_param->m_isPressed)
    return;
  
  setCurrentKeymap(i_keymap, true);
  
  // generate prefixed event
  generateEvents(i_param->m_c, m_currentKeymap, &Event::prefixed);
  
  m_isPrefix = true;
  m_doesEditNextModifier = false;
  m_doesIgnoreModifierForPrefix = !!i_doesIgnoreModifiers;

  {
    Acquire a(&m_log, 1);
    m_log << _T("(") << i_keymap->getName() << _T(", ")
	  << (i_doesIgnoreModifiers ? _T("true") : _T("false")) << _T(")");
  }
}

// other keymap's key
void Engine::funcKeymap(FunctionParam *i_param, const Keymap *i_keymap)
{
  Current c(i_param->m_c);
  c.m_keymap = i_keymap;
  {
    Acquire a(&m_log, 1);
    m_log << _T("(") << c.m_keymap->getName() << _T(")") << std::endl;
    i_param->m_doesNeedEndl = false;
  }
  generateKeyboardEvents(c);
}

// sync
void Engine::funcSync(FunctionParam *i_param)
{
  if (i_param->m_isPressed)
    generateModifierEvents(i_param->m_af->m_modifier);
  if (!i_param->m_isPressed || m_currentFocusOfThread->m_isConsole)
    return;
  
  Key *sync = m_setting->m_keyboard.getSyncKey();
  if (sync->getScanCodesSize() == 0)
    return;
  const ScanCode *sc = sync->getScanCodes();
  
  // set variables exported from mayu.dll
  g_hookData->m_syncKey = sc->m_scan;
  g_hookData->m_syncKeyIsExtended = !!(sc->m_flags & ScanCode::E0E1);
  m_isSynchronizing = true;
#if defined(_WINNT)
  generateKeyEvent(sync, false, false);
#elif defined(_WIN95)
  generateKeyEvent(sync, true, false);
#else
#  error
#endif
  
  m_cs.release();
  DWORD r = WaitForSingleObject(m_eSync, 5000);
  if (r == WAIT_TIMEOUT)
  {
    Acquire a(&m_log, 0);
    m_log << _T(" *FAILED*") << std::endl;
  }
  m_cs.acquire();
  m_isSynchronizing = false;
}

// toggle lock
void Engine::funcToggle(FunctionParam *i_param, ModifierLockType i_lock,
			ToggleType i_toggle)
{
  if (i_param->m_isPressed)			// ignore PRESS
    return;

  Modifier::Type mt = static_cast<Modifier::Type>(i_lock);
  switch (i_toggle)
  {
    case ToggleType_toggle:
      m_currentLock.press(mt, !m_currentLock.isPressed(mt));
      break;
    case ToggleType_off:
      m_currentLock.press(mt, false);
      break;
    case ToggleType_on:
      m_currentLock.press(mt, true);
      break;
  }
}

// edit next user input key's modifier
void Engine::funcEditNextModifier(FunctionParam *i_param,
				  const Modifier &i_modifier)
{
  if (!i_param->m_isPressed)
    return;
  
  m_isPrefix = true;
  m_doesEditNextModifier = true;
  m_doesIgnoreModifierForPrefix = true;
  m_modifierForNextKey = i_modifier;
}

// variable
void Engine::funcVariable(FunctionParam *i_param, int i_mag, int i_inc)
{
  if (!i_param->m_isPressed)
    return;
  m_variable *= i_mag;
  m_variable += i_inc;
}

// repeat N times
void Engine::funcRepeat(FunctionParam *i_param, const KeySeq *i_keySeq,
			int i_max)
{
  if (i_param->m_isPressed)
  {
    int end = MIN(m_variable, i_max);
    for (int i = 0; i < end - 1; ++ i)
      generateKeySeqEvents(i_param->m_c, i_keySeq, Part_all);
    if (0 < end)
      generateKeySeqEvents(i_param->m_c, i_keySeq, Part_down);
  }
  else
    generateKeySeqEvents(i_param->m_c, i_keySeq, Part_up);
}

// undefined (bell)
void Engine::funcUndefined(FunctionParam *i_param)
{
  if (!i_param->m_isPressed)
    return;
  MessageBeep(MB_OK);
}

// ignore
void Engine::funcIgnore(FunctionParam *)
{
  // do nothing
}

// post message
void Engine::funcPostMessage(FunctionParam *i_param, ToWindowType i_window,
			     UINT i_message, WPARAM i_wParam, LPARAM i_lParam)
{
  if (!i_param->m_isPressed)
    return;

  int window = static_cast<int>(i_window);
  
  HWND hwnd = i_param->m_hwnd;
  if (0 < window)
  {
    for (int i = 0; i < window; ++ i)
      hwnd = GetParent(hwnd);
  }
  else if (window == ToWindowType_toMainWindow)
  {
    while (true)
    {
      HWND p = GetParent(hwnd);
      if (!p)
	break;
      hwnd = p;
    }
  }
  else if (window == ToWindowType_toOverlappedWindow)
  {
    while (hwnd)
    {
      LONG style = GetWindowLong(hwnd, GWL_STYLE);
      if ((style & WS_CHILD) == 0)
	break;
      hwnd = GetParent(hwnd);
    }
  }

  if (hwnd)
    PostMessage(hwnd, i_message, i_wParam, i_lParam);
}


// ShellExecute
void Engine::funcShellExecute(FunctionParam *i_param,
			      const StrExprArg &/*i_operation*/,
			      const StrExprArg &/*i_file*/,
			      const StrExprArg &/*i_parameters*/,
			      const StrExprArg &/*i_directory*/,
			      ShowCommandType /*i_showCommand*/)
{
  if (!i_param->m_isPressed)
    return;
  m_afShellExecute = i_param->m_af;
  PostMessage(m_hwndAssocWindow,
	      WM_APP_engineNotify, EngineNotify_shellExecute, 0);
}


// shell execute
void Engine::shellExecute()
{
  Acquire a(&m_cs);
  
  FunctionData_ShellExecute *fd =
    reinterpret_cast<FunctionData_ShellExecute *>(
      m_afShellExecute->m_functionData);
  
  int r = (int)ShellExecute(
    NULL,
    fd->m_operation.eval().empty() ? _T("open") : fd->m_operation.eval().c_str(),
    fd->m_file.eval().empty() ? NULL : fd->m_file.eval().c_str(),
    fd->m_parameters.eval().empty() ? NULL : fd->m_parameters.eval().c_str(),
    fd->m_directory.eval().empty() ? NULL : fd->m_directory.eval().c_str(),
    fd->m_showCommand);
  if (32 < r)
    return; // success

  typedef TypeTable<int> ErrorTable;
  static const ErrorTable errorTable[] =
  {
    { 0, _T("The operating system is out of memory or resources.") },
    { ERROR_FILE_NOT_FOUND, _T("The specified file was not found.") },
    { ERROR_PATH_NOT_FOUND, _T("The specified path was not found.") },
    { ERROR_BAD_FORMAT, _T("The .exe file is invalid ")
      _T("(non-Win32R .exe or error in .exe image).") },
    { SE_ERR_ACCESSDENIED,
      _T("The operating system denied access to the specified file.") },
    { SE_ERR_ASSOCINCOMPLETE,
      _T("The file name association is incomplete or invalid.") },
    { SE_ERR_DDEBUSY,
      _T("The DDE transaction could not be completed ")
      _T("because other DDE transactions were being processed. ") },
    { SE_ERR_DDEFAIL, _T("The DDE transaction failed.") },
    { SE_ERR_DDETIMEOUT, _T("The DDE transaction could not be completed ")
      _T("because the request timed out.") },
    { SE_ERR_DLLNOTFOUND,
      _T("The specified dynamic-link library was not found.") },
    { SE_ERR_FNF, _T("The specified file was not found.") },
    { SE_ERR_NOASSOC, _T("There is no application associated ")
      _T("with the given file name extension.") },
    { SE_ERR_OOM,
      _T("There was not enough memory to complete the operation.") },
    { SE_ERR_PNF, _T("The specified path was not found.") },
    { SE_ERR_SHARE, _T("A sharing violation occurred.") },
  };

  tstring errorMessage(_T("Unknown error."));
  getTypeName(&errorMessage, r, errorTable, NUMBER_OF(errorTable));
  
  Acquire b(&m_log, 0);
  m_log << _T("error: ") << fd << _T(": ") << errorMessage << std::endl;
}


struct EnumWindowsForSetForegroundWindowParam
{
  const FunctionData_SetForegroundWindow *m_fd;
  HWND m_hwnd;

public:
  EnumWindowsForSetForegroundWindowParam(
    const FunctionData_SetForegroundWindow *i_fd)
    : m_fd(i_fd),
      m_hwnd(NULL)
  {
  }
};


/// enum windows for SetForegroundWindow
static BOOL CALLBACK enumWindowsForSetForegroundWindow(
  HWND i_hwnd, LPARAM i_lParam)
{
  EnumWindowsForSetForegroundWindowParam &ep =
    *reinterpret_cast<EnumWindowsForSetForegroundWindowParam *>(i_lParam);

  _TCHAR name[GANA_MAX_ATOM_LENGTH];
  if (!GetClassName(i_hwnd, name, NUMBER_OF(name)))
    return TRUE;
  tsmatch what;
  if (!boost::regex_search(tstring(name), what, ep.m_fd->m_windowClassName))
    if (ep.m_fd->m_logicalOp == LogicalOperatorType_and)
      return TRUE;				// match failed

  if (ep.m_fd->m_logicalOp == LogicalOperatorType_and)
  {
    if (GetWindowText(i_hwnd, name, NUMBER_OF(name)) == 0)
      name[0] = _T('\0');
    if (!boost::regex_search(tstring(name), what,
			     ep.m_fd->m_windowTitleName))
      return TRUE;				// match failed
  }

  ep.m_hwnd = i_hwnd;
  return FALSE;
}


/// SetForegroundWindow
void Engine::funcSetForegroundWindow(FunctionParam *i_param, const tregex &,
				     LogicalOperatorType , const tregex &)
{
  if (!i_param->m_isPressed)
    return;
  EnumWindowsForSetForegroundWindowParam
    ep(static_cast<const FunctionData_SetForegroundWindow *>(
      i_param->m_af->m_functionData));
  EnumWindows(enumWindowsForSetForegroundWindow,
	      reinterpret_cast<LPARAM>(&ep));
  if (ep.m_hwnd)
    PostMessage(m_hwndAssocWindow,
		WM_APP_engineNotify, EngineNotify_setForegroundWindow,
		reinterpret_cast<LPARAM>(ep.m_hwnd));

}


// load setting
void Engine::funcLoadSetting(FunctionParam *i_param, const StrExprArg &i_name)
{
  if (!i_param->m_isPressed)
    return;
  if (!i_name.eval().empty())
  {
    // set MAYU_REGISTRY_ROOT\.mayuIndex which name is same with i_name
    Registry reg(MAYU_REGISTRY_ROOT);

    tregex split(_T("^([^;]*);([^;]*);(.*)$"));
    tstringi dot_mayu;
    for (size_t i = 0; i < MAX_MAYU_REGISTRY_ENTRIES; ++ i)
    {
      _TCHAR buf[100];
      _sntprintf(buf, NUMBER_OF(buf), _T(".mayu%d"), i);
      if (!reg.read(buf, &dot_mayu))
	break;
      
      tsmatch what;
      if (boost::regex_match(dot_mayu, what, split) &&
	  what.str(1) == i_name.eval())
      {	
	reg.write(_T(".mayuIndex"), i);
	goto success;
      }
    }

    {
      Acquire a(&m_log, 0);
      m_log << _T("unknown setting name: ") << i_name;
    }
    return;
    
    success: ;
  }
  PostMessage(m_hwndAssocWindow,
	      WM_APP_engineNotify, EngineNotify_loadSetting, 0);
}

// virtual key
void Engine::funcVK(FunctionParam *i_param, VKey i_vkey)
{
  long key = static_cast<long>(i_vkey);
  BYTE vkey = static_cast<BYTE>(i_vkey);
  bool isExtended = !!(key & VKey_extended);
  bool isUp       = !i_param->m_isPressed && !!(key & VKey_released);
  bool isDown     = i_param->m_isPressed && !!(key & VKey_pressed);
  
  if (vkey == VK_LBUTTON && isDown)
    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
  else if (vkey == VK_LBUTTON && isUp)
    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
  else if (vkey == VK_MBUTTON && isDown)
    mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, 0);
  else if (vkey == VK_MBUTTON && isUp)
    mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, 0);
  else if (vkey == VK_RBUTTON && isDown)
    mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
  else if (vkey == VK_RBUTTON && isUp)
    mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
#if(_WIN32_WINNT >= 0x0500)
  else if (vkey == VK_XBUTTON1 && isDown)
    mouse_event(MOUSEEVENTF_XDOWN, 0, 0, XBUTTON1, 0);
  else if (vkey == VK_XBUTTON1 && isUp)
    mouse_event(MOUSEEVENTF_XUP, 0, 0, XBUTTON1, 0);
  else if (vkey == VK_XBUTTON2 && isDown)
    mouse_event(MOUSEEVENTF_XDOWN, 0, 0, XBUTTON2, 0);
  else if (vkey == VK_XBUTTON2 && isUp)
    mouse_event(MOUSEEVENTF_XUP, 0, 0, XBUTTON2, 0);
#endif /* _WIN32_WINNT >= 0x0500 */
  else if (isUp || isDown)
    keybd_event(vkey,
		static_cast<BYTE>(MapVirtualKey(vkey, 0)),
		(isExtended ? KEYEVENTF_EXTENDEDKEY : 0) |
		(i_param->m_isPressed ? 0 : KEYEVENTF_KEYUP), 0);
}

// wait
void Engine::funcWait(FunctionParam *i_param, int i_milliSecond)
{
  if (!i_param->m_isPressed)
    return;
  if (i_milliSecond < 0 || 5000 < i_milliSecond)	// too long wait
    return;
  
  m_isSynchronizing = true;
  m_cs.release();
  Sleep(i_milliSecond);
  m_cs.acquire();
  m_isSynchronizing = false;
}

// investigate WM_COMMAND, WM_SYSCOMMAND
void Engine::funcInvestigateCommand(FunctionParam *i_param)
{
  if (!i_param->m_isPressed)
    return;
  Acquire a(&m_log, 0);
  g_hookData->m_doesNotifyCommand = !g_hookData->m_doesNotifyCommand;
  if (g_hookData->m_doesNotifyCommand)
    m_log << _T(" begin") << std::endl;
  else
    m_log << _T(" end") << std::endl;
}

// show mayu dialog box
void Engine::funcMayuDialog(FunctionParam *i_param, MayuDialogType i_dialog,
			    ShowCommandType i_showCommand)
{
  if (!i_param->m_isPressed)
    return;
  PostMessage(getAssociatedWndow(), WM_APP_engineNotify, EngineNotify_showDlg,
	      static_cast<LPARAM>(i_dialog) |
	      static_cast<LPARAM>(i_showCommand));
}

// describe bindings
void Engine::funcDescribeBindings(FunctionParam *i_param)
{
  if (!i_param->m_isPressed)
    return;
  {
    Acquire a(&m_log, 1);
    m_log << std::endl;
  }
  describeBindings();
}

// show help message
void Engine::funcHelpMessage(FunctionParam *i_param, const StrExprArg &i_title,
			     const StrExprArg &i_message)
{
  if (!i_param->m_isPressed)
    return;

  m_helpTitle = i_title.eval();
  m_helpMessage = i_message.eval();
  bool doesShow = !(i_title.eval().size() == 0 && i_message.eval().size() == 0);
  PostMessage(getAssociatedWndow(), WM_APP_engineNotify,
	      EngineNotify_helpMessage, doesShow);
}

// show variable
void Engine::funcHelpVariable(FunctionParam *i_param, const StrExprArg &i_title)
{
  if (!i_param->m_isPressed)
    return;

  _TCHAR buf[20];
  _sntprintf(buf, NUMBER_OF(buf), _T("%d"), m_variable);

  m_helpTitle = i_title.eval();
  m_helpMessage = buf;
  PostMessage(getAssociatedWndow(), WM_APP_engineNotify,
	      EngineNotify_helpMessage, true);
}

// raise window
void Engine::funcWindowRaise(FunctionParam *i_param,
			     TargetWindowType i_twt)
{
  HWND hwnd;
  if (!getSuitableMdiWindow(i_param, &hwnd, &i_twt))
    return;
  SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0,
	       SWP_ASYNCWINDOWPOS | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOSIZE);
}

// lower window
void Engine::funcWindowLower(FunctionParam *i_param, TargetWindowType i_twt)
{
  HWND hwnd;
  if (!getSuitableMdiWindow(i_param, &hwnd, &i_twt))
    return;
  SetWindowPos(hwnd, HWND_BOTTOM, 0, 0, 0, 0,
	       SWP_ASYNCWINDOWPOS | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOSIZE);
}

// minimize window
void Engine::funcWindowMinimize(FunctionParam *i_param, TargetWindowType i_twt)
{
  HWND hwnd;
  if (!getSuitableMdiWindow(i_param, &hwnd, &i_twt))
    return;
  PostMessage(hwnd, WM_SYSCOMMAND,
	      IsIconic(hwnd) ? SC_RESTORE : SC_MINIMIZE, 0);
}

// maximize window
void Engine::funcWindowMaximize(FunctionParam *i_param, TargetWindowType i_twt)
{
  HWND hwnd;
  if (!getSuitableMdiWindow(i_param, &hwnd, &i_twt))
    return;
  PostMessage(hwnd, WM_SYSCOMMAND,
	      IsZoomed(hwnd) ? SC_RESTORE : SC_MAXIMIZE, 0);
}

// maximize horizontally or virtically
void Engine::funcWindowHVMaximize(FunctionParam *i_param,
				  BooleanType i_isHorizontal,
				  TargetWindowType i_twt)
{
  HWND hwnd;
  RECT rc, rcd;
  if (!getSuitableMdiWindow(i_param, &hwnd, &i_twt, &rc, &rcd))
    return;

  // erase non window
  while (true)
  {
    WindowPositions::iterator i = m_windowPositions.begin();
    WindowPositions::iterator end = m_windowPositions.end();
    for (; i != end; ++ i)
      if (!IsWindow((*i).m_hwnd))
	break;
    if (i == end)
      break;
    m_windowPositions.erase(i);
  }

  // find target
  WindowPositions::iterator i = m_windowPositions.begin();
  WindowPositions::iterator end = m_windowPositions.end();
  WindowPositions::iterator target = end;
  for (; i != end; ++ i)
    if ((*i).m_hwnd == hwnd)
    {
      target = i;
      break;
    }
  
  if (IsZoomed(hwnd))
    PostMessage(hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
  else
  {
    WindowPosition::Mode mode = WindowPosition::Mode_normal;
    
    if (target != end)
    {
      WindowPosition &wp = *target;
      rc = wp.m_rc;
      if (wp.m_mode == WindowPosition::Mode_HV)
	mode = wp.m_mode =
	  i_isHorizontal ? WindowPosition::Mode_V : WindowPosition::Mode_H;
      else if (( i_isHorizontal && wp.m_mode == WindowPosition::Mode_V) ||
	       (!i_isHorizontal && wp.m_mode == WindowPosition::Mode_H))
	mode = wp.m_mode = WindowPosition::Mode_HV;
      else
	m_windowPositions.erase(target);
    }
    else
    {
      mode = i_isHorizontal ? WindowPosition::Mode_H : WindowPosition::Mode_V;
      m_windowPositions.push_front(WindowPosition(hwnd, rc, mode));
    }
    
    if (static_cast<int>(mode) & static_cast<int>(WindowPosition::Mode_H))
      rc.left = rcd.left, rc.right = rcd.right;
    if (static_cast<int>(mode) & static_cast<int>(WindowPosition::Mode_V))
      rc.top = rcd.top, rc.bottom = rcd.bottom;
    
    asyncMoveWindow(hwnd, rc.left, rc.top, rcWidth(&rc), rcHeight(&rc));
  }
}

// maximize window horizontally
void Engine::funcWindowHMaximize(FunctionParam *i_param,
				 TargetWindowType i_twt)
{
  funcWindowHVMaximize(i_param, BooleanType_true, i_twt);
}

// maximize window virtically
void Engine::funcWindowVMaximize(FunctionParam *i_param,
				 TargetWindowType i_twt)
{
  funcWindowHVMaximize(i_param, BooleanType_false, i_twt);
}

// move window
void Engine::funcWindowMove(FunctionParam *i_param, int i_dx, int i_dy,
			    TargetWindowType i_twt)
{
  funcWindowMoveTo(i_param, GravityType_C, i_dx, i_dy, i_twt);
}

// move window to ...
void Engine::funcWindowMoveTo(FunctionParam *i_param,
			      GravityType i_gravityType,
			      int i_dx, int i_dy, TargetWindowType i_twt)
{
  HWND hwnd;
  RECT rc, rcd;
  if (!getSuitableMdiWindow(i_param, &hwnd, &i_twt, &rc, &rcd))
    return;
  
  int x = rc.left + i_dx;
  int y = rc.top + i_dy;

  if (i_gravityType & GravityType_N)
    y = i_dy + rcd.top;
  if (i_gravityType & GravityType_E)
    x = i_dx + rcd.right - rcWidth(&rc);
  if (i_gravityType & GravityType_W)
    x = i_dx + rcd.left;
  if (i_gravityType & GravityType_S)
    y = i_dy + rcd.bottom - rcHeight(&rc);
  asyncMoveWindow(hwnd, x, y);
}


// move window visibly
void Engine::funcWindowMoveVisibly(FunctionParam *i_param,
				   TargetWindowType i_twt)
{
  HWND hwnd;
  RECT rc, rcd;
  if (!getSuitableMdiWindow(i_param, &hwnd, &i_twt, &rc, &rcd))
    return;

  int x = rc.left;
  int y = rc.top;
  if (rc.left < rcd.left)
    x = rcd.left;
  else if (rcd.right < rc.right)
    x = rcd.right - rcWidth(&rc);
  if (rc.top < rcd.top)
    y = rcd.top;
  else if (rcd.bottom < rc.bottom)
    y = rcd.bottom - rcHeight(&rc);
  asyncMoveWindow(hwnd, x, y);
}


struct EnumDisplayMonitorsForWindowMonitorToParam
{
  std::vector<HMONITOR> m_monitors;
  std::vector<MONITORINFO> m_monitorinfos;
  int m_primaryMonitorIdx;
  int m_currentMonitorIdx;

  HMONITOR m_hmon;

public:
  EnumDisplayMonitorsForWindowMonitorToParam(HMONITOR i_hmon)
    : m_hmon(i_hmon),
      m_primaryMonitorIdx(-1), m_currentMonitorIdx(-1)
  {
  }
};

static BOOL CALLBACK enumDisplayMonitorsForWindowMonitorTo(
  HMONITOR i_hmon, HDC i_hdc, LPRECT i_rcMonitor, LPARAM i_data)
{
  EnumDisplayMonitorsForWindowMonitorToParam &ep =
    *reinterpret_cast<EnumDisplayMonitorsForWindowMonitorToParam *>(i_data);

  ep.m_monitors.push_back(i_hmon);

  MONITORINFO mi;
  mi.cbSize = sizeof(mi);
  getMonitorInfo(i_hmon, &mi);
  ep.m_monitorinfos.push_back(mi);

  if(mi.dwFlags & MONITORINFOF_PRIMARY)
    ep.m_primaryMonitorIdx = ep.m_monitors.size() - 1;
  if(i_hmon == ep.m_hmon)
    ep.m_currentMonitorIdx = ep.m_monitors.size() - 1;

  return TRUE;
}

/// move window to other monitor
void Engine::funcWindowMonitorTo(
    FunctionParam *i_param, WindowMonitorFromType i_fromType, int i_monitor,
    BooleanType i_adjustPos, BooleanType i_adjustSize)
{
  HWND hwnd;
  if(! getSuitableWindow(i_param, &hwnd))
    return;

  HMONITOR hmonCur;
  hmonCur = monitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);

  EnumDisplayMonitorsForWindowMonitorToParam ep(hmonCur);
  enumDisplayMonitors(NULL, NULL, enumDisplayMonitorsForWindowMonitorTo,
                      reinterpret_cast<LPARAM>(&ep));
  if(ep.m_monitors.size() < 1 ||
     ep.m_primaryMonitorIdx < 0 || ep.m_currentMonitorIdx < 0)
    return;

  int targetIdx;
  switch(i_fromType) {
  case WindowMonitorFromType_primary:
    targetIdx = (ep.m_primaryMonitorIdx + i_monitor) % ep.m_monitors.size();
    break;

  case WindowMonitorFromType_current:
    targetIdx = (ep.m_currentMonitorIdx + i_monitor) % ep.m_monitors.size();
    break;
  }
  if(ep.m_currentMonitorIdx == targetIdx)
    return;

  RECT rcCur, rcTarget, rcWin;
  rcCur = ep.m_monitorinfos[ep.m_currentMonitorIdx].rcWork;
  rcTarget = ep.m_monitorinfos[targetIdx].rcWork;
  GetWindowRect(hwnd, &rcWin);

  int x = rcTarget.left + (rcWin.left - rcCur.left);
  int y = rcTarget.top + (rcWin.top - rcCur.top);
  int w = rcWidth(&rcWin);
  int h = rcHeight(&rcWin);

  if(i_adjustPos) {
    if(x + w > rcTarget.right)
      x = rcTarget.right - w;
    if(x < rcTarget.left)
      x = rcTarget.left;
    if(w > rcWidth(&rcTarget)) {
      x = rcTarget.left;
      w = rcWidth(&rcTarget);
    }

    if(y + h > rcTarget.bottom)
      y = rcTarget.bottom - h;
    if(y < rcTarget.top)
      y = rcTarget.top;
    if(h > rcHeight(&rcTarget)) {
      y = rcTarget.top;
      h = rcHeight(&rcTarget);
    }
  }

  if(i_adjustPos && i_adjustSize) {
    if(IsZoomed(hwnd))
      PostMessage(hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
    asyncMoveWindow(hwnd, x, y, w, h);
  } else {
    asyncMoveWindow(hwnd, x, y);
  }
}

/// move window to other monitor
void Engine::funcWindowMonitor(
    FunctionParam *i_param, int i_monitor,
    BooleanType i_adjustPos, BooleanType i_adjustSize)
{
  funcWindowMonitorTo(i_param, WindowMonitorFromType_primary, i_monitor,
                      i_adjustPos, i_adjustSize);
}


//
void Engine::funcWindowClingToLeft(FunctionParam *i_param,
				   TargetWindowType i_twt)
{
  funcWindowMoveTo(i_param, GravityType_W, 0, 0, i_twt);
}

//
void Engine::funcWindowClingToRight(FunctionParam *i_param,
				    TargetWindowType i_twt)
{
  funcWindowMoveTo(i_param, GravityType_E, 0, 0, i_twt);
}

//
void Engine::funcWindowClingToTop(FunctionParam *i_param,
				  TargetWindowType i_twt)
{
  funcWindowMoveTo(i_param, GravityType_N, 0, 0, i_twt);
}

//
void Engine::funcWindowClingToBottom(FunctionParam *i_param,
				     TargetWindowType i_twt)
{
  funcWindowMoveTo(i_param, GravityType_S, 0, 0, i_twt);
}

// close window
void Engine::funcWindowClose(FunctionParam *i_param, TargetWindowType i_twt)
{
  HWND hwnd;
  if (!getSuitableMdiWindow(i_param, &hwnd, &i_twt))
    return;
  PostMessage(hwnd, WM_SYSCOMMAND, SC_CLOSE, 0);
}

// toggle top-most flag of the window
void Engine::funcWindowToggleTopMost(FunctionParam *i_param)
{
  HWND hwnd;
  if (!getSuitableWindow(i_param, &hwnd))
    return;
  SetWindowPos(
    hwnd,
    (GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST) ?
    HWND_NOTOPMOST : HWND_TOPMOST,
    0, 0, 0, 0,
    SWP_ASYNCWINDOWPOS | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOSIZE);
}

// identify the window
void Engine::funcWindowIdentify(FunctionParam *i_param)
{
  if (!i_param->m_isPressed)
    return;

  _TCHAR className[GANA_MAX_ATOM_LENGTH];
  bool ok = false;
  if (GetClassName(i_param->m_hwnd, className, NUMBER_OF(className)))
  {
    if (_tcsicmp(className, _T("ConsoleWindowClass")) == 0)
    {
      _TCHAR titleName[1024];
      if (GetWindowText(i_param->m_hwnd, titleName, NUMBER_OF(titleName)) == 0)
	titleName[0] = _T('\0');
      {
	Acquire a(&m_log, 1);
	m_log << _T("HWND:\t") << std::hex
	      << reinterpret_cast<int>(i_param->m_hwnd)
	      << std::dec << std::endl;
      }
      Acquire a(&m_log, 0);
      m_log << _T("CLASS:\t") << className << std::endl;
      m_log << _T("TITLE:\t") << titleName << std::endl;

      HWND hwnd = getToplevelWindow(i_param->m_hwnd, NULL);
      RECT rc;
      GetWindowRect(hwnd, &rc);
      m_log << _T("Toplevel Window Position/Size: (")
	    << rc.left << _T(", ") << rc.top << _T(") / (")
	    << rcWidth(&rc) << _T("x") << rcHeight(&rc)
	    << _T(")") << std::endl;
      
      SystemParametersInfo(SPI_GETWORKAREA, 0, (void *)&rc, FALSE);
      m_log << _T("Desktop Window Position/Size: (")
	    << rc.left << _T(", ") << rc.top << _T(") / (")
	    << rcWidth(&rc) << _T("x") << rcHeight(&rc)
	    << _T(")") << std::endl;

      m_log << std::endl;
      ok = true;
    }
  }
  if (!ok)
  {
    UINT WM_MAYU_MESSAGE = RegisterWindowMessage(
		addSessionId(WM_MAYU_MESSAGE_NAME).c_str());
    CHECK_TRUE( PostMessage(i_param->m_hwnd, WM_MAYU_MESSAGE,
			    MayuMessage_notifyName, 0) );
  }
}

// set alpha blending parameter to the window
void Engine::funcWindowSetAlpha(FunctionParam *i_param, int i_alpha)
{
#if defined(_WINNT)
  HWND hwnd;
  if (!getSuitableWindow(i_param, &hwnd))
    return;
  
  if (i_alpha < 0)	// remove all alpha
  {
    for (WindowsWithAlpha::iterator i = m_windowsWithAlpha.begin(); 
	 i != m_windowsWithAlpha.end(); ++ i)
    {
      SetWindowLong(*i, GWL_EXSTYLE,
		    GetWindowLong(*i, GWL_EXSTYLE) & ~WS_EX_LAYERED);
      RedrawWindow(*i, NULL, NULL,
		   RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);
    }
    m_windowsWithAlpha.clear();
  }
  else
  {
    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    if (exStyle & WS_EX_LAYERED)	// remove alpha
    {
      WindowsWithAlpha::iterator
	i = std::find(m_windowsWithAlpha.begin(), m_windowsWithAlpha.end(),
		      hwnd);
      if (i == m_windowsWithAlpha.end())
	return;	// already layered by the application
    
      m_windowsWithAlpha.erase(i);
    
      SetWindowLong(hwnd, GWL_EXSTYLE, exStyle & ~WS_EX_LAYERED);
    }
    else	// add alpha
    {
      SetWindowLong(hwnd, GWL_EXSTYLE, exStyle | WS_EX_LAYERED);
      i_alpha %= 101;
      if (!setLayeredWindowAttributes(hwnd, 0,
				      (BYTE)(255 * i_alpha / 100), LWA_ALPHA))
      {
	Acquire a(&m_log, 0);
	m_log << _T("error: &WindowSetAlpha(") << i_alpha
	      << _T(") failed for HWND: ") << std::hex
	      << hwnd << std::dec << std::endl;
	return;
      }
      m_windowsWithAlpha.push_front(hwnd);
    }
    RedrawWindow(hwnd, NULL, NULL,
		 RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);
  }
#endif // _WINNT
}


// redraw the window
void Engine::funcWindowRedraw(FunctionParam *i_param)
{
  HWND hwnd;
  if (!getSuitableWindow(i_param, &hwnd))
    return;
  RedrawWindow(hwnd, NULL, NULL,
	       RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);
}

// resize window to
void Engine::funcWindowResizeTo(FunctionParam *i_param, int i_width,
				int i_height, TargetWindowType i_twt)
{
  HWND hwnd;
  RECT rc, rcd;
  if (!getSuitableMdiWindow(i_param, &hwnd, &i_twt, &rc, &rcd))
    return;
  
  if (i_width == 0)
    i_width = rcWidth(&rc);
  else if (i_width < 0)
    i_width += rcWidth(&rcd);
  
  if (i_height == 0)
    i_height = rcHeight(&rc);
  else if (i_height < 0)
    i_height += rcHeight(&rcd);
  
  asyncResize(hwnd, i_width, i_height);
}

// move the mouse cursor
void Engine::funcMouseMove(FunctionParam *i_param, int i_dx, int i_dy)
{
  if (!i_param->m_isPressed)
    return;
  POINT pt;
  GetCursorPos(&pt);
  SetCursorPos(pt.x + i_dx, pt.y + i_dy);
}

// send a mouse-wheel-message to Windows
void Engine::funcMouseWheel(FunctionParam *i_param, int i_delta)
{
  if (!i_param->m_isPressed)
    return;
  mouse_event(MOUSEEVENTF_WHEEL, 0, 0, i_delta, 0);
}

// convert the contents of the Clipboard to upper case
void Engine::funcClipboardChangeCase(FunctionParam *i_param,
				     BooleanType i_doesConvertToUpperCase)
{
  if (!i_param->m_isPressed)
    return;
  
  HGLOBAL hdata;
  const _TCHAR *text = getTextFromClipboard(&hdata);
  HGLOBAL hdataNew = NULL;
  if (text)
  {
    int size = static_cast<int>(GlobalSize(hdata));
    hdataNew = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, size);
    if (hdataNew)
    {
      if (_TCHAR *dataNew = reinterpret_cast<_TCHAR *>(GlobalLock(hdataNew)))
      {
	std::memcpy(dataNew, text, size);
	_TCHAR *dataEnd = dataNew + size;
	while (dataNew < dataEnd && *dataNew)
	{
	  _TCHAR c = *dataNew;
	  if (_istlead(c))
	    dataNew += 2;
	  else
	    *dataNew++ =
	      i_doesConvertToUpperCase ? _totupper(c) : _totlower(c);
	}
	GlobalUnlock(hdataNew);
      }
    }
  }
  closeClipboard(hdata, hdataNew);
}

// convert the contents of the Clipboard to upper case
void Engine::funcClipboardUpcaseWord(FunctionParam *i_param)
{
  funcClipboardChangeCase(i_param, BooleanType_true);
}

// convert the contents of the Clipboard to lower case
void Engine::funcClipboardDowncaseWord(FunctionParam *i_param)
{
  funcClipboardChangeCase(i_param, BooleanType_false);
}

// set the contents of the Clipboard to the string
void Engine::funcClipboardCopy(FunctionParam *i_param, const StrExprArg &i_text)
{
  if (!i_param->m_isPressed)
    return;
  if (!OpenClipboard(NULL))
    return;
  
  HGLOBAL hdataNew =
    GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE,
		(i_text.eval().size() + 1) * sizeof(_TCHAR));
  if (!hdataNew)
    return;
  _TCHAR *dataNew = reinterpret_cast<_TCHAR *>(GlobalLock(hdataNew));
  _tcscpy(dataNew, i_text.eval().c_str());
  GlobalUnlock(hdataNew);
  closeClipboard(NULL, hdataNew);
}

//
void Engine::funcEmacsEditKillLinePred(
  FunctionParam *i_param, const KeySeq *i_keySeq1, const KeySeq *i_keySeq2)
{
  m_emacsEditKillLine.m_doForceReset = false;
  if (!i_param->m_isPressed)
    return;
  
  int r = m_emacsEditKillLine.pred();
  const KeySeq *keySeq;
  if (r == 1)
    keySeq = i_keySeq1;
  else if (r == 2)
    keySeq = i_keySeq2;
  else // r == 0
    return;
  ASSERT(keySeq);
  generateKeySeqEvents(i_param->m_c, keySeq, Part_all);
}

//
void Engine::funcEmacsEditKillLineFunc(FunctionParam *i_param)
{
  if (!i_param->m_isPressed)
    return;
  m_emacsEditKillLine.func();
  m_emacsEditKillLine.m_doForceReset = false;
}

// clear log
void Engine::funcLogClear(FunctionParam *i_param)
{
  if (!i_param->m_isPressed)
    return;
  PostMessage(getAssociatedWndow(), WM_APP_engineNotify,
	      EngineNotify_clearLog, 0);
}

// recenter
void Engine::funcRecenter(FunctionParam *i_param)
{
  if (!i_param->m_isPressed)
    return;
  if (m_hwndFocus)
  {
    UINT WM_MAYU_MESSAGE = RegisterWindowMessage(
		addSessionId(WM_MAYU_MESSAGE_NAME).c_str());
    PostMessage(m_hwndFocus, WM_MAYU_MESSAGE, MayuMessage_funcRecenter, 0);
  }
}

// set IME open status
void Engine::funcSetImeStatus(FunctionParam *i_param, ToggleType i_toggle)
{
  if (!i_param->m_isPressed)
    return;
  if (m_hwndFocus)
  {
    UINT WM_MAYU_MESSAGE = RegisterWindowMessage(
		addSessionId(WM_MAYU_MESSAGE_NAME).c_str());
    int status = -1;
    switch (i_toggle)
    {
      case ToggleType_toggle:
	status = -1;
	break;
      case ToggleType_off:
	status = 0;
	break;
      case ToggleType_on:
	status = 1;
	break;
    }
    PostMessage(m_hwndFocus, WM_MAYU_MESSAGE, MayuMessage_funcSetImeStatus, status);
  }
}

// set IME open status
void Engine::funcSetImeString(FunctionParam *i_param, const StrExprArg &i_data)
{
#if defined(_WINNT)
  if (!i_param->m_isPressed)
    return;
  if (m_hwndFocus)
  {
    UINT WM_MAYU_MESSAGE = RegisterWindowMessage(
		addSessionId(WM_MAYU_MESSAGE_NAME).c_str());
    PostMessage(m_hwndFocus, WM_MAYU_MESSAGE, MayuMessage_funcSetImeString, i_data.eval().size() * sizeof(_TCHAR));

    DWORD len = 0;
    DWORD error;
    DisconnectNamedPipe(m_hookPipe);
    ConnectNamedPipe(m_hookPipe, NULL);
    error = WriteFile(m_hookPipe, i_data.eval().c_str(),
		      i_data.eval().size() * sizeof(_TCHAR),
		      &len, NULL);

    //FlushFileBuffers(m_hookPipe);
  }
#else
  Acquire a(&m_log);
  m_log << _T("supported on only Windows NT/2000/XP") << std::endl;
#endif
}

// Direct SSTP Server
class DirectSSTPServer
{
public:
  tstring m_path;
  HWND m_hwnd;
  tstring m_name;
  tstring m_keroname;

public:
  DirectSSTPServer()
    : m_hwnd(NULL)
  {
  }
};


class ParseDirectSSTPData
{
  typedef boost::match_results<boost::regex::const_iterator> MR;

public:
  typedef std::map<tstring, DirectSSTPServer> DirectSSTPServers;

private:
  DirectSSTPServers *m_directSSTPServers;
  
public:
  // constructor
  ParseDirectSSTPData(DirectSSTPServers *i_directSSTPServers)
    : m_directSSTPServers(i_directSSTPServers)
  {
  }
  
  bool operator()(const MR& i_what)
  {
#ifdef _UNICODE
    tstring id(to_wstring(std::string(i_what[1].first, i_what[1].second)));
    tstring member(to_wstring(std::string(i_what[2].first, i_what[2].second)));
    tstring value(to_wstring(std::string(i_what[3].first, i_what[3].second)));
#else
    tstring id(i_what[1].first, i_what[1].second);
    tstring member(i_what[2].first, i_what[2].second);
    tstring value(i_what[3].first, i_what[3].second);
#endif

    if (member == _T("path"))
      (*m_directSSTPServers)[id].m_path = value;
    else if (member == _T("hwnd"))
      (*m_directSSTPServers)[id].m_hwnd =
	reinterpret_cast<HWND>(_ttoi(value.c_str()));
    else if (member == _T("name"))
      (*m_directSSTPServers)[id].m_name = value;
    else if (member == _T("keroname"))
      (*m_directSSTPServers)[id].m_keroname = value;
    return true; 
  }
};

// Direct SSTP
void Engine::funcDirectSSTP(FunctionParam *i_param,
			    const tregex &i_name,
			    const StrExprArg &i_protocol,
			    const std::list<tstringq> &i_headers)
{
  if (!i_param->m_isPressed)
    return;

  // check Direct SSTP server exist ?
  if (HANDLE hm = OpenMutex(MUTEX_ALL_ACCESS, FALSE, _T("sakura")))
    CloseHandle(hm);
  else
  {
    Acquire a(&m_log, 0);
    m_log << _T(" Error(1): Direct SSTP server does not exist.");
    return;
  }

  HANDLE hfm = OpenFileMapping(FILE_MAP_READ, FALSE, _T("Sakura"));
  if (!hfm)
  {
    Acquire a(&m_log, 0);
    m_log << _T(" Error(2): Direct SSTP server does not provide data.");
    return;
  }
  
  char *data =
    reinterpret_cast<char *>(MapViewOfFile(hfm, FILE_MAP_READ, 0, 0, 0));
  if (!data)
  {
    CloseHandle(hfm);
    Acquire a(&m_log, 0);
    m_log << _T(" Error(3): Direct SSTP server does not provide data.");
    return;
  }
  
  long length = *(long *)data;
  const char *begin = data + 4;
  const char *end = data + length;
  boost::regex getSakura("([0-9a-fA-F]{32})\\.([^\x01]+)\x01(.*?)\r\n");
  
  ParseDirectSSTPData::DirectSSTPServers servers;
  boost::regex_iterator<boost::regex::const_iterator>
    it(begin, end, getSakura), last;
  for (; it != last; ++it)
    ((ParseDirectSSTPData)(&servers))(*it);

  // make request
  tstring request;
  if (!i_protocol.eval().size())
    request += _T("NOTIFY SSTP/1.1");
  else
    request += i_protocol.eval();
  request += _T("\r\n");

  bool hasSender = false;
  for (std::list<tstringq>::const_iterator
	 i = i_headers.begin(); i != i_headers.end(); ++ i)
  {
    if (_tcsnicmp(_T("Charset"), i->c_str(), 7) == 0 ||
	_tcsnicmp(_T("Hwnd"),    i->c_str(), 4) == 0)
      continue;
    if (_tcsnicmp(_T("Sender"), i->c_str(), 6) == 0)
      hasSender = true;
    request += i->c_str();
    request += _T("\r\n");
  }

  if (!hasSender)
  {
    request += _T("Sender: ");
    request += loadString(IDS_mayu);
    request += _T("\r\n");
  }
  
  _TCHAR buf[100];
  _sntprintf(buf, NUMBER_OF(buf), _T("HWnd: %d\r\n"),
	     reinterpret_cast<int>(m_hwndAssocWindow));
  request += buf;

#ifdef _UNICODE
  request += _T("Charset: UTF-8\r\n");
#else
  request += _T("Charset: Shift_JIS\r\n");
#endif
  request += _T("\r\n");

#ifdef _UNICODE
  std::string request_UTF_8 = to_UTF_8(request);
#endif

  // send request to Direct SSTP Server which matches i_name;
  for (ParseDirectSSTPData::DirectSSTPServers::iterator
	 i = servers.begin(); i != servers.end(); ++ i)
  {
    tsmatch what;
    if (boost::regex_match(i->second.m_name, what, i_name))
    {
      COPYDATASTRUCT cd;
      cd.dwData = 9801;
#ifdef _UNICODE
      cd.cbData = request_UTF_8.size();
      cd.lpData = (void *)request_UTF_8.c_str();
#else
      cd.cbData = request.size();
      cd.lpData = (void *)request.c_str();
#endif
      DWORD result;
      SendMessageTimeout(i->second.m_hwnd, WM_COPYDATA,
			 reinterpret_cast<WPARAM>(m_hwndAssocWindow),
			 reinterpret_cast<LPARAM>(&cd),
			 SMTO_ABORTIFHUNG | SMTO_BLOCK, 5000, &result);
    }
  }
  
  UnmapViewOfFile(data);
  CloseHandle(hfm);
}


namespace shu
{
  class PlugIn
  {
    enum Type
    {
      Type_A,
      Type_W
    };
    
  private:
    HMODULE m_dll;
    FARPROC m_func;
    Type m_type;
    tstringq m_funcParam;
    
  public:
    PlugIn() : m_dll(NULL)
    {
    }
    
    ~PlugIn()
    {
      FreeLibrary(m_dll);
    }

    bool load(const tstringq &i_dllName, const tstringq &i_funcName,
	      const tstringq &i_funcParam, tomsgstream &i_log)
    {
      m_dll = LoadLibrary((_T("Plugins\\") + i_dllName).c_str());
      if (!m_dll)
      {
	m_dll = LoadLibrary((_T("Plugin\\") + i_dllName).c_str());
	if (!m_dll)
	{
	  m_dll = LoadLibrary(i_dllName.c_str());
	  if (!m_dll)
	  {
	    Acquire a(&i_log);
	    i_log << std::endl;
	    i_log << _T("error: &PlugIn() failed to load ") << i_dllName << std::endl;
	    return false;
	  }
	}
      }

      // get function
#ifdef UNICODE
#  define to_wstring
#else
#  define to_string
#endif
      m_type = Type_W;
      m_func = GetProcAddress(m_dll, to_string(_T("mayu") + i_funcName + _T("W")).c_str());
      if (!m_func)
      {
	m_type = Type_A;
	m_func
	  = GetProcAddress(m_dll, to_string(_T("mayu") + i_funcName + _T("A")).c_str());
	if (!m_func)
	{
	  m_func = GetProcAddress(m_dll, to_string(_T("mayu") + i_funcName).c_str());
	  if (!m_func)
	  {
	    m_func = GetProcAddress(m_dll, to_string(i_funcName).c_str());
	    if (!m_func)
	    {
	      Acquire a(&i_log);
	      i_log << std::endl;
	      i_log << _T("error: &PlugIn() failed to find function: ")
		    << i_funcName << std::endl;
	      return false;
	    }
	  }
	}
      }
      
      m_funcParam = i_funcParam;
      return true;
    }

    void exec()
    {
      ASSERT( m_dll );
      ASSERT( m_func );
      
      typedef void (WINAPI * PLUGIN_FUNCTION_A)(const char *i_arg);
      typedef void (WINAPI * PLUGIN_FUNCTION_W)(const wchar_t *i_arg);
      switch (m_type)
      {
	case Type_A:
	  reinterpret_cast<PLUGIN_FUNCTION_A>(m_func)(to_string(m_funcParam).c_str());
	  break;
	case Type_W:
	  reinterpret_cast<PLUGIN_FUNCTION_W>(m_func)(to_wstring(m_funcParam).c_str());
	  break;
      }
    }
#undef to_string
#undef to_wstring
  };

  static void plugInThread(void *i_plugin)
  {
    PlugIn *plugin = static_cast<PlugIn *>(i_plugin);
    plugin->exec();
    delete plugin;
  }
}

void Engine::funcPlugIn(FunctionParam *i_param,
			const StrExprArg &i_dllName,
			const StrExprArg &i_funcName,
			const StrExprArg &i_funcParam,
			BooleanType i_doesCreateThread)
{
  if (!i_param->m_isPressed)
    return;

  shu::PlugIn *plugin = new shu::PlugIn();
  if (!plugin->load(i_dllName.eval(), i_funcName.eval(), i_funcParam.eval(), m_log))
  {
    delete plugin;
    return;
  }
  if (i_doesCreateThread)
  {
    if (_beginthread(shu::plugInThread, 0, plugin) == -1)
    {
      delete plugin;
      Acquire a(&m_log);
      m_log << std::endl;
      m_log << _T("error: &PlugIn() failed to create thread.");
    }
    return;
  }
  else
    plugin->exec();
}


void Engine::funcMouseHook(FunctionParam *i_param,
			   MouseHookType i_hookType, int i_hookParam)
{
  GetCursorPos(&g_hookData->m_mousePos);
  g_hookData->m_mouseHookType = i_hookType;
  g_hookData->m_mouseHookParam = i_hookParam;

  switch (i_hookType)
  {
    case MouseHookType_WindowMove:
    {
      // For this type, g_hookData->m_mouseHookParam means
      // target window type to move.
      HWND target;
      bool isMDI;

      // i_hooParam < 0 means target window to move is MDI.
      if (i_hookParam < 0)
	isMDI = true;
      else
	isMDI = false;

      // abs(i_hookParam) == 2: target is window under mouse cursor
      // otherwise: target is current focus window
      if (i_hookParam == 2 || i_hookParam == -2)
	target = WindowFromPoint(g_hookData->m_mousePos);
      else
	target = i_param->m_hwnd;

      g_hookData->m_hwndMouseHookTarget =
	getToplevelWindow(target, &isMDI);
      break;
    default:
      g_hookData->m_hwndMouseHookTarget = NULL;
      break;
    }
  }
  return;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// StrExpr
class StrExpr
{
private:
  tstringq m_symbol;
protected:
  static const Engine *s_engine;
public:
  StrExpr(const tstringq &i_symbol) : m_symbol(i_symbol) {};

  virtual ~StrExpr() {};

  virtual StrExpr *clone() const
  {
    return new StrExpr(*this);
  }

  virtual tstringq eval() const
  {
    return m_symbol;
  }

  static void setEngine(const Engine *i_engine) { s_engine = i_engine; }
};

const Engine *StrExpr::s_engine = NULL;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// StrExprClipboard
class StrExprClipboard : public StrExpr
{
public:
  StrExprClipboard(const tstringq &i_symbol) : StrExpr(i_symbol) {};

  ~StrExprClipboard() {};

  StrExpr *clone() const
  {
    return new StrExprClipboard(*this);
  }

  tstringq eval() const
  {
    HGLOBAL g;
    const _TCHAR *text = getTextFromClipboard(&g);
    const tstring value(text == NULL ? _T("") : text);
    closeClipboard(g);
    return value;
  }
};


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// StrExprWindowClassName
class StrExprWindowClassName : public StrExpr
{
public:
  StrExprWindowClassName(const tstringq &i_symbol) : StrExpr(i_symbol) {};

  ~StrExprWindowClassName() {};

  StrExpr *clone() const
  {
    return new StrExprWindowClassName(*this);
  }

  tstringq eval() const
  {
    return s_engine->getCurrentWindowClassName();
  }
};


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// StrExprWindowTitleName
class StrExprWindowTitleName : public StrExpr
{
public:
  StrExprWindowTitleName(const tstringq &i_symbol) : StrExpr(i_symbol) {};

  ~StrExprWindowTitleName() {};

  StrExpr *clone() const
  {
    return new StrExprWindowTitleName(*this);
  }

  tstringq eval() const
  {
    return s_engine->getCurrentWindowTitleName();
  }
};


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// StrExprArg


// default constructor
StrExprArg::StrExprArg()
{
  m_expr = new StrExpr(_T(""));
}


// copy contructor
StrExprArg::StrExprArg(const StrExprArg &i_data)
{
  m_expr = i_data.m_expr->clone();
}


StrExprArg &StrExprArg::operator=(const StrExprArg &i_data)
{
  if (i_data.m_expr == m_expr)
    return *this;

  delete m_expr;
  m_expr = i_data.m_expr->clone();

  return *this;
}


// initializer
StrExprArg::StrExprArg(const tstringq &i_symbol, Type i_type)
{
  switch (i_type)
  {
    case Literal:
      m_expr = new StrExpr(i_symbol);
      break;
    case Builtin:
      if (i_symbol == _T("Clipboard"))
	m_expr = new StrExprClipboard(i_symbol);
      else if (i_symbol == _T("WindowClassName"))
	m_expr = new StrExprWindowClassName(i_symbol);
      else if (i_symbol == _T("WindowTitleName"))
	m_expr = new StrExprWindowTitleName(i_symbol);
      break;
    default:
      break;
  }
}


StrExprArg::~StrExprArg()
{
  delete m_expr;
}


tstringq StrExprArg::eval() const
{
  return m_expr->eval();
}

void StrExprArg::setEngine(const Engine *i_engine)
{
  StrExpr::setEngine(i_engine);
}

// stream output
tostream &operator<<(tostream &i_ost, const StrExprArg &i_data)
{
  i_ost << i_data.eval();
  return i_ost;
}
