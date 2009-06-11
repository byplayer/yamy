//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// engine.cpp


#include "misc.h"

#include "engine.h"
#include "errormessage.h"
#include "hook.h"
#include "mayurc.h"
#include "windowstool.h"

#include <iomanip>

#include <process.h>


// check focus window
void Engine::checkFocusWindow()
{
  int count = 0;
  
  restart:
  count ++;
  
  HWND hwndFore = GetForegroundWindow();
  DWORD threadId = GetWindowThreadProcessId(hwndFore, NULL);

  if (hwndFore)
  {
    {
      Acquire a(&m_cs);
      if (m_currentFocusOfThread &&
	  m_currentFocusOfThread->m_threadId == threadId &&
	  m_currentFocusOfThread->m_hwndFocus == m_hwndFocus)
	return;

      m_emacsEditKillLine.reset();
      
      // erase dead thread
      if (!m_detachedThreadIds.empty())
      {
	for (DetachedThreadIds::iterator i = m_detachedThreadIds.begin();
	     i != m_detachedThreadIds.end(); i ++)
	{
	  FocusOfThreads::iterator j = m_focusOfThreads.find((*i));
	  if (j != m_focusOfThreads.end())
	  {
	    FocusOfThread *fot = &((*j).second);
	    Acquire a(&m_log, 1);
	    m_log << _T("RemoveThread") << std::endl;
	    m_log << _T("\tHWND:\t") << std::hex << (int)fot->m_hwndFocus
		  << std::dec << std::endl;
	    m_log << _T("\tTHREADID:") << fot->m_threadId << std::endl;
	    m_log << _T("\tCLASS:\t") << fot->m_className << std::endl;
	    m_log << _T("\tTITLE:\t") << fot->m_titleName << std::endl;
	    m_log << std::endl;
	    m_focusOfThreads.erase(j);
	  }
	}
	m_detachedThreadIds.erase
	  (m_detachedThreadIds.begin(), m_detachedThreadIds.end());
      }
      
      FocusOfThreads::iterator i = m_focusOfThreads.find(threadId);
      if (i != m_focusOfThreads.end())
      {
	m_currentFocusOfThread = &((*i).second);
	if (!m_currentFocusOfThread->m_isConsole || 2 <= count)
	{
	  if (m_currentFocusOfThread->m_keymaps.empty())
	    setCurrentKeymap(NULL);
	  else
	    setCurrentKeymap(*m_currentFocusOfThread->m_keymaps.begin());
	  m_hwndFocus = m_currentFocusOfThread->m_hwndFocus;
	  checkShow(m_hwndFocus);
	
	  Acquire a(&m_log, 1);
	  m_log << _T("FocusChanged") << std::endl;
	  m_log << _T("\tHWND:\t")
		<< std::hex << (int)m_currentFocusOfThread->m_hwndFocus
		<< std::dec << std::endl;
	  m_log << _T("\tTHREADID:")
		<< m_currentFocusOfThread->m_threadId << std::endl;
	  m_log << _T("\tCLASS:\t")
		<< m_currentFocusOfThread->m_className << std::endl;
	  m_log << _T("\tTITLE:\t")
		<< m_currentFocusOfThread->m_titleName << std::endl;
	  m_log << std::endl;
	  return;
	}
      }
    }
    
    _TCHAR className[GANA_MAX_ATOM_LENGTH];
    if (GetClassName(hwndFore, className, NUMBER_OF(className)))
    {
      if (_tcsicmp(className, _T("ConsoleWindowClass")) == 0)
      {
	_TCHAR titleName[1024];
	if (GetWindowText(hwndFore, titleName, NUMBER_OF(titleName)) == 0)
	  titleName[0] = _T('\0');
	setFocus(hwndFore, threadId, className, titleName, true);
	Acquire a(&m_log, 1);
	m_log << _T("HWND:\t") << std::hex << reinterpret_cast<int>(hwndFore)
	      << std::dec << std::endl;
	m_log << _T("THREADID:") << threadId << std::endl;
	m_log << _T("CLASS:\t") << className << std::endl;
	m_log << _T("TITLE:\t") << titleName << std::endl << std::endl;
	goto restart;
      }
    }
  }
  
  Acquire a(&m_cs);
  if (m_globalFocus.m_keymaps.empty())
  {
    Acquire a(&m_log, 1);
    m_log << _T("NO GLOBAL FOCUS") << std::endl;
    m_currentFocusOfThread = NULL;
    setCurrentKeymap(NULL);
  }
  else
  {
    if (m_currentFocusOfThread != &m_globalFocus)
    {
      Acquire a(&m_log, 1);
      m_log << _T("GLOBAL FOCUS") << std::endl;
      m_currentFocusOfThread = &m_globalFocus;
      setCurrentKeymap(m_globalFocus.m_keymaps.front());
    }
  }
  m_hwndFocus = NULL;
}



// is modifier pressed ?
bool Engine::isPressed(Modifier::Type i_mt)
{
  const Keymap::ModAssignments &ma = m_currentKeymap->getModAssignments(i_mt);
  for (Keymap::ModAssignments::const_iterator i = ma.begin();
       i != ma.end(); ++ i)
    if ((*i).m_key->m_isPressed)
      return true;
  return false;
}


// fix modifier key (if fixed, return true)
bool Engine::fixModifierKey(ModifiedKey *io_mkey, Keymap::AssignMode *o_am)
{
  // for all modifier ...
  for (int i = Modifier::Type_begin; i != Modifier::Type_end; ++ i)
  {
    // get modifier assignments (list of modifier keys)
    const Keymap::ModAssignments &ma =
      m_currentKeymap->getModAssignments(static_cast<Modifier::Type>(i));
    
    for (Keymap::ModAssignments::const_iterator
	   j = ma.begin(); j != ma.end(); ++ j)
      if (io_mkey->m_key == (*j).m_key) // is io_mkey a modifier ?
      {
	{
	  Acquire a(&m_log, 1);
	  m_log << _T("* Modifier Key") << std::endl;
	}
	// set dontcare for this modifier
	io_mkey->m_modifier.dontcare(static_cast<Modifier::Type>(i));
	*o_am = (*j).m_assignMode;
	return true;
      }
  }
  *o_am = Keymap::AM_notModifier;
  return false;
}


// output to m_log
void Engine::outputToLog(const Key *i_key, const ModifiedKey &i_mkey,
			 int i_debugLevel)
{
  size_t i;
  Acquire a(&m_log, i_debugLevel);

  // output scan codes
  for (i = 0; i < i_key->getScanCodesSize(); ++ i)
  {
    if (i_key->getScanCodes()[i].m_flags & ScanCode::E0) m_log << _T("E0-");
    if (i_key->getScanCodes()[i].m_flags & ScanCode::E1) m_log << _T("E1-");
    if (!(i_key->getScanCodes()[i].m_flags & ScanCode::E0E1))
      m_log << _T("   ");
    m_log << _T("0x") << std::hex << std::setw(2) << std::setfill(_T('0'))
	  << static_cast<int>(i_key->getScanCodes()[i].m_scan)
	  << std::dec << _T(" ");
  }
  
  if (!i_mkey.m_key) // key corresponds to no phisical key
  {
    m_log << std::endl;
    return;
  }
  
  m_log << _T("  ") << i_mkey << std::endl;
}


