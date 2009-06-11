//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// setting.cpp


#include "misc.h"

#include "dlgsetting.h"
#include "errormessage.h"
#include "mayu.h"
#include "mayurc.h"
#include "registry.h"
#include "setting.h"
#include "windowstool.h"
#include "vkeytable.h"
#include "array.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sys/stat.h>


namespace Event
{
  Key prefixed(_T("prefixed"));
  Key before_key_down(_T("before-key-down"));
  Key after_key_up(_T("after-key-up"));
  Key *events[] =
  {
    &prefixed,
    &before_key_down,
    &after_key_up,
    NULL,
  };
}


// get mayu filename
static bool getFilenameFromRegistry(
  tstringi *o_name, tstringi *o_filename, Setting::Symbols *o_symbols)
{
  Registry reg(MAYU_REGISTRY_ROOT);
  int index;
  reg.read(_T(".mayuIndex"), &index, 0);
  _TCHAR buf[100];
  _sntprintf(buf, NUMBER_OF(buf), _T(".mayu%d"), index);

  tstringi entry;
  if (!reg.read(buf, &entry))
    return false;
  
  tregex getFilename(_T("^([^;]*);([^;]*);(.*)$"));
  tsmatch getFilenameResult;
  if (!boost::regex_match(entry, getFilenameResult, getFilename))
    return false;
  
  if (o_name)
    *o_name = getFilenameResult.str(1);
  if (o_filename)
    *o_filename = getFilenameResult.str(2);
  if (o_symbols)
  {
    tstringi symbols = getFilenameResult.str(3);
    tregex symbol(_T("-D([^;]*)(.*)$"));
    tsmatch symbolResult;
    while (boost::regex_search(symbols, symbolResult, symbol))
    {
      o_symbols->insert(symbolResult.str(1));
      symbols = symbolResult.str(2);
    }
  }
  return true;
}


