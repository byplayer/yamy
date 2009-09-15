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

	if (hwndFore) {
		{
			Acquire a(&m_cs);
			if (m_currentFocusOfThread &&
					m_currentFocusOfThread->m_threadId == threadId &&
					m_currentFocusOfThread->m_hwndFocus == m_hwndFocus)
				return;

			m_emacsEditKillLine.reset();

			// erase dead thread
			if (!m_detachedThreadIds.empty()) {
				for (ThreadIds::iterator i = m_detachedThreadIds.begin();
						i != m_detachedThreadIds.end(); i ++) {
					FocusOfThreads::iterator j = m_focusOfThreads.find((*i));
					if (j != m_focusOfThreads.end()) {
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
			if (i != m_focusOfThreads.end()) {
				m_currentFocusOfThread = &((*i).second);
				if (!m_currentFocusOfThread->m_isConsole || 2 <= count) {
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
		if (GetClassName(hwndFore, className, NUMBER_OF(className))) {
			if (_tcsicmp(className, _T("ConsoleWindowClass")) == 0) {
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
	if (m_globalFocus.m_keymaps.empty()) {
		Acquire a(&m_log, 1);
		m_log << _T("NO GLOBAL FOCUS") << std::endl;
		m_currentFocusOfThread = NULL;
		setCurrentKeymap(NULL);
	} else {
		if (m_currentFocusOfThread != &m_globalFocus) {
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
	for (int i = Modifier::Type_begin; i != Modifier::Type_end; ++ i) {
		// get modifier assignments (list of modifier keys)
		const Keymap::ModAssignments &ma =
			m_currentKeymap->getModAssignments(static_cast<Modifier::Type>(i));

		for (Keymap::ModAssignments::const_iterator
				j = ma.begin(); j != ma.end(); ++ j)
			if (io_mkey->m_key == (*j).m_key) { // is io_mkey a modifier ?
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
	for (i = 0; i < i_key->getScanCodesSize(); ++ i) {
		if (i_key->getScanCodes()[i].m_flags & ScanCode::E0) m_log << _T("E0-");
		if (i_key->getScanCodes()[i].m_flags & ScanCode::E1) m_log << _T("E1-");
		if (!(i_key->getScanCodes()[i].m_flags & ScanCode::E0E1))
			m_log << _T("   ");
		m_log << _T("0x") << std::hex << std::setw(2) << std::setfill(_T('0'))
		<< static_cast<int>(i_key->getScanCodes()[i].m_scan)
		<< std::dec << _T(" ");
	}

	if (!i_mkey.m_key) { // key corresponds to no phisical key
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
	if (i_doesAddToHistory) {
		m_keymapPrefixHistory.push_back(const_cast<Keymap *>(m_currentKeymap));
		if (MAX_KEYMAP_PREFIX_HISTORY < m_keymapPrefixHistory.size())
			m_keymapPrefixHistory.pop_front();
	} else
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
	if (m_lastPressedKey[0] == i_key) {
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
		if (*e == i_key) {
			isEvent = true;
			break;
		}

	bool isAlreadyReleased = false;

	if (!isEvent) {
		if (i_doPress && !i_key->m_isPressedOnWin32)
			++ m_currentKeyPressCountOnWin32;
		else if (!i_doPress) {
			if (i_key->m_isPressedOnWin32)
				-- m_currentKeyPressCountOnWin32;
			else
				isAlreadyReleased = true;
		}
		i_key->m_isPressedOnWin32 = i_doPress;

		if (i_isByAssign)
			i_key->m_isPressedByAssign = i_doPress;

		Key *sync = m_setting->m_keyboard.getSyncKey();

		if (!isAlreadyReleased || i_key == sync) {
			KEYBOARD_INPUT_DATA kid = { 0, 0, 0, 0, 0 };
			const ScanCode *sc = i_key->getScanCodes();
			for (size_t i = 0; i < i_key->getScanCodesSize(); ++ i) {
				kid.MakeCode = sc[i].m_scan;
				kid.Flags = sc[i].m_flags;
				if (!i_doPress)
					kid.Flags |= KEYBOARD_INPUT_DATA::BREAK;
				injectInput(&kid, NULL);
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
				i_c.m_keymap->searchAssignment(i_c.m_mkey)) {
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

	for (int i = Modifier::Type_begin; i < Modifier::Type_BASIC; ++ i) {
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
			for (Keyboard::Mods::iterator i = mods.begin(); i != mods.end(); ++ i) {
				if ((*i)->m_isPressedOnWin32)
					noneIsPressed = false;
				if ((*i)->m_isPressedByAssign)
					noneIsPressedByAssign = false;
			}
			if (noneIsPressed) {
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
			if (i == Modifier::Type_Alt || i == Modifier::Type_Windows) {
				for (Keyboard::Mods::iterator j = mods.begin(); j != mods.end(); ++ j)
					if ((*j) == m_lastGeneratedKey) {
						Keyboard::Mods *mods =
							&m_setting->m_keyboard.getModifiers(Modifier::Type_Shift);
						if (mods->size() == 0)
							mods = &m_setting->m_keyboard.getModifiers(
									   Modifier::Type_Control);
						if (0 < mods->size()) {
							generateKeyEvent(mods->front(), true, false);
							generateKeyEvent(mods->front(), false, false);
						}
						break;
					}
			}

			for (Keyboard::Mods::iterator j = mods.begin(); j != mods.end(); ++ j) {
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
	switch (i_a->getType()) {
		// key
	case Action::Type_key: {
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
				  mkey.m_modifier.isDontcare(Modifier::Type_Down))) {
			Modifier modifier = mkey.m_modifier;
			modifier.add(i_c.m_mkey.m_modifier);
			generateModifierEvents(modifier);
			generateKeyEvent(mkey.m_key, true, true);
		}
		break;
	}

	// keyseq
	case Action::Type_keySeq: {
		const ActionKeySeq *aks = reinterpret_cast<const ActionKeySeq *>(i_a);
		generateKeySeqEvents(i_c, aks->m_keySeq,
							 i_doPress ? Part_down : Part_up);
		break;
	}

	// function
	case Action::Type_function: {
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

		if (param.m_doesNeedEndl) {
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
	else {
		size_t i;
		for (i = 0 ; i < actions.size() - 1; ++ i) {
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
			MAX_GENERATE_KEYBOARD_EVENTS_RECURSION_COUNT) {
		Acquire a(&m_log);
		m_log << _T("error: too deep keymap recursion.  there may be a loop.")
		<< std::endl;
		return;
	}

	const Keymap::KeyAssignment *keyAssign
	= i_c.m_keymap->searchAssignment(i_c.m_mkey);
	if (!keyAssign) {
		const KeySeq *keySeq = i_c.m_keymap->getDefaultKeySeq();
		ASSERT( keySeq );
		generateKeySeqEvents(i_c, keySeq, i_c.isPressed() ? Part_down : Part_up);
	} else {
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
	if (mkey.m_key) {
		cnew.m_mkey = mkey;
		if (isPhysicallyPressed) {
			cnew.m_mkey.m_modifier.off(Modifier::Type_Up);
			cnew.m_mkey.m_modifier.on(Modifier::Type_Down);
		} else {
			cnew.m_mkey.m_modifier.on(Modifier::Type_Up);
			cnew.m_mkey.m_modifier.off(Modifier::Type_Down);
		}
		for (int i = Modifier::Type_begin; i != Modifier::Type_end; ++ i) {
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


unsigned int Engine::injectInput(const KEYBOARD_INPUT_DATA *i_kid, const KBDLLHOOKSTRUCT *i_kidRaw)
{
	if (i_kid->Flags & KEYBOARD_INPUT_DATA::E1) {
		INPUT kid[2];
		int count = 1;

		kid[0].type = INPUT_MOUSE;
		kid[0].mi.dx = 0;
		kid[0].mi.dy = 0;
		kid[0].mi.time = 0;
		kid[0].mi.mouseData = 0;
		kid[0].mi.dwExtraInfo = 0;
		switch (i_kid->MakeCode) {
		case 1:
			if (i_kid->Flags & KEYBOARD_INPUT_DATA::BREAK) {
				kid[0].mi.dwFlags = MOUSEEVENTF_LEFTUP;
			} else {
				kid[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
			}
			break;
		case 2:
			if (i_kid->Flags & KEYBOARD_INPUT_DATA::BREAK) {
				kid[0].mi.dwFlags = MOUSEEVENTF_RIGHTUP;
			} else {
				kid[0].mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
			}
			break;
		case 3:
			if (i_kid->Flags & KEYBOARD_INPUT_DATA::BREAK) {
				kid[0].mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
			} else {
				kid[0].mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
			}
			break;
		case 4:
			if (i_kid->Flags & KEYBOARD_INPUT_DATA::BREAK) {
				return 1;
			} else {
				kid[0].mi.mouseData = WHEEL_DELTA;
				kid[0].mi.dwFlags = MOUSEEVENTF_WHEEL;
			}
			break;
		case 5:
			if (i_kid->Flags & KEYBOARD_INPUT_DATA::BREAK) {
				return 1;
			} else {
				kid[0].mi.mouseData = -WHEEL_DELTA;
				kid[0].mi.dwFlags = MOUSEEVENTF_WHEEL;
			}
			break;
		case 6:
			kid[0].mi.mouseData = XBUTTON1;
			if (i_kid->Flags & KEYBOARD_INPUT_DATA::BREAK) {
				kid[0].mi.dwFlags = MOUSEEVENTF_XUP;
			} else {
				kid[0].mi.dwFlags = MOUSEEVENTF_XDOWN;
			}
			break;
		case 7:
			kid[0].mi.mouseData = XBUTTON2;
			if (i_kid->Flags & KEYBOARD_INPUT_DATA::BREAK) {
				kid[0].mi.dwFlags = MOUSEEVENTF_XUP;
			} else {
				kid[0].mi.dwFlags = MOUSEEVENTF_XDOWN;
			}
			break;
		case 8:
			if (i_kid->Flags & KEYBOARD_INPUT_DATA::BREAK) {
				return 1;
			} else {
				kid[0].mi.mouseData = WHEEL_DELTA;
				kid[0].mi.dwFlags = MOUSEEVENTF_HWHEEL;
			}
			break;
		case 9:
			if (i_kid->Flags & KEYBOARD_INPUT_DATA::BREAK) {
				return 1;
			} else {
				kid[0].mi.mouseData = -WHEEL_DELTA;
				kid[0].mi.dwFlags = MOUSEEVENTF_HWHEEL;
			}
			break;
		default:
			return 1;
			break;
		}
		if (!(i_kid->Flags & KEYBOARD_INPUT_DATA::BREAK) &&
			i_kid->MakeCode != 4 && i_kid->MakeCode != 5 &&
			i_kid->MakeCode != 8 && i_kid->MakeCode != 9) {
			HWND hwnd;
			POINT pt;

			if (GetCursorPos(&pt) && (hwnd = WindowFromPoint(pt))) {
				_TCHAR className[GANA_MAX_ATOM_LENGTH];
				if (GetClassName(hwnd, className, NUMBER_OF(className))) {
					if (_tcsicmp(className, _T("ConsoleWindowClass")) == 0) {
						SetForegroundWindow(hwnd);
					}
				}
			}
			if (m_dragging) {
				kid[0].mi.dx = 65535 * m_msllHookCurrent.pt.x / GetSystemMetrics(SM_CXVIRTUALSCREEN);
				kid[0].mi.dy = 65535 * m_msllHookCurrent.pt.y / GetSystemMetrics(SM_CYVIRTUALSCREEN);
				kid[0].mi.dwFlags |= MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK;

				kid[1].type = INPUT_MOUSE;
				kid[1].mi.dx = 65535 * pt.x / GetSystemMetrics(SM_CXVIRTUALSCREEN);
				kid[1].mi.dy = 65535 * pt.y / GetSystemMetrics(SM_CYVIRTUALSCREEN);
				kid[1].mi.time = 0;
				kid[1].mi.mouseData = 0;
				kid[1].mi.dwExtraInfo = 0;
				kid[1].mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK;

				count = 2;
			}
		}
		SendInput(count, &kid[0], sizeof(kid[0]));
	} else {
		INPUT kid;

		kid.type = INPUT_KEYBOARD;
		kid.ki.wVk = 0;
		kid.ki.wScan = i_kid->MakeCode;
		kid.ki.dwFlags = KEYEVENTF_SCANCODE;
		kid.ki.time = i_kidRaw ? i_kidRaw->time : 0;
		kid.ki.dwExtraInfo = i_kidRaw ? i_kidRaw->dwExtraInfo : 0;
		if (i_kid->Flags & KEYBOARD_INPUT_DATA::BREAK) {
			kid.ki.dwFlags |= KEYEVENTF_KEYUP;
		}
		if (i_kid->Flags & KEYBOARD_INPUT_DATA::E0) {
			kid.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
		}
		SendInput(1, &kid, sizeof(kid));
	}
	return 1;
}


// pop all pressed key on win32
void Engine::keyboardResetOnWin32()
{
	for (Keyboard::KeyIterator
			i = m_setting->m_keyboard.getKeyIterator();  *i; ++ i) {
		if ((*i)->m_isPressedOnWin32)
			generateKeyEvent((*i), false, true);
	}
}


unsigned int WINAPI Engine::keyboardDetour(Engine *i_this, WPARAM i_wParam, LPARAM i_lParam)
{
	return i_this->keyboardDetour(reinterpret_cast<KBDLLHOOKSTRUCT*>(i_lParam));
}

unsigned int Engine::keyboardDetour(KBDLLHOOKSTRUCT *i_kid)
{
#if 0
	Acquire a(&m_log, 1);
	m_log << std::hex
	<< _T("keyboardDetour: vkCode=") << i_kid->vkCode
	<< _T(" scanCode=") << i_kid->scanCode
	<< _T(" flags=") << i_kid->flags << std::endl;
#endif
	if ((i_kid->flags & LLKHF_INJECTED) || !m_isEnabled) {
		return 0;
	} else {
		Key key;
		KEYBOARD_INPUT_DATA kid;

		kid.UnitId = 0;
		kid.MakeCode = i_kid->scanCode;
		kid.Flags = 0;
		if (i_kid->flags & LLKHF_UP) {
			kid.Flags |= KEYBOARD_INPUT_DATA::BREAK;
		}
		if (i_kid->flags & LLKHF_EXTENDED) {
			kid.Flags |= KEYBOARD_INPUT_DATA::E0;
		}
		kid.Reserved = 0;
		kid.ExtraInformation = 0;

		WaitForSingleObject(m_queueMutex, INFINITE);
		m_inputQueue->push_back(kid);
		SetEvent(m_readEvent);
		ReleaseMutex(m_queueMutex);
		return 1;
	}
}

unsigned int WINAPI Engine::mouseDetour(Engine *i_this, WPARAM i_wParam, LPARAM i_lParam)
{
	return i_this->mouseDetour(i_wParam, reinterpret_cast<MSLLHOOKSTRUCT*>(i_lParam));
}

unsigned int Engine::mouseDetour(WPARAM i_message, MSLLHOOKSTRUCT *i_mid)
{
	if (i_mid->flags & LLMHF_INJECTED || !m_isEnabled || !m_setting || !m_setting->m_mouseEvent) {
		return 0;
	} else {
		KEYBOARD_INPUT_DATA kid;

		kid.UnitId = 0;
		kid.Flags = KEYBOARD_INPUT_DATA::E1;
		kid.Reserved = 0;
		kid.ExtraInformation = 0;
		switch (i_message) {
		case WM_LBUTTONUP:
			kid.Flags |= KEYBOARD_INPUT_DATA::BREAK;
		case WM_LBUTTONDOWN:
			kid.MakeCode = 1;
			break;
		case WM_RBUTTONUP:
			kid.Flags |= KEYBOARD_INPUT_DATA::BREAK;
		case WM_RBUTTONDOWN:
			kid.MakeCode = 2;
			break;
		case WM_MBUTTONUP:
			kid.Flags |= KEYBOARD_INPUT_DATA::BREAK;
		case WM_MBUTTONDOWN:
			kid.MakeCode = 3;
			break;
		case WM_MOUSEWHEEL:
			if (i_mid->mouseData & (1<<31)) {
				kid.MakeCode = 5;
			} else {
				kid.MakeCode = 4;
			}
			break;
		case WM_XBUTTONUP:
			kid.Flags |= KEYBOARD_INPUT_DATA::BREAK;
		case WM_XBUTTONDOWN:
			switch ((i_mid->mouseData >> 16) & 0xFFFFU) {
			case XBUTTON1:
				kid.MakeCode = 6;
				break;
			case XBUTTON2:
				kid.MakeCode = 7;
				break;
			default:
				return 0;
				break;
			}
			break;
		case WM_MOUSEHWHEEL:
			if (i_mid->mouseData & (1<<31)) {
				kid.MakeCode = 9;
			} else {
				kid.MakeCode = 8;
			}
			break;
		case WM_MOUSEMOVE: {
			LONG dx = i_mid->pt.x - g_hookData->m_mousePos.x;
			LONG dy = i_mid->pt.y - g_hookData->m_mousePos.y;
			HWND target = reinterpret_cast<HWND>(g_hookData->m_hwndMouseHookTarget);

			LONG dr = 0;
			dr += (i_mid->pt.x - m_msllHookCurrent.pt.x) * (i_mid->pt.x - m_msllHookCurrent.pt.x);
			dr += (i_mid->pt.y - m_msllHookCurrent.pt.y) * (i_mid->pt.y - m_msllHookCurrent.pt.y);
			if (m_buttonPressed && !m_dragging && m_setting->m_dragThreshold &&
				(m_setting->m_dragThreshold * m_setting->m_dragThreshold < dr)) {
				kid.MakeCode = 0;
				WaitForSingleObject(m_queueMutex, INFINITE);
				m_dragging = true;
				m_inputQueue->push_back(kid);
				SetEvent(m_readEvent);
				ReleaseMutex(m_queueMutex);
			}

			switch (g_hookData->m_mouseHookType) {
			case MouseHookType_Wheel:
				// For this type, g_hookData->m_mouseHookParam means
				// translate rate mouse move to wheel.
				mouse_event(MOUSEEVENTF_WHEEL, 0, 0,
							g_hookData->m_mouseHookParam * dy, 0);
				return 1;
				break;
			case MouseHookType_WindowMove: {
				RECT curRect;

				if (!GetWindowRect(target, &curRect))
					return 0;

				// g_hookData->m_mouseHookParam < 0 means
				// target window to move is MDI.
				if (g_hookData->m_mouseHookParam < 0) {
					HWND parent = GetParent(target);
					POINT p = {curRect.left, curRect.top};

					if (parent == NULL || !ScreenToClient(parent, &p))
						return 0;

					curRect.left = p.x;
					curRect.top = p.y;
				}

				SetWindowPos(target, NULL,
							 curRect.left + dx,
							 curRect.top + dy,
							 0, 0,
							 SWP_ASYNCWINDOWPOS | SWP_NOACTIVATE |
							 SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
				g_hookData->m_mousePos = i_mid->pt;
				return 0;
				break;
			}
			case MouseHookType_None:
			default:
				return 0;
				break;
			}
		}
		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDBLCLK:
		case WM_MBUTTONDBLCLK:
		case WM_XBUTTONDBLCLK:
		default:
			return 0;
			break;
		}

		WaitForSingleObject(m_queueMutex, INFINITE);

		if (kid.Flags & KEYBOARD_INPUT_DATA::BREAK) {
			m_buttonPressed = false;
			if (m_dragging) {
				KEYBOARD_INPUT_DATA kid2;

				m_dragging = false;
				kid2.UnitId = 0;
				kid2.Flags = KEYBOARD_INPUT_DATA::E1 | KEYBOARD_INPUT_DATA::BREAK;
				kid2.Reserved = 0;
				kid2.ExtraInformation = 0;
				kid2.MakeCode = 0;
				m_inputQueue->push_back(kid2);
			}
		} else if (i_message != WM_MOUSEWHEEL && i_message != WM_MOUSEHWHEEL) {
			m_buttonPressed = true;
			m_msllHookCurrent = *i_mid;
		}

		m_inputQueue->push_back(kid);

		if (i_message == WM_MOUSEWHEEL || i_message == WM_MOUSEHWHEEL) {
			kid.UnitId = 0;
			kid.Flags |= KEYBOARD_INPUT_DATA::BREAK;
			kid.Reserved = 0;
			kid.ExtraInformation = 0;
			m_inputQueue->push_back(kid);
		}

		SetEvent(m_readEvent);
		ReleaseMutex(m_queueMutex);

		return 1;
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
	// loop
	Key key;
	while (1) {
		KEYBOARD_INPUT_DATA kid;

		WaitForSingleObject(m_queueMutex, INFINITE);
		while (SignalObjectAndWait(m_queueMutex, m_readEvent, INFINITE, true) == WAIT_OBJECT_0) {
			if (m_inputQueue == NULL) {
				ReleaseMutex(m_queueMutex);
				return;
			}

			if (m_inputQueue->empty()) {
				ResetEvent(m_readEvent);
				continue;
			}

			kid = m_inputQueue->front();
			m_inputQueue->pop_front();
			if (m_inputQueue->empty()) {
				ResetEvent(m_readEvent);
			}

			break;

#if 0
			case WAIT_OBJECT_0 + NUMBER_OF(handles): {
				MSG message;

				while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE)) {
					switch (message.message) {
					case WM_APP + 201: {
						if (message.wParam) {
							m_currentLock.on(Modifier::Type_Touchpad);
							m_currentLock.on(Modifier::Type_TouchpadSticky);
						} else
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
#endif
		}
		ReleaseMutex(m_queueMutex);

		checkFocusWindow();

		if (!m_setting ||	// m_setting has not been loaded
				!m_isEnabled) {	// disabled
			if (m_isLogMode) {
				Key key;
				key.addScanCode(ScanCode(kid.MakeCode, kid.Flags));
				outputToLog(&key, ModifiedKey(), 0);
				if (kid.Flags & KEYBOARD_INPUT_DATA::E1) {
					// through mouse event even if log mode
					injectInput(&kid, NULL);
				}
			} else {
				injectInput(&kid, NULL);
			}
			updateLastPressedKey(NULL);
			continue;
		}

		Acquire a(&m_cs);

		if (!m_currentFocusOfThread ||
				!m_currentKeymap) {
			injectInput(&kid, NULL);
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
		if (!c.m_mkey.m_key) {
			c.m_mkey.m_key = m_setting->m_keyboard.searchPrefixKey(key);
			if (c.m_mkey.m_key)
				continue;
		}

		// press the key and update counter
		bool isPhysicallyPressed
		= !(key.getScanCodes()[0].m_flags & ScanCode::BREAK);
		if (c.m_mkey.m_key) {
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
		if (m_isPrefix) {
			if (isModifier && m_doesIgnoreModifierForPrefix)
				am = Keymap::AM_true;
			if (m_doesEditNextModifier) {
				Modifier modifier = m_modifierForNextKey;
				modifier.add(c.m_mkey.m_modifier);
				c.m_mkey.m_modifier = modifier;
			}
		}

		if (m_isLogMode) {
			outputToLog(&key, c.m_mkey, 0);
			if (kid.Flags & KEYBOARD_INPUT_DATA::E1) {
				// through mouse event even if log mode
				injectInput(&kid, NULL);
			}
		} else if (am == Keymap::AM_true) {
			{
				Acquire a(&m_log, 1);
				m_log << _T("* true modifier") << std::endl;
			}
			// true modifier doesn't generate scan code
			outputToLog(&key, c.m_mkey, 1);
		} else if (am == Keymap::AM_oneShot || am == Keymap::AM_oneShotRepeatable) {
			{
				Acquire a(&m_log, 1);
				if (am == Keymap::AM_oneShot)
					m_log << _T("* one shot modifier") << std::endl;
				else
					m_log << _T("* one shot repeatable modifier") << std::endl;
			}
			// oneShot modifier doesn't generate scan code
			outputToLog(&key, c.m_mkey, 1);
			if (isPhysicallyPressed) {
				if (am == Keymap::AM_oneShotRepeatable	// the key is repeating
						&& m_oneShotKey.m_key == c.m_mkey.m_key) {
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
			} else {
				if (m_oneShotKey.m_key) {
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
		} else if (c.m_mkey.m_key) {
			// normal key
			outputToLog(&key, c.m_mkey, 1);
			if (isPhysicallyPressed)
				m_oneShotKey.m_key = NULL;
			beginGeneratingKeyboardEvents(c, isModifier);
		} else {
			// undefined key
			if (kid.Flags & KEYBOARD_INPUT_DATA::E1) {
				// through mouse event even if undefined for fail safe
				injectInput(&kid, NULL);
			}
		}

		// if counter is zero, reset modifiers and keys on win32
		if (m_currentKeyPressCount <= 0) {
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
}


Engine::Engine(tomsgstream &i_log)
		: m_hwndAssocWindow(NULL),
		m_setting(NULL),
		m_buttonPressed(false),
		m_dragging(false),
		m_keyboardHandler(installKeyboardHook, Engine::keyboardDetour),
		m_mouseHandler(installMouseHook, Engine::mouseDetour),
		m_inputQueue(NULL),
		m_readEvent(NULL),
		m_queueMutex(NULL),
		m_sts4mayu(NULL),
		m_cts4mayu(NULL),
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
		m_log(i_log) {
	BOOL (WINAPI *pChangeWindowMessageFilter)(UINT, DWORD) =
		reinterpret_cast<BOOL (WINAPI*)(UINT, DWORD)>(GetProcAddress(GetModuleHandle(_T("user32.dll")), "ChangeWindowMessageFilter"));

	if(pChangeWindowMessageFilter != NULL) {
		pChangeWindowMessageFilter(WM_COPYDATA, MSGFLT_ADD);
	}

	for (size_t i = 0; i < NUMBER_OF(m_lastPressedKey); ++ i)
		m_lastPressedKey[i] = NULL;

	// set default lock state
	for (int i = 0; i < Modifier::Type_end; ++ i)
		m_currentLock.dontcare(static_cast<Modifier::Type>(i));
	for (int i = Modifier::Type_Lock0; i <= Modifier::Type_Lock9; ++ i)
		m_currentLock.release(static_cast<Modifier::Type>(i));

	// create event for sync
	CHECK_TRUE( m_eSync = CreateEvent(NULL, FALSE, FALSE, NULL) );
	// create named pipe for &SetImeString
	m_hookPipe = CreateNamedPipe(addSessionId(HOOK_PIPE_NAME).c_str(),
								 PIPE_ACCESS_OUTBOUND,
								 PIPE_TYPE_BYTE, 1,
								 0, 0, 0, NULL);
	StrExprArg::setEngine(this);

	m_msllHookCurrent.pt.x = 0;
	m_msllHookCurrent.pt.y = 0;
	m_msllHookCurrent.mouseData = 0;
	m_msllHookCurrent.flags = 0;
	m_msllHookCurrent.time = 0;
	m_msllHookCurrent.dwExtraInfo = 0;
}




// start keyboard handler thread
void Engine::start() {
	m_keyboardHandler.start(this);
	m_mouseHandler.start(this);

	CHECK_TRUE( m_inputQueue = new std::deque<KEYBOARD_INPUT_DATA> );
	CHECK_TRUE( m_queueMutex = CreateMutex(NULL, FALSE, NULL) );
	CHECK_TRUE( m_readEvent = CreateEvent(NULL, TRUE, FALSE, NULL) );
	m_ol.Offset = 0;
	m_ol.OffsetHigh = 0;
	m_ol.hEvent = m_readEvent;

	CHECK_TRUE( m_threadHandle = (HANDLE)_beginthreadex(NULL, 0, keyboardHandler, this, 0, &m_threadId) );
}


// stop keyboard handler thread
void Engine::stop() {
	m_mouseHandler.stop();
	m_keyboardHandler.stop();

	WaitForSingleObject(m_queueMutex, INFINITE);
	delete m_inputQueue;
	m_inputQueue = NULL;
	SetEvent(m_readEvent);
	ReleaseMutex(m_queueMutex);

	WaitForSingleObject(m_threadHandle, 2000);
	CHECK_TRUE( CloseHandle(m_threadHandle) );
	m_threadHandle = NULL;

	CHECK_TRUE( CloseHandle(m_readEvent) );
	m_readEvent = NULL;

	for (ThreadIds::iterator i = m_attachedThreadIds.begin();
		 i != m_attachedThreadIds.end(); i++) {
		 PostThreadMessage(*i, WM_NULL, 0, 0);
	}
}


bool Engine::prepairQuit() {
	// terminate and unload DLL for ThumbSense support if loaded
	manageTs4mayu(_T("sts4mayu.dll"), _T("SynCOM.dll"),
				  false, &m_sts4mayu);
	manageTs4mayu(_T("cts4mayu.dll"), _T("TouchPad.dll"),
				  false, &m_cts4mayu);
	return true;
}


Engine::~Engine() {
	CHECK_TRUE( CloseHandle(m_eSync) );

	// destroy named pipe for &SetImeString
	if (m_hookPipe && m_hookPipe != INVALID_HANDLE_VALUE) {
		DisconnectNamedPipe(m_hookPipe);
		CHECK_TRUE( CloseHandle(m_hookPipe) );
	}
}


void Engine::manageTs4mayu(TCHAR *i_ts4mayuDllName,
						   TCHAR *i_dependDllName,
						   bool i_load, HMODULE *i_pTs4mayu) {
	Acquire a(&m_log, 0);

	if (i_load == false) {
		if (*i_pTs4mayu) {
			bool (WINAPI *pTs4mayuTerm)();

			pTs4mayuTerm = (bool (WINAPI*)())GetProcAddress(*i_pTs4mayu, "ts4mayuTerm");
			if (pTs4mayuTerm() == true)
				FreeLibrary(*i_pTs4mayu);
			*i_pTs4mayu = NULL;
			m_log << i_ts4mayuDllName <<_T(" unloaded") << std::endl;
		}
	} else {
		if (*i_pTs4mayu) {
			m_log << i_ts4mayuDllName << _T(" already loaded") << std::endl;
		} else {
			if (SearchPath(NULL, i_dependDllName, NULL, 0, NULL, NULL) == 0) {
				m_log << _T("load ") << i_ts4mayuDllName
				<< _T(" failed: can't find ") << i_dependDllName
				<< std::endl;
			} else {
				*i_pTs4mayu = LoadLibrary(i_ts4mayuDllName);
				if (*i_pTs4mayu == NULL) {
					m_log << _T("load ") << i_ts4mayuDllName
					<< _T(" failed: can't find it") << std::endl;
				} else {
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
bool Engine::setSetting(Setting *i_setting) {
	Acquire a(&m_cs);
	if (m_isSynchronizing)
		return false;

	if (m_setting) {
		for (Keyboard::KeyIterator i = m_setting->m_keyboard.getKeyIterator();
				*i; ++ i) {
			Key *key = i_setting->m_keyboard.searchKey(*(*i));
			if (key) {
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
	if (m_currentFocusOfThread) {
		for (FocusOfThreads::iterator i = m_focusOfThreads.begin();
				i != m_focusOfThreads.end(); i ++) {
			FocusOfThread *fot = &(*i).second;
			m_setting->m_keymaps.searchWindow(&fot->m_keymaps,
											  fot->m_className, fot->m_titleName);
		}
	}
	m_setting->m_keymaps.searchWindow(&m_globalFocus.m_keymaps, _T(""), _T(""));
	if (m_globalFocus.m_keymaps.empty()) {
		Acquire a(&m_log, 0);
		m_log << _T("internal error: m_globalFocus.m_keymap is empty")
		<< std::endl;
	}
	m_currentFocusOfThread = &m_globalFocus;
	setCurrentKeymap(m_globalFocus.m_keymaps.front());
	m_hwndFocus = NULL;
	return true;
}


void Engine::checkShow(HWND i_hwnd) {
	// update show style of window
	// this update should be done in hook DLL, but to
	// avoid update-loss for some applications(such as
	// cmd.exe), we update here.
	bool isMaximized = false;
	bool isMinimized = false;
	bool isMDIMaximized = false;
	bool isMDIMinimized = false;
	while (i_hwnd) {
#ifdef MAYU64
		LONG_PTR exStyle = GetWindowLongPtr(i_hwnd, GWL_EXSTYLE);
#else
		LONG exStyle = GetWindowLong(i_hwnd, GWL_EXSTYLE);
#endif
		if (exStyle & WS_EX_MDICHILD) {
			WINDOWPLACEMENT placement;
			placement.length = sizeof(WINDOWPLACEMENT);
			if (GetWindowPlacement(i_hwnd, &placement)) {
				switch (placement.showCmd) {
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

#ifdef MAYU64
		LONG_PTR style = GetWindowLongPtr(i_hwnd, GWL_STYLE);
#else
		LONG style = GetWindowLong(i_hwnd, GWL_STYLE);
#endif
		if ((style & WS_CHILD) == 0) {
			WINDOWPLACEMENT placement;
			placement.length = sizeof(WINDOWPLACEMENT);
			if (GetWindowPlacement(i_hwnd, &placement)) {
				switch (placement.showCmd) {
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
					  bool i_isConsole) {
	Acquire a(&m_cs);
	if (m_isSynchronizing)
		return false;
	if (i_hwndFocus == NULL)
		return true;

	// remove newly created thread's id from m_detachedThreadIds
	if (!m_detachedThreadIds.empty()) {
		ThreadIds::iterator i;
		bool retry;
		do {
			retry = false;
			for (i = m_detachedThreadIds.begin();
					i != m_detachedThreadIds.end(); ++ i)
				if (*i == i_threadId) {
					m_detachedThreadIds.erase(i);
					retry = true;
					break;
				}
		} while (retry);
	}

	FocusOfThread *fot;
	FocusOfThreads::iterator i = m_focusOfThreads.find(i_threadId);
	if (i != m_focusOfThreads.end()) {
		fot = &(*i).second;
		if (fot->m_hwndFocus == i_hwndFocus &&
				fot->m_isConsole == i_isConsole &&
				fot->m_className == i_className &&
				fot->m_titleName == i_titleName)
			return true;
	} else {
		i = m_focusOfThreads.insert(
				FocusOfThreads::value_type(i_threadId, FocusOfThread())).first;
		fot = &(*i).second;
		fot->m_threadId = i_threadId;
	}
	fot->m_hwndFocus = i_hwndFocus;
	fot->m_isConsole = i_isConsole;
	fot->m_className = i_className;
	fot->m_titleName = i_titleName;

	if (m_setting) {
		m_setting->m_keymaps.searchWindow(&fot->m_keymaps,
										  i_className, i_titleName);
		ASSERT(0 < fot->m_keymaps.size());
	} else
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
						  bool i_isImeCompToggled) {
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
					 bool i_isMDI) {
	Acquire a(&m_cs);
	if (m_isSynchronizing)
		return false;
	Acquire b(&m_log, 1);
	Modifier::Type max, min;
	if (i_isMDI == true) {
		max = Modifier::Type_MdiMaximized;
		min = Modifier::Type_MdiMinimized;
	} else {
		max = Modifier::Type_Maximized;
		min = Modifier::Type_Minimized;
	}
	m_currentLock.on(max, i_isMaximized);
	m_currentLock.on(min, i_isMinimized);
	m_log << _T("Set show to ") << (i_isMaximized ? _T("Maximized") :
									i_isMinimized ? _T("Minimized") : _T("Normal"));
	if (i_isMDI == true) {
		m_log << _T(" (MDI)");
	}
	m_log << std::endl;
	return true;
}


// sync
bool Engine::syncNotify() {
	Acquire a(&m_cs);
	if (!m_isSynchronizing)
		return false;
	CHECK_TRUE( SetEvent(m_eSync) );
	return true;
}


// thread attach notify
bool Engine::threadAttachNotify(DWORD i_threadId) {
	Acquire a(&m_cs);
	m_attachedThreadIds.push_back(i_threadId);
	return true;
}


// thread detach notify
bool Engine::threadDetachNotify(DWORD i_threadId) {
	Acquire a(&m_cs);
	m_detachedThreadIds.push_back(i_threadId);
	m_attachedThreadIds.erase(remove(m_attachedThreadIds.begin(), m_attachedThreadIds.end(), i_threadId),
							  m_attachedThreadIds.end());
	return true;
}


// get help message
void Engine::getHelpMessages(tstring *o_helpMessage, tstring *o_helpTitle) {
	Acquire a(&m_cs);
	*o_helpMessage = m_helpMessage;
	*o_helpTitle = m_helpTitle;
}


unsigned int WINAPI Engine::InputHandler::run(void *i_this)
{
	reinterpret_cast<InputHandler*>(i_this)->run();
	_endthreadex(0);
	return 0;
}

Engine::InputHandler::InputHandler(INSTALL_HOOK i_installHook, INPUT_DETOUR i_inputDetour)
	: m_installHook(i_installHook), m_inputDetour(i_inputDetour)
{
	CHECK_TRUE(m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL));
	CHECK_TRUE(m_hThread = (HANDLE)_beginthreadex(NULL, 0, run, this, CREATE_SUSPENDED, &m_threadId));
}

Engine::InputHandler::~InputHandler()
{
	CloseHandle(m_hEvent);
}

void Engine::InputHandler::run()
{
	MSG msg;

	CHECK_FALSE(m_installHook(m_inputDetour, m_engine, true));
	PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
	SetEvent(m_hEvent);

	while (GetMessage(&msg, NULL, 0, 0)) {
		// nothing to do...
	}

	CHECK_FALSE(m_installHook(m_inputDetour, m_engine, false));

	return;
}

int Engine::InputHandler::start(Engine *i_engine)
{
	m_engine = i_engine;
	ResumeThread(m_hThread);
	WaitForSingleObject(m_hEvent, INFINITE);
	return 0;
}

int Engine::InputHandler::stop()
{
	PostThreadMessage(m_threadId, WM_QUIT, 0, 0);
	WaitForSingleObject(m_hThread, INFINITE);
	return 0;
}