// describe bindings
void Engine::describeBindings()
{
  Acquire a(&m_log, 0);

  Keymap::DescribeParam dp;
  for (KeymapPtrList::iterator i = m_currentFocusOfThread->m_keymaps.begin();
       i != m_currentFocusOfThread->m_keymaps.end(); ++ i)
    (*i)->describe(m_log, &dp);
  m_log << std::endl;
}


// update m_lastPressedKey
void Engine::updateLastPressedKey(Key *i_key)
{
  m_lastPressedKey[1] = m_lastPressedKey[0];
  m_lastPressedKey[0] = i_key;
}

// set current keymap
void Engine::setCurrentKeymap(const Keymap *i_keymap, bool i_doesAddToHistory)
{
  if (i_doesAddToHistory)
  {
    m_keymapPrefixHistory.push_back(const_cast<Keymap *>(m_currentKeymap));
    if (MAX_KEYMAP_PREFIX_HISTORY < m_keymapPrefixHistory.size())
      m_keymapPrefixHistory.pop_front();
  }
  else
    m_keymapPrefixHistory.clear();
  m_currentKeymap = i_keymap;
}


// get current modifiers
Modifier Engine::getCurrentModifiers(Key *i_key, bool i_isPressed)
{
  Modifier cmods;
  cmods.add(m_currentLock);

  cmods.press(Modifier::Type_Shift  , isPressed(Modifier::Type_Shift  ));
  cmods.press(Modifier::Type_Alt    , isPressed(Modifier::Type_Alt    ));
  cmods.press(Modifier::Type_Control, isPressed(Modifier::Type_Control));
  cmods.press(Modifier::Type_Windows, isPressed(Modifier::Type_Windows));
  cmods.press(Modifier::Type_Up     , !i_isPressed);
  cmods.press(Modifier::Type_Down   , i_isPressed);

  cmods.press(Modifier::Type_Repeat , false);
  if (m_lastPressedKey[0] == i_key)
  {
    if (i_isPressed)
      cmods.press(Modifier::Type_Repeat, true);
    else
      if (m_lastPressedKey[1] == i_key)
	cmods.press(Modifier::Type_Repeat, true);
  }

  for (int i = Modifier::Type_Mod0; i <= Modifier::Type_Mod9; ++ i)
    cmods.press(static_cast<Modifier::Type>(i),
		isPressed(static_cast<Modifier::Type>(i)));
  
  return cmods;
}


// generate keyboard event for a key
void Engine::generateKeyEvent(Key *i_key, bool i_doPress, bool i_isByAssign)
{
  // check if key is event
  bool isEvent = false;
  for (Key **e = Event::events; *e; ++ e)
    if (*e == i_key)
    {
      isEvent = true;
      break;
    }

  bool isAlreadyReleased = false;
    
  if (!isEvent)
  {
    if (i_doPress && !i_key->m_isPressedOnWin32)
      ++ m_currentKeyPressCountOnWin32;
    else if (!i_doPress)
    {
      if (i_key->m_isPressedOnWin32)
	-- m_currentKeyPressCountOnWin32;
      else
	isAlreadyReleased = true;
    }
    i_key->m_isPressedOnWin32 = i_doPress;
    
    if (i_isByAssign)
      i_key->m_isPressedByAssign = i_doPress;

    Key *sync = m_setting->m_keyboard.getSyncKey();
    
    if (!isAlreadyReleased || i_key == sync)
    {
      KEYBOARD_INPUT_DATA kid = { 0, 0, 0, 0, 0 };
      const ScanCode *sc = i_key->getScanCodes();
      for (size_t i = 0; i < i_key->getScanCodesSize(); ++ i)
      {
	kid.MakeCode = sc[i].m_scan;
	kid.Flags = sc[i].m_flags;
	if (!i_doPress)
	  kid.Flags |= KEYBOARD_INPUT_DATA::BREAK;
	DWORD len;
#if defined(_WINNT)
	WriteFile(m_device, &kid, sizeof(kid), &len, &m_ol);
	CHECK_TRUE( GetOverlappedResult(m_device, &m_ol, &len, TRUE) );
#elif defined(_WIN95)
	DeviceIoControl(m_device, 2, &kid, sizeof(kid), NULL, 0, &len, NULL);
#else
#  error
#endif
      }
      
      m_lastGeneratedKey = i_doPress ? i_key : NULL;
    }
  }
  
  {
    Acquire a(&m_log, 1);
    m_log << _T("\t\t    =>\t");
    if (isAlreadyReleased)
      m_log << _T("(already released) ");
  }
  ModifiedKey mkey(i_key);
  mkey.m_modifier.on(Modifier::Type_Up, !i_doPress);
  mkey.m_modifier.on(Modifier::Type_Down, i_doPress);
  outputToLog(i_key, mkey, 1);
}


// genete event
void Engine::generateEvents(Current i_c, const Keymap *i_keymap, Key *i_event)
{
  // generate
  i_c.m_keymap = i_keymap;
  i_c.m_mkey.m_key = i_event;
  if (const Keymap::KeyAssignment *keyAssign =
      i_c.m_keymap->searchAssignment(i_c.m_mkey))
  {
    {
      Acquire a(&m_log, 1);
      m_log << std::endl << _T("           ")
	    << i_event->getName() << std::endl;
    }
    generateKeySeqEvents(i_c, keyAssign->m_keySeq, Part_all);
  }
}