// get home directory path
void getHomeDirectories(HomeDirectories *o_pathes)
{
  tstringi filename;
  if (getFilenameFromRegistry(NULL, &filename, NULL) &&
      !filename.empty())
  {
    tregex getPath(_T("^(.*[/\\\\])[^/\\\\]*$"));
    tsmatch getPathResult;
    if (boost::regex_match(filename, getPathResult, getPath))
      o_pathes->push_back(getPathResult.str(1));
  }
  
  const _TCHAR *home = _tgetenv(_T("HOME"));
  if (home)
    o_pathes->push_back(home);

  const _TCHAR *homedrive = _tgetenv(_T("HOMEDRIVE"));
  const _TCHAR *homepath = _tgetenv(_T("HOMEPATH"));
  if (homedrive && homepath)
    o_pathes->push_back(tstringi(homedrive) + homepath);
  
  const _TCHAR *userprofile = _tgetenv(_T("USERPROFILE"));
  if (userprofile)
    o_pathes->push_back(userprofile);
  
  _TCHAR buf[GANA_MAX_PATH];
  DWORD len = GetCurrentDirectory(NUMBER_OF(buf), buf);
  if (0 < len && len < NUMBER_OF(buf))
    o_pathes->push_back(buf);

  if (GetModuleFileName(GetModuleHandle(NULL), buf, NUMBER_OF(buf)))
    o_pathes->push_back(pathRemoveFileSpec(buf));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// SettingLoader


// is there no more tokens ?
bool SettingLoader::isEOL()
{
  return m_ti == m_tokens.end();
}


// get next token
Token *SettingLoader::getToken()
{
  if (isEOL())
    throw ErrorMessage() << _T("too few words.");
  return &*(m_ti ++);
}

  
// look next token
Token *SettingLoader::lookToken()
{
  if (isEOL())
    throw ErrorMessage() << _T("too few words.");
  return &*m_ti;
}


// argument "("
bool SettingLoader::getOpenParen(bool i_doesThrow, const _TCHAR *i_name)
{
  if (!isEOL() && lookToken()->isOpenParen())
  {
    getToken();
    return true;
  }
  if (i_doesThrow)
    throw ErrorMessage() << _T("there must be `(' after `&")
			 << i_name << _T("'.");
  return false;
}


// argument ")"
bool SettingLoader::getCloseParen(bool i_doesThrow, const _TCHAR *i_name)
{
  if (!isEOL() && lookToken()->isCloseParen())
  {
    getToken();
    return true;
  }
  if (i_doesThrow)
    throw ErrorMessage() << _T("`&")  << i_name
			 << _T("': too many arguments.");
  return false;
}


// argument ","
bool SettingLoader::getComma(bool i_doesThrow, const _TCHAR *i_name)
{
  if (!isEOL() && lookToken()->isComma())
  {
    getToken();
    return true;
  }
  if (i_doesThrow)
    throw ErrorMessage() << _T("`&")  << i_name
			 << _T("': comma expected.");
  return false;
}


// <INCLUDE>
void SettingLoader::load_INCLUDE()
{
  SettingLoader loader(m_soLog, m_log);
  loader.m_defaultAssignModifier = m_defaultAssignModifier;
  loader.m_defaultKeySeqModifier = m_defaultKeySeqModifier;
  if (!loader.load(m_setting, (*getToken()).getString()))
    m_isThereAnyError = true;
}


// <SCAN_CODES>
void SettingLoader::load_SCAN_CODES(Key *o_key)
{
  for (int j = 0; j < Key::MAX_SCAN_CODES_SIZE && !isEOL(); ++ j)
  {
    ScanCode sc;
    sc.m_flags = 0;
    while (true)
    {
      Token *t = getToken();
      if (t->isNumber())
      {
	sc.m_scan = (u_char)t->getNumber();
	o_key->addScanCode(sc);
	break;
      }
      if      (*t == _T("E0-")) sc.m_flags |= ScanCode::E0;
      else if (*t == _T("E1-")) sc.m_flags |= ScanCode::E1;
      else  throw ErrorMessage() << _T("`") << *t
				 << _T("': invalid modifier.");
    }
  }
}


// <DEFINE_KEY>
void SettingLoader::load_DEFINE_KEY()
{
  Token *t = getToken();
  Key key;
      
  // <KEY_NAMES>
  if (*t == _T('('))
  {
    key.addName(getToken()->getString());
    while (t = getToken(), *t != _T(')'))
      key.addName(t->getString());
    if (*getToken() != _T("="))
      throw ErrorMessage() << _T("there must be `=' after `)'.");
  }
  else
  {
    key.addName(t->getString());
    while (t = getToken(), *t != _T("="))
      key.addName(t->getString());
  }

  load_SCAN_CODES(&key);
  m_setting->m_keyboard.addKey(key);
}


// <DEFINE_MODIFIER>
void SettingLoader::load_DEFINE_MODIFIER()
{
  Token *t = getToken();
  Modifier::Type mt;
  if      (*t == _T("shift")  ) mt = Modifier::Type_Shift;
  else if (*t == _T("alt")     ||
	   *t == _T("meta")    ||
	   *t == _T("menu")   ) mt = Modifier::Type_Alt;
  else if (*t == _T("control") ||
	   *t == _T("ctrl")   ) mt = Modifier::Type_Control;
  else if (*t == _T("windows") ||
	   *t == _T("win")    ) mt = Modifier::Type_Windows;
  else throw ErrorMessage() << _T("`") << *t
			    << _T("': invalid modifier name.");
    
  if (*getToken() != _T("="))
    throw ErrorMessage() << _T("there must be `=' after modifier name.");
    
  while (!isEOL())
  {
    t = getToken();
    Key *key =
      m_setting->m_keyboard.searchKeyByNonAliasName(t->getString());
    if (!key)
      throw ErrorMessage() << _T("`") << *t << _T("': invalid key name.");
    m_setting->m_keyboard.addModifier(mt, key);
  }
}


// <DEFINE_SYNC_KEY>
void SettingLoader::load_DEFINE_SYNC_KEY()
{
  Key *key = m_setting->m_keyboard.getSyncKey();
  key->initialize();
  key->addName(_T("sync"));
  
  if (*getToken() != _T("="))
    throw ErrorMessage() << _T("there must be `=' after `sync'.");
  
  load_SCAN_CODES(key);
}


// <DEFINE_ALIAS>
void SettingLoader::load_DEFINE_ALIAS()
{
  Token *name = getToken();
  
  if (*getToken() != _T("="))
    throw ErrorMessage() << _T("there must be `=' after `alias'.");

  Token *t = getToken();
  Key *key = m_setting->m_keyboard.searchKeyByNonAliasName(t->getString());
  if (!key)
    throw ErrorMessage() << _T("`") << *t << _T("': invalid key name.");
  m_setting->m_keyboard.addAlias(name->getString(), key);
}


// <DEFINE_SUBSTITUTE>
void SettingLoader::load_DEFINE_SUBSTITUTE()
{
  typedef std::list<ModifiedKey> AssignedKeys;
  AssignedKeys assignedKeys;
  do
  {
    ModifiedKey mkey;
    mkey.m_modifier =
      load_MODIFIER(Modifier::Type_ASSIGN, m_defaultAssignModifier);
    mkey.m_key = load_KEY_NAME();
    assignedKeys.push_back(mkey);
  } while (!(*lookToken() == _T("=>") || *lookToken() == _T("=")));
  getToken();
  
  KeySeq *keySeq = load_KEY_SEQUENCE(_T(""), false, Modifier::Type_ASSIGN);
  ModifiedKey mkey = keySeq->getFirstModifiedKey();
  if (!mkey.m_key)
    throw ErrorMessage() << _T("no key is specified for substitute.");
  
  for (AssignedKeys::iterator i = assignedKeys.begin();
       i != assignedKeys.end(); ++ i)
    m_setting->m_keyboard.addSubstitute(*i, mkey);
}


// <DEFINE_OPTION>
void SettingLoader::load_DEFINE_OPTION()
{
  Token *t = getToken();
  if (*t == _T("KL-")) {
    if (*getToken() != _T("=")) {
      throw ErrorMessage() << _T("there must be `=' after `def option KL-'.");
    }

    load_ARGUMENT(&m_setting->m_correctKanaLockHandling);
    
  } else if (*t == _T("delay-of")) {
    if (*getToken() != _T("!!!")) {
      throw ErrorMessage()
	<< _T("there must be `!!!' after `def option delay-of'.");
    }
    
    if (*getToken() != _T("=")) {
      throw ErrorMessage()
	<< _T("there must be `=' after `def option delay-of !!!'.");
    }
    
    load_ARGUMENT(&m_setting->m_oneShotRepeatableDelay);
    
  } else if (*t == _T("sts4mayu")) {
    if (*getToken() != _T("=")) {
      throw ErrorMessage()
	<< _T("there must be `=' after `def option sts4mayu'.");
    }
    
    load_ARGUMENT(&m_setting->m_sts4mayu);
    
  } else if (*t == _T("cts4mayu")) {
    if (*getToken() != _T("=")) {
      throw ErrorMessage()
	<< _T("there must be `=' after `def option cts4mayu'.");
    }
    
    load_ARGUMENT(&m_setting->m_cts4mayu);
    
  } else {
    throw ErrorMessage() << _T("syntax error `def option ") << *t << _T("'.");
  }
}



// <KEYBOARD_DEFINITION>
void SettingLoader::load_KEYBOARD_DEFINITION()
{
  Token *t = getToken();
    
  // <DEFINE_KEY>
  if (*t == _T("key")) load_DEFINE_KEY();

  // <DEFINE_MODIFIER>
  else if (*t == _T("mod")) load_DEFINE_MODIFIER();
  
  // <DEFINE_SYNC_KEY>
  else if (*t == _T("sync")) load_DEFINE_SYNC_KEY();
  
  // <DEFINE_ALIAS>
  else if (*t == _T("alias")) load_DEFINE_ALIAS();
  
  // <DEFINE_SUBSTITUTE>
  else if (*t == _T("subst")) load_DEFINE_SUBSTITUTE();

  // <DEFINE_OPTION>
  else if (*t == _T("option")) load_DEFINE_OPTION();
  
  //
  else throw ErrorMessage() << _T("syntax error `") << *t << _T("'.");
}


// <..._MODIFIER>
Modifier SettingLoader::load_MODIFIER(
  Modifier::Type i_mode, Modifier i_modifier, Modifier::Type *o_mode)
{
  if (o_mode)
    *o_mode = Modifier::Type_begin;
  
  Modifier isModifierSpecified;
  enum { PRESS, RELEASE, DONTCARE } flag = PRESS;

  int i;
  for (i = i_mode; i < Modifier::Type_ASSIGN; ++ i)
  {
    i_modifier.dontcare(Modifier::Type(i));
    isModifierSpecified.on(Modifier::Type(i));
  }
  
  Token *t = NULL;
  
  continue_loop:
  while (!isEOL())
  {
    t = lookToken();

    const static struct { const _TCHAR *m_s; Modifier::Type m_mt; } map[] =
    {
      // <BASIC_MODIFIER>
      { _T("S-"),  Modifier::Type_Shift },
      { _T("A-"),  Modifier::Type_Alt },
      { _T("M-"),  Modifier::Type_Alt },
      { _T("C-"),  Modifier::Type_Control },
      { _T("W-"),  Modifier::Type_Windows },
      // <KEYSEQ_MODIFIER>
      { _T("U-"),  Modifier::Type_Up },
      { _T("D-"),  Modifier::Type_Down },
      // <ASSIGN_MODIFIER>
      { _T("R-"),  Modifier::Type_Repeat },
      { _T("IL-"), Modifier::Type_ImeLock },
      { _T("IC-"), Modifier::Type_ImeComp },
      { _T("I-"),  Modifier::Type_ImeComp },
      { _T("NL-"), Modifier::Type_NumLock },
      { _T("CL-"), Modifier::Type_CapsLock },
      { _T("SL-"), Modifier::Type_ScrollLock },
      { _T("KL-"), Modifier::Type_KanaLock },
      { _T("MAX-"), Modifier::Type_Maximized },
      { _T("MIN-"), Modifier::Type_Minimized },
      { _T("MMAX-"), Modifier::Type_MdiMaximized },
      { _T("MMIN-"), Modifier::Type_MdiMinimized },
      { _T("T-"), Modifier::Type_Touchpad },
      { _T("TS-"), Modifier::Type_TouchpadSticky },
      { _T("M0-"), Modifier::Type_Mod0 },
      { _T("M1-"), Modifier::Type_Mod1 },
      { _T("M2-"), Modifier::Type_Mod2 },
      { _T("M3-"), Modifier::Type_Mod3 },
      { _T("M4-"), Modifier::Type_Mod4 },
      { _T("M5-"), Modifier::Type_Mod5 },
      { _T("M6-"), Modifier::Type_Mod6 },
      { _T("M7-"), Modifier::Type_Mod7 },
      { _T("M8-"), Modifier::Type_Mod8 },
      { _T("M9-"), Modifier::Type_Mod9 },
      { _T("L0-"), Modifier::Type_Lock0 },
      { _T("L1-"), Modifier::Type_Lock1 },
      { _T("L2-"), Modifier::Type_Lock2 },
      { _T("L3-"), Modifier::Type_Lock3 },
      { _T("L4-"), Modifier::Type_Lock4 },
      { _T("L5-"), Modifier::Type_Lock5 },
      { _T("L6-"), Modifier::Type_Lock6 },
      { _T("L7-"), Modifier::Type_Lock7 },
      { _T("L8-"), Modifier::Type_Lock8 },
      { _T("L9-"), Modifier::Type_Lock9 },
    };

    for (int i = 0; i < NUMBER_OF(map); ++ i)
      if (*t == map[i].m_s)
      {
	getToken();
	Modifier::Type mt = map[i].m_mt;
	if (static_cast<int>(i_mode) <= static_cast<int>(mt))
	  throw ErrorMessage() << _T("`") << *t
			       << _T("': invalid modifier at this context.");
	switch (flag)
	{
	  case PRESS: i_modifier.press(mt); break;
	  case RELEASE: i_modifier.release(mt); break;
	  case DONTCARE: i_modifier.dontcare(mt); break;
	}
	isModifierSpecified.on(mt);
	flag = PRESS;
	
	if (o_mode && *o_mode < mt)
	{
	  if (mt < Modifier::Type_BASIC)
	    *o_mode = Modifier::Type_BASIC;
	  else if (mt < Modifier::Type_KEYSEQ)
	    *o_mode = Modifier::Type_KEYSEQ;
	  else if (mt < Modifier::Type_ASSIGN)
	    *o_mode = Modifier::Type_ASSIGN;
	}
	goto continue_loop;
      }
      
    if (*t == _T("*"))
    {
      getToken();
      flag = DONTCARE;
      continue;
    }
      
    if (*t == _T("~"))
    {
      getToken();
      flag = RELEASE;
      continue;
    }

    break;
  }
  
  for (i = Modifier::Type_begin; i != Modifier::Type_end; ++ i)
    if (!isModifierSpecified.isOn(Modifier::Type(i)))
      switch (flag)
      {
	case PRESS: break;
	case RELEASE: i_modifier.release(Modifier::Type(i)); break;
	case DONTCARE: i_modifier.dontcare(Modifier::Type(i)); break;
      }

  // fix up and down
  bool isDontcareUp   = i_modifier.isDontcare(Modifier::Type_Up);
  bool isDontcareDown = i_modifier.isDontcare(Modifier::Type_Down);
  bool isOnUp         = i_modifier.isOn(Modifier::Type_Up);
  bool isOnDown       = i_modifier.isOn(Modifier::Type_Down);
  if (isDontcareUp && isDontcareDown)
    ;
  else if (isDontcareUp)
    i_modifier.on(Modifier::Type_Up, !isOnDown);
  else if (isDontcareDown)
    i_modifier.on(Modifier::Type_Down, !isOnUp);
  else if (isOnUp == isOnDown)
  {
    i_modifier.dontcare(Modifier::Type_Up);
    i_modifier.dontcare(Modifier::Type_Down);
  }

  // fix repeat
  if (!isModifierSpecified.isOn(Modifier::Type_Repeat))
    i_modifier.dontcare(Modifier::Type_Repeat);
  return i_modifier;
}


// <KEY_NAME>
Key *SettingLoader::load_KEY_NAME()
{
  Token *t = getToken();
  Key *key = m_setting->m_keyboard.searchKey(t->getString());
  if (!key)
    throw ErrorMessage() << _T("`") << *t << _T("': invalid key name.");
  return key;
}


// <KEYMAP_DEFINITION>
void SettingLoader::load_KEYMAP_DEFINITION(const Token *i_which)
{
  Keymap::Type type = Keymap::Type_keymap;
  Token *name = getToken();	// <KEYMAP_NAME>
  tstringi windowClassName;
  tstringi windowTitleName;
  KeySeq *keySeq = NULL;
  Keymap *parentKeymap = NULL;
  bool isKeymap2 = false;
  bool doesLoadDefaultKeySeq = false;

  if (!isEOL())
  {
    Token *t = lookToken();
    if (*i_which == _T("window"))	// <WINDOW>
    {
      if (t->isOpenParen())
	// "(" <WINDOW_CLASS_NAME> "&&" <WINDOW_TITLE_NAME> ")"
	// "(" <WINDOW_CLASS_NAME> "||" <WINDOW_TITLE_NAME> ")"
      {
	getToken();
	windowClassName = getToken()->getRegexp();
	t = getToken();
	if (*t == _T("&&"))
	  type = Keymap::Type_windowAnd;
	else if (*t == _T("||"))
	  type = Keymap::Type_windowOr;
	else
	  throw ErrorMessage() << _T("`") << *t << _T("': unknown operator.");
	windowTitleName = getToken()->getRegexp();
	if (!getToken()->isCloseParen())
	  throw ErrorMessage() << _T("there must be `)'.");
      }
      else if (t->isRegexp())	// <WINDOW_CLASS_NAME>
      {
	getToken();
	type = Keymap::Type_windowAnd;
	windowClassName = t->getRegexp();
      }
    }
    else if (*i_which == _T("keymap"))
      ;
    else if (*i_which == _T("keymap2"))
      isKeymap2 = true;
    else
      ASSERT(false);
    
    if (!isEOL())
      doesLoadDefaultKeySeq = true;
  }

  m_currentKeymap = m_setting->m_keymaps.add(
    Keymap(type, name->getString(), windowClassName, windowTitleName,
	   NULL, NULL));

  if (doesLoadDefaultKeySeq)
  {
    Token *t = lookToken();
    // <KEYMAP_PARENT>
    if (*t == _T(":"))
    {
      getToken();
      t = getToken();
      parentKeymap = m_setting->m_keymaps.searchByName(t->getString());
      if (!parentKeymap)
	throw ErrorMessage() << _T("`") << *t
			     << _T("': unknown keymap name.");
    }
    if (!isEOL())
    {
      t = getToken();
      if (!(*t == _T("=>") || *t == _T("=")))
	throw ErrorMessage() << _T("`") << *t << _T("': syntax error.");
      keySeq = SettingLoader::load_KEY_SEQUENCE();
    }
  }
  if (keySeq == NULL)
  {
    FunctionData *fd;
    if (type == Keymap::Type_keymap && !isKeymap2)
      fd = createFunctionData(_T("KeymapParent"));
    else if (type == Keymap::Type_keymap && !isKeymap2)
      fd = createFunctionData(_T("Undefined"));
    else // (type == Keymap::Type_windowAnd || type == Keymap::Type_windowOr)
      fd = createFunctionData(_T("KeymapParent"));
    ASSERT( fd );
    keySeq = m_setting->m_keySeqs.add(
      KeySeq(name->getString()).add(ActionFunction(fd)));
  }

  m_currentKeymap->setIfNotYet(keySeq, parentKeymap);
}


// &lt;ARGUMENT&gt;
void SettingLoader::load_ARGUMENT(bool *o_arg)
{
  *o_arg = !(*getToken() == _T("false"));
}


// &lt;ARGUMENT&gt;
void SettingLoader::load_ARGUMENT(int *o_arg)
{
  *o_arg = getToken()->getNumber();
}


// &lt;ARGUMENT&gt;
void SettingLoader::load_ARGUMENT(unsigned int *o_arg)
{
  *o_arg = getToken()->getNumber();
}


// &lt;ARGUMENT&gt;
void SettingLoader::load_ARGUMENT(long *o_arg)
{
  *o_arg = getToken()->getNumber();
}


// &lt;ARGUMENT&gt;
void SettingLoader::load_ARGUMENT(tstringq *o_arg)
{
  *o_arg = getToken()->getString();
}


// &lt;ARGUMENT&gt;
void SettingLoader::load_ARGUMENT(std::list<tstringq> *o_arg)
{
  while (true)
  {
    if (!lookToken()->isString())
      return;
    o_arg->push_back(getToken()->getString());
    
    if (!lookToken()->isComma())
      return;
    getToken();
  }
}


// &lt;ARGUMENT&gt;
void SettingLoader::load_ARGUMENT(tregex *o_arg)
{
  *o_arg = getToken()->getRegexp();
}


// &lt;ARGUMENT_VK&gt;
void SettingLoader::load_ARGUMENT(VKey *o_arg)
{
  Token *t = getToken();
  int vkey = 0;
  while (true)
  {
    if (t->isNumber()) { vkey |= static_cast<BYTE>(t->getNumber()); break; }
    else if (*t == _T("E-")) vkey |= VKey_extended;
    else if (*t == _T("U-")) vkey |= VKey_released;
    else if (*t == _T("D-")) vkey |= VKey_pressed;
    else
    {
      const VKeyTable *vkt;
      for (vkt = g_vkeyTable; vkt->m_name; ++ vkt)
	if (*t == vkt->m_name)
	  break;
      if (!vkt->m_name)
	throw ErrorMessage() << _T("`") << *t
			     << _T("': unknown virtual key name.");
      vkey |= vkt->m_code;
      break;
    }
    t = getToken();
  }
  if (!(vkey & VKey_released) && !(vkey & VKey_pressed))
    vkey |= VKey_released | VKey_pressed;
  *o_arg = static_cast<VKey>(vkey);
}


// &lt;ARGUMENT_WINDOW&gt;
void SettingLoader::load_ARGUMENT(ToWindowType *o_arg)
{
  Token *t = getToken();
  if (t->isNumber())
  {
    if (ToWindowType_toBegin <= t->getNumber())
    {
      *o_arg = static_cast<ToWindowType>(t->getNumber());
      return;
    }
  }
  else if (getTypeValue(o_arg, t->getString()))
    return;
  throw ErrorMessage() << _T("`") << *t << _T("': invalid target window.");
}


// &lt;ARGUMENT&gt;
void SettingLoader::load_ARGUMENT(GravityType *o_arg)
{
  Token *t = getToken();
  if (getTypeValue(o_arg, t->getString()))
    return;
  throw ErrorMessage() << _T("`") << *t << _T("': unknown gravity symbol.");
}


// &lt;ARGUMENT&gt;
void SettingLoader::load_ARGUMENT(MouseHookType *o_arg)
{
  Token *t = getToken();
  if (getTypeValue(o_arg, t->getString()))
    return;
  throw ErrorMessage() << _T("`") << *t << _T("': unknown MouseHookType symbol.");
}


// &lt;ARGUMENT&gt;
void SettingLoader::load_ARGUMENT(MayuDialogType *o_arg)
{
  Token *t = getToken();
  if (getTypeValue(o_arg, t->getString()))
    return;
  throw ErrorMessage() << _T("`") << *t << _T("': unknown dialog box.");
}


// &lt;ARGUMENT_LOCK&gt;
void SettingLoader::load_ARGUMENT(ModifierLockType *o_arg)
{
  Token *t = getToken();
  if (getTypeValue(o_arg, t->getString()))
    return;
  throw ErrorMessage() << _T("`") << *t << _T("': unknown lock name.");
}


// &lt;ARGUMENT_LOCK&gt;
void SettingLoader::load_ARGUMENT(ToggleType *o_arg)
{
  Token *t = getToken();
  if (getTypeValue(o_arg, t->getString()))
    return;
  throw ErrorMessage() << _T("`") << *t << _T("': unknown toggle name.");
}


// &lt;ARGUMENT_SHOW_WINDOW&gt;
void SettingLoader::load_ARGUMENT(ShowCommandType *o_arg)
{
  Token *t = getToken();
  if (getTypeValue(o_arg, t->getString()))
    return;
  throw ErrorMessage() << _T("`") << *t << _T("': unknown show command.");
}


// &lt;ARGUMENT_TARGET_WINDOW&gt;
void SettingLoader::load_ARGUMENT(TargetWindowType *o_arg)
{
  Token *t = getToken();
  if (getTypeValue(o_arg, t->getString()))
    return;
  throw ErrorMessage() << _T("`") << *t
		       << _T("': unknown target window type.");
}


// &lt;bool&gt;
void SettingLoader::load_ARGUMENT(BooleanType *o_arg)
{
  Token *t = getToken();
  if (getTypeValue(o_arg, t->getString()))
    return;
  throw ErrorMessage() << _T("`") << *t << _T("': must be true or false.");
}


// &lt;ARGUMENT&gt;
void SettingLoader::load_ARGUMENT(LogicalOperatorType *o_arg)
{
  Token *t = getToken();
  if (getTypeValue(o_arg, t->getString()))
    return;
  throw ErrorMessage() << _T("`") << *t << _T("': must be 'or' or 'and'.");
}


// &lt;ARGUMENT&gt;
void SettingLoader::load_ARGUMENT(Modifier *o_arg)
{
  Modifier modifier;
  for (int i = Modifier::Type_begin; i != Modifier::Type_end; ++ i)
    modifier.dontcare(static_cast<Modifier::Type>(i));
  *o_arg = load_MODIFIER(Modifier::Type_ASSIGN, modifier);
}


// &lt;ARGUMENT&gt;
void SettingLoader::load_ARGUMENT(const Keymap **o_arg)
{
  Token *t = getToken();
  const Keymap *&keymap = *o_arg;
  keymap = m_setting->m_keymaps.searchByName(t->getString());
  if (!keymap)
    throw ErrorMessage() << _T("`") << *t << _T("': unknown keymap name.");
}


// &lt;ARGUMENT&gt;
void SettingLoader::load_ARGUMENT(const KeySeq **o_arg)
{
  Token *t = getToken();
  const KeySeq *&keySeq = *o_arg;
  if (t->isOpenParen())
  {
    keySeq = load_KEY_SEQUENCE(_T(""), true);
    getToken(); // close paren
  }
  else if (*t == _T("$"))
  {
    t = getToken();
    keySeq = m_setting->m_keySeqs.searchByName(t->getString());
    if (!keySeq)
      throw ErrorMessage() << _T("`$") << *t << _T("': unknown keyseq name.");
  }
  else
    throw ErrorMessage() << _T("`") << *t << _T("': it is not keyseq.");
}


// &lt;ARGUMENT&gt;
void SettingLoader::load_ARGUMENT(StrExprArg *o_arg)
{
  Token *t = getToken();
  StrExprArg::Type type = StrExprArg::Literal;
  if (*t == _T("$") && t->isQuoted() == false
      && lookToken()->getType() == Token::Type_string)
  {
    type = StrExprArg::Builtin;
    t = getToken();
  }
  *o_arg = StrExprArg(t->getString(), type);
}


// &lt;ARGUMENT&gt;
void SettingLoader::load_ARGUMENT(WindowMonitorFromType *o_arg)
{
  Token *t = getToken();
  if (getTypeValue(o_arg, t->getString()))
    return;
  throw ErrorMessage() << _T("`") << *t
		       << _T("': unknown monitor from type.");
}


// <KEY_SEQUENCE>
KeySeq *SettingLoader::load_KEY_SEQUENCE(
  const tstringi &i_name, bool i_isInParen, Modifier::Type i_mode)
{
  KeySeq keySeq(i_name);
  while (!isEOL())
  {
    Modifier::Type mode;
    Modifier modifier = load_MODIFIER(i_mode, m_defaultKeySeqModifier, &mode);
    keySeq.setMode(mode);
    Token *t = lookToken();
    if (t->isCloseParen() && i_isInParen)
      break;
    else if (t->isOpenParen())
    {
      getToken(); // open paren
      KeySeq *ks = load_KEY_SEQUENCE(_T(""), true, i_mode);
      getToken(); // close paren
      keySeq.add(ActionKeySeq(ks));
    }
    else if (*t == _T("$")) // <KEYSEQ_NAME>
    {
      getToken();
      t = getToken();
      KeySeq *ks = m_setting->m_keySeqs.searchByName(t->getString());
      if (ks == NULL)
	throw ErrorMessage() << _T("`$") << *t
			     << _T("': unknown keyseq name.");
      if (!ks->isCorrectMode(i_mode))
	throw ErrorMessage()
	  << _T("`$") << *t
	  << _T("': Some of R-, IL-, IC-, NL-, CL-, SL-, KL-, MAX-, MIN-, MMAX-, MMIN-, T-, TS-, M0...M9- and L0...L9- are used in the keyseq.  They are prohibited in this context.");
      keySeq.setMode(ks->getMode());
      keySeq.add(ActionKeySeq(ks));
    }
    else if (*t == _T("&")) // <FUNCTION_NAME>
    {
      getToken();
      t = getToken();
      
      // search function
      ActionFunction af(createFunctionData(t->getString()), modifier);
      if (af.m_functionData == NULL)
	throw ErrorMessage() << _T("`&") << *t
			     << _T("': unknown function name.");
      af.m_functionData->load(this);
      keySeq.add(af);
    }
    else // <KEYSEQ_MODIFIED_KEY_NAME>
    {
      ModifiedKey mkey;
      mkey.m_modifier = modifier;
      mkey.m_key = load_KEY_NAME();
      keySeq.add(ActionKey(mkey));
    }
  }
  return m_setting->m_keySeqs.add(keySeq);
}


// <KEY_ASSIGN>
void SettingLoader::load_KEY_ASSIGN()
{
  typedef std::list<ModifiedKey> AssignedKeys;
  AssignedKeys assignedKeys;
  
  ModifiedKey mkey;
  mkey.m_modifier =
    load_MODIFIER(Modifier::Type_ASSIGN, m_defaultAssignModifier);
  if (*lookToken() == _T("="))
  {
    getToken();
    m_defaultKeySeqModifier = load_MODIFIER(Modifier::Type_KEYSEQ,
					    m_defaultKeySeqModifier);
    m_defaultAssignModifier = mkey.m_modifier;
    return;
  }
  
  while (true)
  {
    mkey.m_key = load_KEY_NAME();
    assignedKeys.push_back(mkey);
    if (*lookToken() == _T("=>") || *lookToken() == _T("="))
      break;
    mkey.m_modifier =
      load_MODIFIER(Modifier::Type_ASSIGN, m_defaultAssignModifier);
  }
  getToken();

  ASSERT(m_currentKeymap);
  KeySeq *keySeq = load_KEY_SEQUENCE();
  for (AssignedKeys::iterator i = assignedKeys.begin();
       i != assignedKeys.end(); ++ i)
    m_currentKeymap->addAssignment(*i, keySeq);
}


// <EVENT_ASSIGN>
void SettingLoader::load_EVENT_ASSIGN()
{
  std::list<ModifiedKey> assignedKeys;

  ModifiedKey mkey;
  mkey.m_modifier.dontcare();			//set all modifiers to dontcare
  
  Token *t = getToken();
  Key **e;
  for (e = Event::events; *e; ++ e)
    if (*t == (*e)->getName())
    {
      mkey.m_key = *e;
      break;
    }
  if (!*e)
    throw ErrorMessage() << _T("`") << *t << _T("': invalid event name.");

  t = getToken();
  if (!(*t == _T("=>") || *t == _T("=")))
    throw ErrorMessage() << _T("`=' is expected.");

  ASSERT(m_currentKeymap);
  KeySeq *keySeq = load_KEY_SEQUENCE();
  m_currentKeymap->addAssignment(mkey, keySeq);
}


// <MODIFIER_ASSIGNMENT>
void SettingLoader::load_MODIFIER_ASSIGNMENT()
{
  // <MODIFIER_NAME>
  Token *t = getToken();
  Modifier::Type mt;

  while (true)
  {
    Keymap::AssignMode am = Keymap::AM_notModifier;
    if      (*t == _T("!")  ) am = Keymap::AM_true, t = getToken();
    else if (*t == _T("!!") ) am = Keymap::AM_oneShot, t = getToken();
    else if (*t == _T("!!!")) am = Keymap::AM_oneShotRepeatable, t = getToken();
    
    if      (*t == _T("shift")) mt = Modifier::Type_Shift;
    else if (*t == _T("alt")  ||
	     *t == _T("meta") ||
	     *t == _T("menu") ) mt = Modifier::Type_Alt;
    else if (*t == _T("control") ||
	     *t == _T("ctrl") ) mt = Modifier::Type_Control;
    else if (*t == _T("windows") ||
	     *t == _T("win")  ) mt = Modifier::Type_Windows;
    else if (*t == _T("mod0") ) mt = Modifier::Type_Mod0;
    else if (*t == _T("mod1") ) mt = Modifier::Type_Mod1;
    else if (*t == _T("mod2") ) mt = Modifier::Type_Mod2;
    else if (*t == _T("mod3") ) mt = Modifier::Type_Mod3;
    else if (*t == _T("mod4") ) mt = Modifier::Type_Mod4;
    else if (*t == _T("mod5") ) mt = Modifier::Type_Mod5;
    else if (*t == _T("mod6") ) mt = Modifier::Type_Mod6;
    else if (*t == _T("mod7") ) mt = Modifier::Type_Mod7;
    else if (*t == _T("mod8") ) mt = Modifier::Type_Mod8;
    else if (*t == _T("mod9") ) mt = Modifier::Type_Mod9;
    else throw ErrorMessage() << _T("`") << *t
			      << _T("': invalid modifier name.");

    if (am == Keymap::AM_notModifier)
      break;
    
    m_currentKeymap->addModifier(mt, Keymap::AO_overwrite, am, NULL);
    if (isEOL())
      return;
    t = getToken();
  }
  
  // <ASSIGN_OP>
  t = getToken();
  Keymap::AssignOperator ao;
  if      (*t == _T("=") ) ao = Keymap::AO_new;
  else if (*t == _T("+=")) ao = Keymap::AO_add;
  else if (*t == _T("-=")) ao = Keymap::AO_sub;
  else  throw ErrorMessage() << _T("`") << *t << _T("': is unknown operator.");

  // <ASSIGN_MODE>? <KEY_NAME>
  while (!isEOL())
  {
    // <ASSIGN_MODE>? 
    t = getToken();
    Keymap::AssignMode am = Keymap::AM_normal;
    if      (*t == _T("!")  ) am = Keymap::AM_true, t = getToken();
    else if (*t == _T("!!") ) am = Keymap::AM_oneShot, t = getToken();
    else if (*t == _T("!!!")) am = Keymap::AM_oneShotRepeatable, t = getToken();
    
    // <KEY_NAME>
    Key *key = m_setting->m_keyboard.searchKey(t->getString());
    if (!key)
      throw ErrorMessage() << _T("`") << *t << _T("': invalid key name.");

    // we can ignore warning C4701
    m_currentKeymap->addModifier(mt, ao, am, key);
    if (ao == Keymap::AO_new)
      ao = Keymap::AO_add;
  }
}


// <KEYSEQ_DEFINITION>
void SettingLoader::load_KEYSEQ_DEFINITION()
{
  if (*getToken() != _T("$"))
    throw ErrorMessage() << _T("there must be `$' after `keyseq'");
  Token *name = getToken();
  if (*getToken() != _T("="))
    throw ErrorMessage() << _T("there must be `=' after keyseq naem");
  load_KEY_SEQUENCE(name->getString(), false, Modifier::Type_ASSIGN);
}


// <DEFINE>
void SettingLoader::load_DEFINE()
{
  m_setting->m_symbols.insert(getToken()->getString());
}


// <IF>
void SettingLoader::load_IF()
{
  if (!getToken()->isOpenParen())
    throw ErrorMessage() << _T("there must be `(' after `if'.");
  Token *t = getToken(); // <SYMBOL> or !
  bool not = false;
  if (*t == _T("!"))
  {
    not = true;
    t = getToken(); // <SYMBOL>
  }
  
  bool doesSymbolExist = (m_setting->m_symbols.find(t->getString())
			  != m_setting->m_symbols.end());
  bool doesRead = ((doesSymbolExist && !not) ||
		   (!doesSymbolExist && not));
  if (0 < m_canReadStack.size())
    doesRead = doesRead && m_canReadStack.back();
  
  if (!getToken()->isCloseParen())
    throw ErrorMessage() << _T("there must be `)'.");

  m_canReadStack.push_back(doesRead);
  if (!isEOL())
  {
    size_t len = m_canReadStack.size();
    load_LINE();
    if (len < m_canReadStack.size())
    {
      bool r = m_canReadStack.back();
      m_canReadStack.pop_back();
      m_canReadStack[len - 1] = r && doesRead;
    }
    else if (len == m_canReadStack.size())
      m_canReadStack.pop_back();
    else
      ; // `end' found
  }
}


// <ELSE> <ELSEIF>
void SettingLoader::load_ELSE(bool i_isElseIf, const tstringi &i_token)
{
  bool doesRead = !load_ENDIF(i_token);
  if (0 < m_canReadStack.size())
    doesRead = doesRead && m_canReadStack.back();
  m_canReadStack.push_back(doesRead);
  if (!isEOL())
  {
    size_t len = m_canReadStack.size();
    if (i_isElseIf)
      load_IF();
    else
      load_LINE();
    if (len < m_canReadStack.size())
    {
      bool r = m_canReadStack.back();
      m_canReadStack.pop_back();
      m_canReadStack[len - 1] = doesRead && r;
    }
    else if (len == m_canReadStack.size())
      m_canReadStack.pop_back();
    else
      ; // `end' found
  }
}


// <ENDIF>
bool SettingLoader::load_ENDIF(const tstringi &i_token)
{
  if (m_canReadStack.size() == 0)
    throw ErrorMessage() << _T("unbalanced `") << i_token << _T("'");
  bool r = m_canReadStack.back();
  m_canReadStack.pop_back();
  return r;
}


// <LINE>
void SettingLoader::load_LINE()
{
  Token *i_token = getToken();

  // <COND_SYMBOL>
  if      (*i_token == _T("if") ||
	   *i_token == _T("and")) load_IF();
  else if (*i_token == _T("else")) load_ELSE(false, i_token->getString());
  else if (*i_token == _T("elseif") ||
	   *i_token == _T("elsif")  ||
	   *i_token == _T("elif")   ||
	   *i_token == _T("or")) load_ELSE(true, i_token->getString());
  else if (*i_token == _T("endif")) load_ENDIF(_T("endif"));
  else if (0 < m_canReadStack.size() && !m_canReadStack.back())
  {
    while (!isEOL())
      getToken();
  }
  else if (*i_token == _T("define")) load_DEFINE();
  // <INCLUDE>
  else if (*i_token == _T("include")) load_INCLUDE();
  // <KEYBOARD_DEFINITION>
  else if (*i_token == _T("def")) load_KEYBOARD_DEFINITION();
  // <KEYMAP_DEFINITION>
  else if (*i_token == _T("keymap")  ||
	   *i_token == _T("keymap2") ||
	   *i_token == _T("window")) load_KEYMAP_DEFINITION(i_token);
  // <KEY_ASSIGN>
  else if (*i_token == _T("key")) load_KEY_ASSIGN();
  // <EVENT_ASSIGN>
  else if (*i_token == _T("event")) load_EVENT_ASSIGN();
  // <MODIFIER_ASSIGNMENT>
  else if (*i_token == _T("mod")) load_MODIFIER_ASSIGNMENT();
  // <KEYSEQ_DEFINITION>
  else if (*i_token == _T("keyseq")) load_KEYSEQ_DEFINITION();
  else
    throw ErrorMessage() << _T("syntax error `") << *i_token << _T("'.");
}

  
// prefix sort predicate used in load(const string &)
static bool prefixSortPred(const tstringi &i_a, const tstringi &i_b)
{
  return i_b.size() < i_a.size();
}


/*
  _UNICODE: read file (UTF-16 LE/BE, UTF-8, locale specific multibyte encoding)
  _MBCS: read file
*/
bool readFile(tstring *o_data, const tstringi &i_filename)
{
  // get size of file
#if 0
  // bcc's _wstat cannot obtain file size
  struct _stat sbuf;
  if (_tstat(i_filename.c_str(), &sbuf) < 0 || sbuf.st_size == 0)
    return false;
#else
  // so, we use _wstati64 for bcc
  struct stati64_t sbuf;
  if (_tstati64(i_filename.c_str(), &sbuf) < 0 || sbuf.st_size == 0)
    return false;
  // following check is needed to cast sbuf.st_size to size_t safely
  // this cast occurs because of above workaround for bcc
  if (sbuf.st_size > UINT_MAX)
    return false;
#endif
  
  // open
  FILE *fp = _tfopen(i_filename.c_str(), _T("rb"));
  if (!fp)
    return false;
  
  // read file
  Array<BYTE> buf(static_cast<size_t>(sbuf.st_size) + 1);
  if (fread(buf.get(), static_cast<size_t>(sbuf.st_size), 1, fp) != 1)
  {
    fclose(fp);
    return false;
  }
  buf.get()[sbuf.st_size] = 0;			// mbstowcs() requires null
						// terminated string

#ifdef _UNICODE
  //
  if (buf.get()[0] == 0xffU && buf.get()[1] == 0xfeU &&
      sbuf.st_size % 2 == 0)
    // UTF-16 Little Endien
  {
    size_t size = static_cast<size_t>(sbuf.st_size) / 2;
    o_data->resize(size);
    BYTE *p = buf.get();
    for (size_t i = 0; i < size; ++ i)
    {
      wchar_t c = static_cast<wchar_t>(*p ++);
      c |= static_cast<wchar_t>(*p ++) << 8;
      (*o_data)[i] = c;
    }
    fclose(fp);
    return true;
  }
  
  //
  if (buf.get()[0] == 0xfeU && buf.get()[1] == 0xffU &&
      sbuf.st_size % 2 == 0)
    // UTF-16 Big Endien
  {
    size_t size = static_cast<size_t>(sbuf.st_size) / 2;
    o_data->resize(size);
    BYTE *p = buf.get();
    for (size_t i = 0; i < size; ++ i)
    {
      wchar_t c = static_cast<wchar_t>(*p ++) << 8;
      c |= static_cast<wchar_t>(*p ++);
      (*o_data)[i] = c;
    }
    fclose(fp);
    return true;
  }

  // try multibyte charset 
  size_t wsize = mbstowcs(NULL, reinterpret_cast<char *>(buf.get()), 0);
  if (wsize != size_t(-1))
  {
    Array<wchar_t> wbuf(wsize);
    mbstowcs(wbuf.get(), reinterpret_cast<char *>(buf.get()), wsize);
    o_data->assign(wbuf.get(), wbuf.get() + wsize);
    fclose(fp);
    return true;
  }
  
  // try UTF-8
  {
    Array<wchar_t> wbuf(static_cast<size_t>(sbuf.st_size));
    BYTE *f = buf.get();
    BYTE *end = buf.get() + sbuf.st_size;
    wchar_t *d = wbuf.get();
    enum { STATE_1, STATE_2of2, STATE_2of3, STATE_3of3 } state = STATE_1;
    
    while (f != end)
    {
      switch (state)
      {
	case STATE_1:
	  if (!(*f & 0x80))			// 0xxxxxxx: 00-7F
	    *d++ = static_cast<wchar_t>(*f++);
	  else if ((*f & 0xe0) == 0xc0)		// 110xxxxx 10xxxxxx: 0080-07FF
	  {
	    *d = ((static_cast<wchar_t>(*f++) & 0x1f) << 6);
	    state = STATE_2of2;
	  }
	  else if ((*f & 0xf0) == 0xe0)		// 1110xxxx 10xxxxxx 10xxxxxx:
						// 0800 - FFFF
	  {
	    *d = ((static_cast<wchar_t>(*f++) & 0x0f) << 12);
	    state = STATE_2of3;
	  }
	  else
	    goto not_UTF_8;
	  break;
	  
	case STATE_2of2:
	case STATE_3of3:
	  if ((*f & 0xc0) != 0x80)
	    goto not_UTF_8;
	  *d++ |= (static_cast<wchar_t>(*f++) & 0x3f);
	  state = STATE_1;
	  break;

	case STATE_2of3:
	  if ((*f & 0xc0) != 0x80)
	    goto not_UTF_8;
	  *d |= ((static_cast<wchar_t>(*f++) & 0x3f) << 6);
	  state = STATE_3of3;
	  break;
      }
    }
    o_data->assign(wbuf.get(), d);
    fclose(fp);
    return true;
    
    not_UTF_8: ;
  }
#endif // _UNICODE

  // assume ascii
  o_data->resize(static_cast<size_t>(sbuf.st_size));
  for (off_t i = 0; i < sbuf.st_size; ++ i)
    (*o_data)[i] = buf.get()[i];
  fclose(fp);
  return true;
}


// load (called from load(Setting *, const tstringi &) only)
void SettingLoader::load(const tstringi &i_filename)
{
  m_currentFilename = i_filename;
  
  tstring data;
  if (!readFile(&data, m_currentFilename))
  {
    Acquire a(m_soLog);
    *m_log << m_currentFilename << _T(" : error: file not found") << std::endl;
#if 1
    *m_log << data << std::endl;
#endif
    m_isThereAnyError = true;
    return;
  }
  
  // prefix
  if (m_prefixesRefCcount == 0)
  {
    static const _TCHAR *prefixes[] =
    {
      _T("="), _T("=>"), _T("&&"), _T("||"), _T(":"), _T("$"), _T("&"),
      _T("-="), _T("+="), _T("!!!"), _T("!!"), _T("!"), 
      _T("E0-"), _T("E1-"),			// <SCAN_CODE_EXTENTION>
      _T("S-"), _T("A-"), _T("M-"), _T("C-"),	// <BASIC_MODIFIER>
      _T("W-"), _T("*"), _T("~"),
      _T("U-"), _T("D-"),			// <KEYSEQ_MODIFIER>
      _T("R-"), _T("IL-"), _T("IC-"), _T("I-"),	// <ASSIGN_MODIFIER>
      _T("NL-"), _T("CL-"), _T("SL-"), _T("KL-"),
      _T("MAX-"), _T("MIN-"), _T("MMAX-"), _T("MMIN-"),
      _T("T-"), _T("TS-"),
      _T("M0-"), _T("M1-"), _T("M2-"), _T("M3-"), _T("M4-"),
      _T("M5-"), _T("M6-"), _T("M7-"), _T("M8-"), _T("M9-"), 
      _T("L0-"), _T("L1-"), _T("L2-"), _T("L3-"), _T("L4-"),
      _T("L5-"), _T("L6-"), _T("L7-"), _T("L8-"), _T("L9-"), 
    };
    m_prefixes = new std::vector<tstringi>;
    for (size_t i = 0; i < NUMBER_OF(prefixes); ++ i)
      m_prefixes->push_back(prefixes[i]);
    std::sort(m_prefixes->begin(), m_prefixes->end(), prefixSortPred);
  }
  m_prefixesRefCcount ++;

  // create parser
  Parser parser(data.c_str(), data.size());
  parser.setPrefixes(m_prefixes);
    
  while (true)
  {
    try
    {
      if (!parser.getLine(&m_tokens))
	break;
      m_ti = m_tokens.begin();
    }
    catch (ErrorMessage &e)
    {
      if (m_log && m_soLog)
      {
	Acquire a(m_soLog);
	*m_log << m_currentFilename << _T("(") << parser.getLineNumber()
	       << _T(") : error: ") << e << std::endl;
      }
      m_isThereAnyError = true;
      continue;
    }
      
    try
    {
      load_LINE();
      if (!isEOL())
	throw WarningMessage() << _T("back garbage is ignored.");
    }
    catch (WarningMessage &w)
    {
      if (m_log && m_soLog)
      {
	Acquire a(m_soLog);
	*m_log << i_filename << _T("(") << parser.getLineNumber()
	       << _T(") : warning: ") << w << std::endl;
      }
    }
    catch (ErrorMessage &e)
    {
      if (m_log && m_soLog)
      {
	Acquire a(m_soLog);
	*m_log << i_filename << _T("(") << parser.getLineNumber()
	       << _T(") : error: ") << e << std::endl;
      }
      m_isThereAnyError = true;
    }
  }
    
  // m_prefixes
  -- m_prefixesRefCcount;
  if (m_prefixesRefCcount == 0)
    delete m_prefixes;

  if (0 < m_canReadStack.size())
  {
    Acquire a(m_soLog);
    *m_log << m_currentFilename << _T("(") << parser.getLineNumber()
	   << _T(") : error: unbalanced `if'.  ")
	   << _T("you forget `endif', didn'i_token you?")
	   << std::endl;
    m_isThereAnyError = true;
  }
}


// is the filename readable ?
bool SettingLoader::isReadable(const tstringi &i_filename,
			       int i_debugLevel) const 
{
  if (i_filename.empty())
    return false;
#ifdef UNICODE
  tifstream ist(to_string(i_filename).c_str());
#else
  tifstream ist(i_filename.c_str());
#endif
  if (ist.good())
  {
    if (m_log && m_soLog)
    {
		Acquire a(m_soLog, 0);
      *m_log << _T("  loading: ") << i_filename << std::endl;
    }
    return true;
  }
  else
  {
    if (m_log && m_soLog)
    {
      Acquire a(m_soLog, i_debugLevel);
      *m_log << _T("not found: ") << i_filename << std::endl;
    }
    return false;
  }
}

#if 0
// get filename from registry
bool SettingLoader::getFilenameFromRegistry(tstringi *o_path) const
{
  // get from registry
  Registry reg(MAYU_REGISTRY_ROOT);
  int index;
  reg.read(_T(".mayuIndex"), &index, 0);
  char buf[100];
  snprintf(buf, NUMBER_OF(buf), _T(".mayu%d"), index);
  if (!reg.read(buf, o_path))
    return false;

  // parse registry entry
  Regexp getFilename(_T("^[^;]*;([^;]*);(.*)$"));
  if (!getFilename.doesMatch(*o_path))
    return false;
  
  tstringi path = getFilename[1];
  tstringi options = getFilename[2];
  
  if (!(0 < path.size() && isReadable(path)))
    return false;
  *o_path = path;
  
  // set symbols
  Regexp symbol(_T("-D([^;]*)"));
  while (symbol.doesMatch(options))
  {
    m_setting->symbols.insert(symbol[1]);
    options = options.substr(symbol.subBegin(1));
  }
  
  return true;
}
#endif


// get filename
bool SettingLoader::getFilename(const tstringi &i_name, tstringi *o_path,
				int i_debugLevel) const
{
  // the default filename is ".mayu"
  const tstringi &name = i_name.empty() ? tstringi(_T(".mayu")) : i_name;
  
  bool isFirstTime = true;

  while (true)
  {
    // find file from registry
    if (i_name.empty())				// called not from 'include'
    {
      Setting::Symbols symbols;
      if (getFilenameFromRegistry(NULL, o_path, &symbols))
      {
	if (o_path->empty())
	  // find file from home directory
	{
	  HomeDirectories pathes;
	  getHomeDirectories(&pathes);
	  for (HomeDirectories::iterator
		 i = pathes.begin(); i != pathes.end(); ++ i)
	  {
	    *o_path = *i + _T("\\") + name;
	    if (isReadable(*o_path, i_debugLevel))
	      goto add_symbols;
	  }
	  return false;
	}
	else
	{
	  if (!isReadable(*o_path, i_debugLevel))
	    return false;
	}
	add_symbols:
	for (Setting::Symbols::iterator
	       i = symbols.begin(); i != symbols.end(); ++ i)
	  m_setting->m_symbols.insert(*i);
	return true;
      }
    }
    
    if (!isFirstTime)
      return false;
    
    // find file from home directory
    HomeDirectories pathes;
    getHomeDirectories(&pathes);
    for (HomeDirectories::iterator i = pathes.begin(); i != pathes.end(); ++ i)
    {
      *o_path = *i + _T("\\") + name;
      if (isReadable(*o_path, i_debugLevel))
	return true;
    }
    
    if (!i_name.empty())
      return false;				// called by 'include'
    
    if (!DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_setting),
		   NULL, dlgSetting_dlgProc))
      return false;
  }
}