// genete modifier events
void Engine::generateModifierEvents(const Modifier &i_mod)
{
  {
    Acquire a(&m_log, 1);
    m_log << _T("* Gen Modifiers\t{") << std::endl;
  }

  for (int i = Modifier::Type_begin; i < Modifier::Type_BASIC; ++ i)
  {
    Keyboard::Mods &mods =
      m_setting->m_keyboard.getModifiers(static_cast<Modifier::Type>(i));

    if (i_mod.isDontcare(static_cast<Modifier::Type>(i)))
      // no need to process
      ;
    else if (i_mod.isPressed(static_cast<Modifier::Type>(i)))
      // we have to press this modifier
    {
      bool noneIsPressed = true;
      bool noneIsPressedByAssign = true;
      for (Keyboard::Mods::iterator i = mods.begin(); i != mods.end(); ++ i)
      {
	if ((*i)->m_isPressedOnWin32)
	  noneIsPressed = false;
	if ((*i)->m_isPressedByAssign)
	  noneIsPressedByAssign = false;
      }
      if (noneIsPressed)
      {
	if (noneIsPressedByAssign)
	  generateKeyEvent(mods.front(), true, false);
	else
	  for (Keyboard::Mods::iterator
		 i = mods.begin(); i != mods.end(); ++ i)
	    if ((*i)->m_isPressedByAssign)
	      generateKeyEvent((*i), true, false);
      }
    }

    else
      // we have to release this modifier
    {
      // avoid such sequences as  "Alt U-ALt" or "Windows U-Windows"
      if (i == Modifier::Type_Alt || i == Modifier::Type_Windows)
      {
	for (Keyboard::Mods::iterator j = mods.begin(); j != mods.end(); ++ j)
	  if ((*j) == m_lastGeneratedKey)
	  {
	    Keyboard::Mods *mods =
	      &m_setting->m_keyboard.getModifiers(Modifier::Type_Shift);
	    if (mods->size() == 0)
	      mods = &m_setting->m_keyboard.getModifiers(
		Modifier::Type_Control);
	    if (0 < mods->size())
	    {
	      generateKeyEvent(mods->front(), true, false);
	      generateKeyEvent(mods->front(), false, false);
	    }
	    break;
	  }
      }
      
      for (Keyboard::Mods::iterator j = mods.begin(); j != mods.end(); ++ j)
      {
	if ((*j)->m_isPressedOnWin32)
	  generateKeyEvent((*j), false, false);
      }
    }
  }
  
  {
    Acquire a(&m_log, 1);
    m_log << _T("\t\t}") << std::endl;
  }
}


// generate keyboard events for action
void Engine::generateActionEvents(const Current &i_c, const Action *i_a,
				  bool i_doPress)
{
  switch (i_a->getType())
  {
    // key
    case Action::Type_key:
    {
      const ModifiedKey &mkey
	= reinterpret_cast<ActionKey *>(
	  const_cast<Action *>(i_a))->m_modifiedKey;

      // release
      if (!i_doPress &&
	  (mkey.m_modifier.isOn(Modifier::Type_Up) ||
	   mkey.m_modifier.isDontcare(Modifier::Type_Up)))
	generateKeyEvent(mkey.m_key, false, true);

      // press
      else if (i_doPress &&
	       (mkey.m_modifier.isOn(Modifier::Type_Down) ||
		mkey.m_modifier.isDontcare(Modifier::Type_Down)))
      {
	Modifier modifier = mkey.m_modifier;
	modifier.add(i_c.m_mkey.m_modifier);
	generateModifierEvents(modifier);
	generateKeyEvent(mkey.m_key, true, true);
      }
      break;
    }

    // keyseq
    case Action::Type_keySeq:
    {
      const ActionKeySeq *aks = reinterpret_cast<const ActionKeySeq *>(i_a);
      generateKeySeqEvents(i_c, aks->m_keySeq,
			   i_doPress ? Part_down : Part_up);
      break;
    }

    // function
    case Action::Type_function:
    {
      const ActionFunction *af = reinterpret_cast<const ActionFunction *>(i_a);
      bool is_up = (!i_doPress &&
		    (af->m_modifier.isOn(Modifier::Type_Up) ||
		     af->m_modifier.isDontcare(Modifier::Type_Up)));
      bool is_down = (i_doPress &&
		      (af->m_modifier.isOn(Modifier::Type_Down) ||
		       af->m_modifier.isDontcare(Modifier::Type_Down)));

      if (!is_down && !is_up)
	break;
      
      {
	Acquire a(&m_log, 1);
	m_log << _T("\t\t     >\t") << af->m_functionData;
      }
      
      FunctionParam param;
      param.m_isPressed = i_doPress;
      param.m_hwnd = m_currentFocusOfThread->m_hwndFocus;
      param.m_c = i_c;
      param.m_doesNeedEndl = true;
      param.m_af = af;
      
      param.m_c.m_mkey.m_modifier.on(Modifier::Type_Up, !i_doPress);
      param.m_c.m_mkey.m_modifier.on(Modifier::Type_Down, i_doPress);

      af->m_functionData->exec(this, &param);
      
      if (param.m_doesNeedEndl)
      {
	Acquire a(&m_log, 1);
	m_log << std::endl;
      }
      break;
    }
  }
}


// generate keyboard events for keySeq
void Engine::generateKeySeqEvents(const Current &i_c, const KeySeq *i_keySeq,
				  Part i_part)
{
  const KeySeq::Actions &actions = i_keySeq->getActions();
  if (actions.empty())
    return;
  if (i_part == Part_up)
    generateActionEvents(i_c, actions[actions.size() - 1], false);
  else
  {
    size_t i;
    for (i = 0 ; i < actions.size() - 1; ++ i)
    {
      generateActionEvents(i_c, actions[i], true);
      generateActionEvents(i_c, actions[i], false);
    }
    generateActionEvents(i_c, actions[i], true);
    if (i_part == Part_all)
      generateActionEvents(i_c, actions[i], false);
  }
}


// generate keyboard events for current key
void Engine::generateKeyboardEvents(const Current &i_c)
{
  if (++ m_generateKeyboardEventsRecursionGuard ==
      MAX_GENERATE_KEYBOARD_EVENTS_RECURSION_COUNT)
  {
    Acquire a(&m_log);
    m_log << _T("error: too deep keymap recursion.  there may be a loop.")
	  << std::endl;
    return;
  }

  const Keymap::KeyAssignment *keyAssign
    = i_c.m_keymap->searchAssignment(i_c.m_mkey);
  if (!keyAssign)
  {
    const KeySeq *keySeq = i_c.m_keymap->getDefaultKeySeq();
    ASSERT( keySeq );
    generateKeySeqEvents(i_c, keySeq, i_c.isPressed() ? Part_down : Part_up);
  }
  else
  {
    if (keyAssign->m_modifiedKey.m_modifier.isOn(Modifier::Type_Up) ||
	keyAssign->m_modifiedKey.m_modifier.isOn(Modifier::Type_Down))
      generateKeySeqEvents(i_c, keyAssign->m_keySeq, Part_all);
    else
      generateKeySeqEvents(i_c, keyAssign->m_keySeq,
			   i_c.isPressed() ? Part_down : Part_up);
  }
  m_generateKeyboardEventsRecursionGuard --;
}


// generate keyboard events for current key
void Engine::beginGeneratingKeyboardEvents(
  const Current &i_c, bool i_isModifier)
{
  //             (1)             (2)             (3)  (4)   (1)
  // up/down:    D-              U-              D-   U-    D-
  // keymap:     m_currentKeymap m_currentKeymap X    X     m_currentKeymap
  // memo:       &Prefix(X)      ...             ...  ...   ...
  // m_isPrefix: false           true            true false false

  Current cnew(i_c);

  bool isPhysicallyPressed
    = cnew.m_mkey.m_modifier.isPressed(Modifier::Type_Down);
  
  // substitute
  ModifiedKey mkey = m_setting->m_keyboard.searchSubstitute(cnew.m_mkey);
  if (mkey.m_key)
  {
    cnew.m_mkey = mkey;
    if (isPhysicallyPressed)
    {
      cnew.m_mkey.m_modifier.off(Modifier::Type_Up);
      cnew.m_mkey.m_modifier.on(Modifier::Type_Down);
    }
    else
    {
      cnew.m_mkey.m_modifier.on(Modifier::Type_Up);
      cnew.m_mkey.m_modifier.off(Modifier::Type_Down);
    }
    for (int i = Modifier::Type_begin; i != Modifier::Type_end; ++ i)
    {
      Modifier::Type type = static_cast<Modifier::Type>(i);
      if (cnew.m_mkey.m_modifier.isDontcare(type) &&
	  !i_c.m_mkey.m_modifier.isDontcare(type))
	cnew.m_mkey.m_modifier.press(
	  type, i_c.m_mkey.m_modifier.isPressed(type));
    }
    
    {
      Acquire a(&m_log, 1);
      m_log << _T("* substitute") << std::endl;
    }
    outputToLog(mkey.m_key, cnew.m_mkey, 1);
  }
  
  // for prefix key
  const Keymap *tmpKeymap = m_currentKeymap;
  if (i_isModifier || !m_isPrefix) ; 
  else if (isPhysicallyPressed)			// when (3)
    m_isPrefix = false;
  else if (!isPhysicallyPressed)		// when (2)
    m_currentKeymap = m_currentFocusOfThread->m_keymaps.front();
  
  // for m_emacsEditKillLine function
  m_emacsEditKillLine.m_doForceReset = !i_isModifier;

  // generate key event !
  m_generateKeyboardEventsRecursionGuard = 0;
  if (isPhysicallyPressed)
    generateEvents(cnew, cnew.m_keymap, &Event::before_key_down);
  generateKeyboardEvents(cnew);
  if (!isPhysicallyPressed)
    generateEvents(cnew, cnew.m_keymap, &Event::after_key_up);
      
  // for m_emacsEditKillLine function
  if (m_emacsEditKillLine.m_doForceReset)
    m_emacsEditKillLine.reset();

  // for prefix key
  if (i_isModifier)
    ;
  else if (!m_isPrefix)				// when (1), (4)
    m_currentKeymap = m_currentFocusOfThread->m_keymaps.front();
  else if (!isPhysicallyPressed)		// when (2)
    m_currentKeymap = tmpKeymap;
}


// pop all pressed key on win32
void Engine::keyboardResetOnWin32()
{
  for (Keyboard::KeyIterator
	 i = m_setting->m_keyboard.getKeyIterator();  *i; ++ i)
  {
    if ((*i)->m_isPressedOnWin32)
      generateKeyEvent((*i), false, true);
  }
}


// keyboard handler thread
unsigned int WINAPI Engine::keyboardHandler(void *i_this)
{
  reinterpret_cast<Engine *>(i_this)->keyboardHandler();
  _endthreadex(0);
  return 0;
}
void Engine::keyboardHandler()
{
  // initialize ok
  CHECK_TRUE( SetEvent(m_threadEvent) );
    
  // loop
  Key key;
  while (!m_doForceTerminate)
  {
    KEYBOARD_INPUT_DATA kid;
    
    DWORD len;
#if defined(_WINNT)
    {
      Acquire a(&m_log, 1);
      m_log << _T("begin ReadFile();") << std::endl;
    }
    if (!ReadFile(m_device, &kid, sizeof(kid), &len, &m_ol))
    {
      if (GetLastError() != ERROR_IO_PENDING)
	continue;
      
      HANDLE handles[] = { m_readEvent, m_interruptThreadEvent };
    rewait:
      switch (MsgWaitForMultipleObjects(NUMBER_OF(handles), &handles[0],
				     FALSE, INFINITE, QS_POSTMESSAGE))
      {
	case WAIT_OBJECT_0:			// m_readEvent
	  if (!GetOverlappedResult(m_device, &m_ol, &len, FALSE))
	    continue;
	  break;
	  
	case WAIT_OBJECT_0 + 1:			// m_interruptThreadEvent
	  CancelIo(m_device);
	  switch (m_interruptThreadReason) {
	    default: {
	      ASSERT( false );
	      Acquire a(&m_log, 0);
	      m_log << _T("internal error: m_interruptThreadReason == ")
		    << m_interruptThreadReason << std::endl;
	      break;
	    }
	      
	    case InterruptThreadReason_Terminate:
	      goto break_while;
	      
	    case InterruptThreadReason_Pause: {
	      CHECK_TRUE( SetEvent(m_threadEvent) );
	      while (WaitForMultipleObjects(1, &m_interruptThreadEvent,
					    FALSE, INFINITE) != WAIT_OBJECT_0)
		;
	      switch (m_interruptThreadReason) {
		case InterruptThreadReason_Terminate:
		  goto break_while;

		case InterruptThreadReason_Resume:
		  break;

		default:
		  ASSERT( false );
		  break;
	      }
	      CHECK_TRUE( SetEvent(m_threadEvent) );
	      break;
	    }
	  }
	  break;
	  
        case WAIT_OBJECT_0 + NUMBER_OF(handles):
	{
	  MSG message;

	  while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE))
	  {
	    switch (message.message)
	    {
	      case WM_APP + 201:
	      {
		if (message.wParam)
		{
		  m_currentLock.on(Modifier::Type_Touchpad);
		  m_currentLock.on(Modifier::Type_TouchpadSticky);
		}
		else
		  m_currentLock.off(Modifier::Type_Touchpad);
		Acquire a(&m_log, 1);
		m_log << _T("touchpad: ") << message.wParam
		      << _T(".") << (message.lParam & 0xffff)
		      << _T(".") << (message.lParam >> 16 & 0xffff)
		      << std::endl;
		break;
	      }
	      default:
		break;
	    }
	  }
	  goto rewait;
	}

	default:
	  ASSERT( false );
	  continue;
      }
    }
    {
      Acquire a(&m_log, 1);
      m_log << _T("end ReadFile();") << std::endl;
    }
#elif defined(_WIN95)
    if (!DeviceIoControl(m_device, 1, NULL, 0, &kid, sizeof(kid), &len, NULL))
    {
      continue; // TODO
    }