// constructor
SettingLoader::SettingLoader(SyncObject *i_soLog, tostream *i_log)
  : m_setting(NULL),
    m_isThereAnyError(false),
    m_soLog(i_soLog),
    m_log(i_log),
    m_currentKeymap(NULL)
{
  m_defaultKeySeqModifier =
    m_defaultAssignModifier.release(Modifier::Type_ImeComp);
}


/* load m_setting
   If called by "include", 'filename' describes filename.
   Otherwise the 'filename' is empty.
 */
bool SettingLoader::load(Setting *i_setting, const tstringi &i_filename)
{
  m_setting = i_setting;
  m_isThereAnyError = false;
    
  tstringi path;
  if (!getFilename(i_filename, &path))
  {
    if (i_filename.empty())
    {
      Acquire a(m_soLog);
      getFilename(i_filename, &path, 0);	// show filenames
      return false;
    }
    else
      throw ErrorMessage() << _T("`") << i_filename
			   << _T("': no such file or other error.");
  }

  // create global keymap's default keySeq
  ActionFunction af(createFunctionData(_T("OtherWindowClass")));
  KeySeq *globalDefault = m_setting->m_keySeqs.add(KeySeq(_T("")).add(af));
  
  // add default keymap
  m_currentKeymap = m_setting->m_keymaps.add(
    Keymap(Keymap::Type_windowOr, _T("Global"), _T(""), _T(""),
	   globalDefault, NULL));

  /*
  // add keyboard layout name
  if (filename.empty())
  {
    char keyboardLayoutName[KL_NAMELENGTH];
    if (GetKeyboardLayoutName(keyboardLayoutName))
    {
      tstringi kl = tstringi(_T("KeyboardLayout/")) + keyboardLayoutName;
      m_setting->symbols.insert(kl);
      Acquire a(m_soLog);
      *m_log << _T("KeyboardLayout: ") << kl << std::endl;
    }
  }
  */
  
  // load
  load(path);

  // finalize
  if (i_filename.empty())
    m_setting->m_keymaps.adjustModifier(m_setting->m_keyboard);
  
  return !m_isThereAnyError;
}


std::vector<tstringi> *SettingLoader::m_prefixes; // m_prefixes terminal symbol
size_t SettingLoader::m_prefixesRefCcount;	/* reference count of
						   m_prefixes */