#else
#  error
#endif

    checkFocusWindow();

    if (!m_setting ||	// m_setting has not been loaded
	!m_isEnabled)	// disabled
    {
      if (m_isLogMode)
      {
	Key key;
	key.addScanCode(ScanCode(kid.MakeCode, kid.Flags));
	outputToLog(&key, ModifiedKey(), 0);
      }
      else
      {
#if defined(_WINNT)
	WriteFile(m_device, &kid, sizeof(kid), &len, &m_ol);
	GetOverlappedResult(m_device, &m_ol, &len, TRUE);
#elif defined(_WIN95)
	DeviceIoControl(m_device, 2, &kid, sizeof(kid), NULL, 0, &len, NULL);
#else
#  error
#endif
      }
      updateLastPressedKey(NULL);
      continue;
    }
    
    Acquire a(&m_cs);

    if (!m_currentFocusOfThread ||
	!m_currentKeymap)
    {
#if defined(_WINNT)
      WriteFile(m_device, &kid, sizeof(kid), &len, &m_ol);
      GetOverlappedResult(m_device, &m_ol, &len, TRUE);
#elif defined(_WIN95)
      DeviceIoControl(m_device, 2, &kid, sizeof(kid), NULL, 0, &len, NULL);
#else
#  error
#endif
      Acquire a(&m_log, 0);
      if (!m_currentFocusOfThread)
	m_log << _T("internal error: m_currentFocusOfThread == NULL")
	      << std::endl;
      if (!m_currentKeymap)
	m_log << _T("internal error: m_currentKeymap == NULL")
	      << std::endl;
      updateLastPressedKey(NULL);
      continue;
    }
    
    Current c;
    c.m_keymap = m_currentKeymap;
    c.m_i = m_currentFocusOfThread->m_keymaps.begin();
    
    // search key
    key.addScanCode(ScanCode(kid.MakeCode, kid.Flags));
    c.m_mkey = m_setting->m_keyboard.searchKey(key);
    if (!c.m_mkey.m_key)
    {
      c.m_mkey.m_key = m_setting->m_keyboard.searchPrefixKey(key);
      if (c.m_mkey.m_key)
	continue;
    }

    // press the key and update counter
    bool isPhysicallyPressed
      = !(key.getScanCodes()[0].m_flags & ScanCode::BREAK);
    if (c.m_mkey.m_key)
    {
      if (!c.m_mkey.m_key->m_isPressed && isPhysicallyPressed)
	++ m_currentKeyPressCount;
      else if (c.m_mkey.m_key->m_isPressed && !isPhysicallyPressed)
	-- m_currentKeyPressCount;
      c.m_mkey.m_key->m_isPressed = isPhysicallyPressed;
    }
    
    // create modifiers
    c.m_mkey.m_modifier = getCurrentModifiers(c.m_mkey.m_key,
					      isPhysicallyPressed);
    Keymap::AssignMode am;
    bool isModifier = fixModifierKey(&c.m_mkey, &am);
    if (m_isPrefix)
    {
      if (isModifier && m_doesIgnoreModifierForPrefix)
	am = Keymap::AM_true;
      if (m_doesEditNextModifier)
      {
	Modifier modifier = m_modifierForNextKey;
	modifier.add(c.m_mkey.m_modifier);
	c.m_mkey.m_modifier = modifier;
      }
    }
    
    if (m_isLogMode)
      outputToLog(&key, c.m_mkey, 0);
    else if (am == Keymap::AM_true)
    {
      {
	Acquire a(&m_log, 1);
	m_log << _T("* true modifier") << std::endl;
      }
      // true modifier doesn't generate scan code
      outputToLog(&key, c.m_mkey, 1);
    }
    else if (am == Keymap::AM_oneShot || am == Keymap::AM_oneShotRepeatable)
    {
      {
	Acquire a(&m_log, 1);
	if (am == Keymap::AM_oneShot)
	  m_log << _T("* one shot modifier") << std::endl;
	else
	  m_log << _T("* one shot repeatable modifier") << std::endl;
      }
      // oneShot modifier doesn't generate scan code
      outputToLog(&key, c.m_mkey, 1);
      if (isPhysicallyPressed)
      {
	if (am == Keymap::AM_oneShotRepeatable	// the key is repeating
	    && m_oneShotKey.m_key == c.m_mkey.m_key)
	{
	  if (m_oneShotRepeatableRepeatCount <
	      m_setting->m_oneShotRepeatableDelay) {
	    ; // delay
	  } else {
	    Current cnew = c;
	    beginGeneratingKeyboardEvents(cnew, false);
	  }
	  ++ m_oneShotRepeatableRepeatCount;
	} else {
	  m_oneShotKey = c.m_mkey;
	  m_oneShotRepeatableRepeatCount = 0;
	}
      }
      else
      {
	if (m_oneShotKey.m_key)
	{
	  Current cnew = c;
	  cnew.m_mkey.m_modifier = m_oneShotKey.m_modifier;
	  cnew.m_mkey.m_modifier.off(Modifier::Type_Up);
	  cnew.m_mkey.m_modifier.on(Modifier::Type_Down);
	  beginGeneratingKeyboardEvents(cnew, false);
	  
	  cnew = c;
	  cnew.m_mkey.m_modifier = m_oneShotKey.m_modifier;
	  cnew.m_mkey.m_modifier.on(Modifier::Type_Up);
	  cnew.m_mkey.m_modifier.off(Modifier::Type_Down);
	  beginGeneratingKeyboardEvents(cnew, false);
	}
	m_oneShotKey.m_key = NULL;
	m_oneShotRepeatableRepeatCount = 0;
      }
    }
    else if (c.m_mkey.m_key)
      // normal key
    {
      outputToLog(&key, c.m_mkey, 1);
      if (isPhysicallyPressed)
	m_oneShotKey.m_key = NULL;
      beginGeneratingKeyboardEvents(c, isModifier);
    }
    
    // if counter is zero, reset modifiers and keys on win32
    if (m_currentKeyPressCount <= 0)
    {
      {
	Acquire a(&m_log, 1);
	m_log << _T("* No key is pressed") << std::endl;
      }
      generateModifierEvents(Modifier());
      if (0 < m_currentKeyPressCountOnWin32)
	keyboardResetOnWin32();
      m_currentKeyPressCount = 0;
      m_currentKeyPressCountOnWin32 = 0;
      m_oneShotKey.m_key = NULL;
      if (m_currentLock.isOn(Modifier::Type_Touchpad) == false)
	m_currentLock.off(Modifier::Type_TouchpadSticky);
    }
    
    key.initialize();
    updateLastPressedKey(isPhysicallyPressed ? c.m_mkey.m_key : NULL);
  }
#if defined(_WINNT)
  break_while:
#endif
  CHECK_TRUE( SetEvent(m_threadEvent) );
}
  

Engine::Engine(tomsgstream &i_log)
  : m_hwndAssocWindow(NULL),
    m_setting(NULL),
    m_device(INVALID_HANDLE_VALUE),
    m_didMayuStartDevice(false),
    m_threadEvent(NULL),
    m_mayudVersion(_T("unknown")),
#if defined(_WINNT)
    m_readEvent(NULL),
    m_interruptThreadEvent(NULL),
    m_sts4mayu(NULL),
    m_cts4mayu(NULL),
#endif // _WINNT
    m_doForceTerminate(false),
    m_isLogMode(false),
    m_isEnabled(true),
    m_isSynchronizing(false),
    m_eSync(NULL),
    m_generateKeyboardEventsRecursionGuard(0),
    m_currentKeyPressCount(0),
    m_currentKeyPressCountOnWin32(0),
    m_lastGeneratedKey(NULL),
    m_oneShotRepeatableRepeatCount(0),
    m_isPrefix(false),
    m_currentKeymap(NULL),
    m_currentFocusOfThread(NULL),
    m_hwndFocus(NULL),
    m_afShellExecute(NULL),
    m_variable(0),
    m_log(i_log)
{
  for (size_t i = 0; i < NUMBER_OF(m_lastPressedKey); ++ i)
    m_lastPressedKey[i] = NULL;
    
  // set default lock state
  for (int i = 0; i < Modifier::Type_end; ++ i)
    m_currentLock.dontcare(static_cast<Modifier::Type>(i));
  for (int i = Modifier::Type_Lock0; i <= Modifier::Type_Lock9; ++ i)
    m_currentLock.release(static_cast<Modifier::Type>(i));

  if (!open()) {
      throw ErrorMessage() << loadString(IDS_driverNotInstalled);
  }
  
  {
    TCHAR versionBuf[256];
    DWORD length = 0;

    if (DeviceIoControl(m_device, IOCTL_MAYU_GET_VERSION, NULL, 0,
			versionBuf, sizeof(versionBuf), &length, NULL)
	&& length
	&& length < sizeof(versionBuf))			// fail safe
	m_mayudVersion = tstring(versionBuf, length / 2);
  }
  // create event for sync
  CHECK_TRUE( m_eSync = CreateEvent(NULL, FALSE, FALSE, NULL) );
#if defined(_WINNT)
  // create named pipe for &SetImeString
  m_hookPipe = CreateNamedPipe(addSessionId(HOOK_PIPE_NAME).c_str(),
			       PIPE_ACCESS_OUTBOUND,
			       PIPE_TYPE_BYTE, 1,
			       0, 0, 0, NULL);
#endif // _WINNT
  StrExprArg::setEngine(this);
}


// open mayu device
bool Engine::open()
{
  // open mayu m_device
#if defined(_WINNT)
  m_device = CreateFile(MAYU_DEVICE_FILE_NAME, GENERIC_READ | GENERIC_WRITE,
			0, NULL, OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
#elif defined(_WIN95)
  m_device = CreateFile(MAYU_DEVICE_FILE_NAME, 0,
			0, NULL, CREATE_NEW, FILE_FLAG_DELETE_ON_CLOSE, NULL);
#else
#  error
#endif

  if (m_device != INVALID_HANDLE_VALUE) {
    return true;
  }

#if defined(_WINNT)
  // start mayud
  SC_HANDLE hscm = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
  if (hscm)
  {
    SC_HANDLE hs = OpenService(hscm, MAYU_DRIVER_NAME, SERVICE_START);
    if (hs)
    {
      StartService(hs, 0, NULL);
      CloseServiceHandle(hs);
      m_didMayuStartDevice = true;
    }
    CloseServiceHandle(hscm);
  }
  
  // open mayu m_device
  m_device = CreateFile(MAYU_DEVICE_FILE_NAME, GENERIC_READ | GENERIC_WRITE,
			0, NULL, OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
#endif // _WINNT
  return (m_device != INVALID_HANDLE_VALUE);
}


// close mayu device
void Engine::close()
{
  if (m_device != INVALID_HANDLE_VALUE) {
    CHECK_TRUE( CloseHandle(m_device) );
  }
  m_device = INVALID_HANDLE_VALUE;
}


// start keyboard handler thread
void Engine::start()
{
  CHECK_TRUE( m_threadEvent = CreateEvent(NULL, FALSE, FALSE, NULL) );
  
#if defined(_WINNT)
  CHECK_TRUE( m_readEvent = CreateEvent(NULL, FALSE, FALSE, NULL) );
  CHECK_TRUE( m_interruptThreadEvent = CreateEvent(NULL, FALSE, FALSE, NULL) );
  m_ol.Offset = 0;
  m_ol.OffsetHigh = 0;
  m_ol.hEvent = m_readEvent;
#endif // _WINNT
  
  CHECK_TRUE( m_threadHandle = (HANDLE)_beginthreadex(NULL, 0, keyboardHandler, this, 0, &m_threadId) );
  CHECK( WAIT_OBJECT_0 ==, WaitForSingleObject(m_threadEvent, INFINITE) );
}


// stop keyboard handler thread
void Engine::stop()
{
  if (m_threadEvent)
  {
    m_doForceTerminate = true;
    do
    {
#if defined(_WINNT)
      m_interruptThreadReason = InterruptThreadReason_Terminate;
      SetEvent(m_interruptThreadEvent);
#elif defined(_WIN95)
      DeviceIoControl(m_device, 3, NULL, 0, NULL, 0, NULL, NULL);
#endif
      //DWORD buf;
      //M_DeviceIoControl(m_device, IOCTL_MAYU_DETOUR_CANCEL,
      //                &buf, sizeof(buf), &buf, sizeof(buf), &buf, NULL);
      
      // wait for message handler thread terminate
    } while (WaitForSingleObject(m_threadEvent, 100) != WAIT_OBJECT_0);
    CHECK_TRUE( CloseHandle(m_threadEvent) );
    m_threadEvent = NULL;
    WaitForSingleObject(m_threadHandle, 100);
    CHECK_TRUE( CloseHandle(m_threadHandle) );
    m_threadHandle = NULL;

#if defined(_WINNT)
    // stop mayud
    if (m_didMayuStartDevice)
    {
      SC_HANDLE hscm = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
      if (hscm)
      {
	SC_HANDLE hs = OpenService(hscm, MAYU_DRIVER_NAME, SERVICE_STOP);
	if (hs)
	{
	  SERVICE_STATUS ss;
	  ControlService(hs, SERVICE_CONTROL_STOP, &ss);
	  CloseServiceHandle(hs);
	}
	CloseServiceHandle(hscm);
      }
    }
    
    CHECK_TRUE( CloseHandle(m_readEvent) );
    m_readEvent = NULL;
    CHECK_TRUE( CloseHandle(m_interruptThreadEvent) );
    m_interruptThreadEvent = NULL;
#endif // _WINNT
  }
}

bool Engine::pause()
{
#if defined(_WINNT)
  if (m_device != INVALID_HANDLE_VALUE) {
    do {
      m_interruptThreadReason = InterruptThreadReason_Pause;
      SetEvent(m_interruptThreadEvent);
    } while (WaitForSingleObject(m_threadEvent, 100) != WAIT_OBJECT_0);
    close();
  }
#endif // _WINNT
  return true;
}


bool Engine::resume()
{
#if defined(_WINNT)
  if (m_device == INVALID_HANDLE_VALUE) {
    if (!open()) {
      return false;				// FIXME
    }
    do {
      m_interruptThreadReason = InterruptThreadReason_Resume;
      SetEvent(m_interruptThreadEvent);
    } while (WaitForSingleObject(m_threadEvent, 100) != WAIT_OBJECT_0);
  }
#endif // _WINNT
  return true;
}


bool Engine::prepairQuit()
{
  // terminate and unload DLL for ThumbSense support if loaded
  manageTs4mayu(_T("sts4mayu.dll"), _T("SynCOM.dll"),
		false, &m_sts4mayu);
  manageTs4mayu(_T("cts4mayu.dll"), _T("TouchPad.dll"),
		false, &m_cts4mayu);
  return true;
}


Engine::~Engine()
{
  stop();
  CHECK_TRUE( CloseHandle(m_eSync) );
  
  // close m_device
  close();
#if defined(_WINNT)
  // destroy named pipe for &SetImeString
  DisconnectNamedPipe(m_hookPipe);
  CHECK_TRUE( CloseHandle(m_hookPipe) );
#endif // _WINNT
}


void Engine::manageTs4mayu(TCHAR *i_ts4mayuDllName,
			   TCHAR *i_dependDllName,
			   bool i_load, HMODULE *i_pTs4mayu)
{
  Acquire a(&m_log, 0);

  if (i_load == false)
  {
    if (*i_pTs4mayu)
    {
      bool (WINAPI *pTs4mayuTerm)();

      pTs4mayuTerm = (bool (WINAPI*)())GetProcAddress(*i_pTs4mayu, "ts4mayuTerm");
      if (pTs4mayuTerm() == true)
	FreeLibrary(*i_pTs4mayu);
      *i_pTs4mayu = NULL;
      m_log << i_ts4mayuDllName <<_T(" unloaded") << std::endl;
    }
  }
  else
  {
    if (*i_pTs4mayu)
    {
      m_log << i_ts4mayuDllName << _T(" already loaded") << std::endl;
    }
    else
    {
      if (SearchPath(NULL, i_dependDllName, NULL, 0, NULL, NULL) == 0)
      {
	m_log << _T("load ") << i_ts4mayuDllName
	      << _T(" failed: can't find ") << i_dependDllName
	      << std::endl;
      }
      else
      {
	*i_pTs4mayu = LoadLibrary(i_ts4mayuDllName);
	if (*i_pTs4mayu == NULL)
	{
	  m_log << _T("load ") << i_ts4mayuDllName
		<< _T(" failed: can't find it") << std::endl;
	}
	else
	{
	  bool (WINAPI *pTs4mayuInit)(UINT);    

	  pTs4mayuInit = (bool (WINAPI*)(UINT))GetProcAddress(*i_pTs4mayu, "ts4mayuInit");
	  if (pTs4mayuInit(m_threadId) == true)
	    m_log << i_ts4mayuDllName <<_T(" loaded") << std::endl;
	  else
	    m_log << i_ts4mayuDllName
		  <<_T(" load failed: can't initialize") << std::endl;
	}
      }
    }
  }
}


// set m_setting
bool Engine::setSetting(Setting *i_setting)
{
  Acquire a(&m_cs);
  if (m_isSynchronizing)
    return false;

  if (m_setting)
  {
    for (Keyboard::KeyIterator i = m_setting->m_keyboard.getKeyIterator();
	 *i; ++ i)
    {
      Key *key = i_setting->m_keyboard.searchKey(*(*i));
      if (key)
      {
	key->m_isPressed = (*i)->m_isPressed;
	key->m_isPressedOnWin32 = (*i)->m_isPressedOnWin32;
	key->m_isPressedByAssign = (*i)->m_isPressedByAssign;
      }
    }
    if (m_lastGeneratedKey)
      m_lastGeneratedKey =
	i_setting->m_keyboard.searchKey(*m_lastGeneratedKey);
    for (size_t i = 0; i < NUMBER_OF(m_lastPressedKey); ++ i)
      if (m_lastPressedKey[i])
	m_lastPressedKey[i] =
	  i_setting->m_keyboard.searchKey(*m_lastPressedKey[i]);
  }
  
  m_setting = i_setting;

  manageTs4mayu(_T("sts4mayu.dll"), _T("SynCOM.dll"),
		m_setting->m_sts4mayu, &m_sts4mayu);
  manageTs4mayu(_T("cts4mayu.dll"), _T("TouchPad.dll"),
		m_setting->m_cts4mayu, &m_cts4mayu);

  g_hookData->m_correctKanaLockHandling = m_setting->m_correctKanaLockHandling;
  if (m_currentFocusOfThread)
  {
    for (FocusOfThreads::iterator i = m_focusOfThreads.begin();
	 i != m_focusOfThreads.end(); i ++)
    {
      FocusOfThread *fot = &(*i).second;
      m_setting->m_keymaps.searchWindow(&fot->m_keymaps,
					fot->m_className, fot->m_titleName);
    }
  }
  m_setting->m_keymaps.searchWindow(&m_globalFocus.m_keymaps, _T(""), _T(""));
  if (m_globalFocus.m_keymaps.empty())
  {
    Acquire a(&m_log, 0);
    m_log << _T("internal error: m_globalFocus.m_keymap is empty")
	  << std::endl;
  }
  m_currentFocusOfThread = &m_globalFocus;
  setCurrentKeymap(m_globalFocus.m_keymaps.front());
  m_hwndFocus = NULL;
  return true;
}


void Engine::checkShow(HWND i_hwnd)
{
  // update show style of window
  // this update should be done in hook DLL, but to
  // avoid update-loss for some applications(such as
  // cmd.exe), we update here.
  bool isMaximized = false;
  bool isMinimized = false;
  bool isMDIMaximized = false;
  bool isMDIMinimized = false;
  while (i_hwnd)
  {
    LONG exStyle = GetWindowLong(i_hwnd, GWL_EXSTYLE);
    if (exStyle & WS_EX_MDICHILD)
    {
      WINDOWPLACEMENT placement;
      placement.length = sizeof(WINDOWPLACEMENT);
      if (GetWindowPlacement(i_hwnd, &placement))
      {
	 switch (placement.showCmd)
	 {
	   case SW_SHOWMAXIMIZED:
	     isMDIMaximized = true;
	     break;
	   case SW_SHOWMINIMIZED:
	     isMDIMinimized = true;
	     break;
	   case SW_SHOWNORMAL:
	   default:
	     break;
	 }
      }
    }

    LONG style = GetWindowLong(i_hwnd, GWL_STYLE);
    if ((style & WS_CHILD) == 0)
    {
      WINDOWPLACEMENT placement;
      placement.length = sizeof(WINDOWPLACEMENT);
      if (GetWindowPlacement(i_hwnd, &placement))
      {
	 switch (placement.showCmd)
	 {
	   case SW_SHOWMAXIMIZED:
	     isMaximized = true;
	     break;
	   case SW_SHOWMINIMIZED:
	     isMinimized = true;
	     break;
	   case SW_SHOWNORMAL:
	   default:
	     break;
	 }
      }
    }  
    i_hwnd = GetParent(i_hwnd);
  }
  setShow(isMDIMaximized, isMDIMinimized, true);
  setShow(isMaximized, isMinimized, false);
}


// focus
bool Engine::setFocus(HWND i_hwndFocus, DWORD i_threadId, 
		      const tstringi &i_className, const tstringi &i_titleName,
		      bool i_isConsole)
{
  Acquire a(&m_cs);
  if (m_isSynchronizing)
    return false;
  if (i_hwndFocus == NULL)
    return true;

  // remove newly created thread's id from m_detachedThreadIds
  if (!m_detachedThreadIds.empty())
  {
    DetachedThreadIds::iterator i;
    do
    {
      for (i = m_detachedThreadIds.begin();
	   i != m_detachedThreadIds.end(); ++ i)
	if (*i == i_threadId)
	{
	  m_detachedThreadIds.erase(i);
	  break;
	}
    } while (i != m_detachedThreadIds.end());
  }
  
  FocusOfThread *fot;
  FocusOfThreads::iterator i = m_focusOfThreads.find(i_threadId);
  if (i != m_focusOfThreads.end())
  {
    fot = &(*i).second;
    if (fot->m_hwndFocus == i_hwndFocus &&
	fot->m_isConsole == i_isConsole &&
	fot->m_className == i_className &&
	fot->m_titleName == i_titleName)
      return true;
  }
  else
  {
    i = m_focusOfThreads.insert(
      FocusOfThreads::value_type(i_threadId, FocusOfThread())).first;
    fot = &(*i).second;
    fot->m_threadId = i_threadId;
  }
  fot->m_hwndFocus = i_hwndFocus;
  fot->m_isConsole = i_isConsole;
  fot->m_className = i_className;
  fot->m_titleName = i_titleName;
  
  if (m_setting)
  {
    m_setting->m_keymaps.searchWindow(&fot->m_keymaps,
				      i_className, i_titleName);
    ASSERT(0 < fot->m_keymaps.size());
  }
  else
    fot->m_keymaps.clear();
  checkShow(i_hwndFocus);
  return true;
}


// lock state
bool Engine::setLockState(bool i_isNumLockToggled,
			  bool i_isCapsLockToggled,
			  bool i_isScrollLockToggled,
			  bool i_isKanaLockToggled,
			  bool i_isImeLockToggled,
			  bool i_isImeCompToggled)
{
  Acquire a(&m_cs);
  if (m_isSynchronizing)
    return false;
  m_currentLock.on(Modifier::Type_NumLock, i_isNumLockToggled);
  m_currentLock.on(Modifier::Type_CapsLock, i_isCapsLockToggled);
  m_currentLock.on(Modifier::Type_ScrollLock, i_isScrollLockToggled);
  m_currentLock.on(Modifier::Type_KanaLock, i_isKanaLockToggled);
  m_currentLock.on(Modifier::Type_ImeLock, i_isImeLockToggled);
  m_currentLock.on(Modifier::Type_ImeComp, i_isImeCompToggled);
  return true;
}


// show
bool Engine::setShow(bool i_isMaximized, bool i_isMinimized,
		     bool i_isMDI)
{
  Acquire a(&m_cs);
  if (m_isSynchronizing)
    return false;
  Acquire b(&m_log, 1);
  Modifier::Type max, min;
  if (i_isMDI == true) {
    max = Modifier::Type_MdiMaximized;
    min = Modifier::Type_MdiMinimized;
  }
  else
  {
    max = Modifier::Type_Maximized;
    min = Modifier::Type_Minimized;
  }
  m_currentLock.on(max, i_isMaximized);
  m_currentLock.on(min, i_isMinimized);
  m_log << _T("Set show to ") << (i_isMaximized ? _T("Maximized") :
				  i_isMinimized ? _T("Minimized") : _T("Normal"));
  if (i_isMDI == true)
  {
    m_log << _T(" (MDI)");
  }
  m_log << std::endl;
  return true;
}


// sync
bool Engine::syncNotify()
{
  Acquire a(&m_cs);
  if (!m_isSynchronizing)
    return false;
  CHECK_TRUE( SetEvent(m_eSync) );
  return true;
}


// thread detach notify
bool Engine::threadDetachNotify(DWORD i_threadId)
{
  Acquire a(&m_cs);
  m_detachedThreadIds.push_back(i_threadId);
  return true;
}


// get help message
void Engine::getHelpMessages(tstring *o_helpMessage, tstring *o_helpTitle)
{
  Acquire a(&m_cs);
  *o_helpMessage = m_helpMessage;
  *o_helpTitle = m_helpTitle;
}


// command notify
void Engine::commandNotify(
  HWND i_hwnd, UINT i_message, WPARAM i_wParam, LPARAM i_lParam)
{
  Acquire b(&m_log, 0);
  HWND hf = m_hwndFocus;
  if (!hf)
    return;

  if (GetWindowThreadProcessId(hf, NULL) == 
      GetWindowThreadProcessId(m_hwndAssocWindow, NULL))
    return;	// inhibit the investigation of MADO TSUKAI NO YUUTSU

  const _TCHAR *target = NULL;
  int number_target = 0;
  
  if (i_hwnd == hf)
    target = _T("ToItself");
  else if (i_hwnd == GetParent(hf))
    target = _T("ToParentWindow");
  else
  {
    // Function::toMainWindow
    HWND h = hf;
    while (true)
    {
      HWND p = GetParent(h);
      if (!p)
	break;
      h = p;
    }
    if (i_hwnd == h)
      target = _T("ToMainWindow");
    else
    {
      // Function::toOverlappedWindow
      HWND h = hf;
      while (h)
      {
	LONG style = GetWindowLong(h, GWL_STYLE);
	if ((style & WS_CHILD) == 0)
	  break;
	h = GetParent(h);
      }
      if (i_hwnd == h)
	target = _T("ToOverlappedWindow");
      else
      {
	// number
	HWND h = hf;
	for (number_target = 0; h; number_target ++, h = GetParent(h))
	  if (i_hwnd == h)
	    break;
	return;
      }
    }
  }

  m_log << _T("&PostMessage(");
  if (target)
    m_log << target;
  else
    m_log << number_target;
  m_log << _T(", ") << i_message
	<< _T(", 0x") << std::hex << i_wParam
	<< _T(", 0x") << i_lParam << _T(") # hwnd = ")
	<< reinterpret_cast<int>(i_hwnd) << _T(", ")
	<< _T("message = ") << std::dec;
  if (i_message == WM_COMMAND)
    m_log << _T("WM_COMMAND, ");
  else if (i_message == WM_SYSCOMMAND)
    m_log << _T("WM_SYSCOMMAND, ");
  else
    m_log << i_message << _T(", ");
  m_log << _T("wNotifyCode = ") << HIWORD(i_wParam) << _T(", ")
	<< _T("wID = ") << LOWORD(i_wParam) << _T(", ")
	<< _T("hwndCtrl = 0x") << std::hex << i_lParam << std::dec << std::endl;
}
