/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2006 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "CArchMiscWindows.h"
#include "CMSWindowsKeyState.h"
#include "CConfig.h"
#include "CHotkeyOptions.h"
#include "CStringUtil.h"
#include "LaunchUtil.h"
#include "resource.h"

#if !defined(WM_XBUTTONDOWN)
#define WM_XBUTTONDOWN		0x020B
#define WM_XBUTTONUP		0x020C
#define WM_XBUTTONDBLCLK	0x020D
#define XBUTTON1			0x0001
#define XBUTTON2			0x0002
#endif

//
// CAdvancedOptions
//

CHotkeyOptions*			CHotkeyOptions::s_singleton = NULL;

CHotkeyOptions::CHotkeyOptions(HWND parent, CConfig* config) :
	m_parent(parent),
	m_config(config)
{
	assert(s_singleton == NULL);
	s_singleton = this;
}

CHotkeyOptions::~CHotkeyOptions()
{
	s_singleton = NULL;
}

void
CHotkeyOptions::doModal()
{
	// do dialog
	m_inputFilter = m_config->getInputFilter();
	DialogBoxParam(s_instance, MAKEINTRESOURCE(IDD_HOTKEY_OPTIONS),
								m_parent, dlgProc, (LPARAM)this);
}

void
CHotkeyOptions::doInit(HWND hwnd)
{
	m_activeRuleIndex = (UInt32)-1;
	fillHotkeys(hwnd);
	openRule(hwnd);
}

void
CHotkeyOptions::fillHotkeys(HWND hwnd, UInt32 select)
{
	HWND rules = getItem(hwnd, IDC_HOTKEY_HOTKEYS);

	SendMessage(rules, LB_RESETCONTENT, 0, 0);
	for (UInt32 i = 0, n = m_inputFilter->getNumRules(); i < n; ++i) {
		CInputFilter::CRule& rule = m_inputFilter->getRule(i);
		SendMessage(rules, LB_INSERTSTRING, (WPARAM)-1,
							(LPARAM)rule.getCondition()->format().c_str());
	}

	if (select < m_inputFilter->getNumRules()) {
		SendMessage(rules, LB_SETCURSEL, select, 0);
	}

	updateHotkeysControls(hwnd);
}

void
CHotkeyOptions::updateHotkeysControls(HWND hwnd)
{
	HWND child = getItem(hwnd, IDC_HOTKEY_HOTKEYS);
	bool selected = (SendMessage(child, LB_GETCURSEL, 0, 0) != LB_ERR);

	enableItem(hwnd, IDC_HOTKEY_ADD_HOTKEY, TRUE);
	enableItem(hwnd, IDC_HOTKEY_EDIT_HOTKEY, selected);
	enableItem(hwnd, IDC_HOTKEY_REMOVE_HOTKEY, selected);
}

void
CHotkeyOptions::addHotkey(HWND hwnd)
{
	closeRule(hwnd);
	CInputFilter::CCondition* condition = NULL;
	if (editCondition(hwnd, condition)) {
		m_inputFilter->addFilterRule(CInputFilter::CRule(condition));
		fillHotkeys(hwnd, m_inputFilter->getNumRules() - 1);
	}
	else {
		delete condition;
	}
	openRule(hwnd);
}

void
CHotkeyOptions::removeHotkey(HWND hwnd)
{
	UInt32 ruleIndex = m_activeRuleIndex;
	closeRule(hwnd);

	m_inputFilter->removeFilterRule(ruleIndex);
	UInt32 n = m_inputFilter->getNumRules();
	if (n > 0 && ruleIndex >= n) {
		ruleIndex = n - 1;
	}
	fillHotkeys(hwnd, ruleIndex);

	openRule(hwnd);
}

void
CHotkeyOptions::editHotkey(HWND hwnd)
{
	// save selected item in action list
	HWND actions = getItem(hwnd, IDC_HOTKEY_ACTIONS);
	LRESULT aIndex = SendMessage(actions, LB_GETCURSEL, 0, 0);

	UInt32 index = m_activeRuleIndex;
	closeRule(hwnd);

	CInputFilter::CRule& rule = m_inputFilter->getRule(index);
	CInputFilter::CCondition* condition = rule.getCondition()->clone();
	if (editCondition(hwnd, condition)) {
		rule.setCondition(condition);
		fillHotkeys(hwnd, index);
	}
	else {
		delete condition;
	}

	openRule(hwnd);

	// restore selected item in action list
	if (aIndex != LB_ERR) {
		SendMessage(actions, LB_SETCURSEL, aIndex, 0);
	}
}

void
CHotkeyOptions::fillActions(HWND hwnd, UInt32 select)
{
	HWND actions = getItem(hwnd, IDC_HOTKEY_ACTIONS);
	SendMessage(actions, LB_RESETCONTENT, 0, 0);
	if (m_activeRuleIndex != (UInt32)-1) {
		UInt32 n  = m_activeRule.getNumActions(true);
		UInt32 n2 = m_activeRule.getNumActions(false);
		for (UInt32 i = 0; i < n; ++i) {
			const CInputFilter::CAction& action =
				m_activeRule.getAction(true, i);
			CString line("A ");
			line += action.format();
			SendMessage(actions, LB_INSERTSTRING, (WPARAM)-1,
								(LPARAM)line.c_str());
			SendMessage(actions, LB_SETITEMDATA, (WPARAM)i, (LPARAM)i);
		}
		for (UInt32 i = 0; i < n2; ++i) {
			const CInputFilter::CAction& action =
				m_activeRule.getAction(false, i);
			CString line("D ");
			line += action.format();
			SendMessage(actions, LB_INSERTSTRING, (WPARAM)-1,
								(LPARAM)line.c_str());
			SendMessage(actions, LB_SETITEMDATA, (WPARAM)i + n,
								(LPARAM)(i | 0x80000000u));
		}

		if (select < n + n2) {
			SendMessage(actions, LB_SETCURSEL, select, 0);
		}
	}

	updateActionsControls(hwnd);
}

void
CHotkeyOptions::updateActionsControls(HWND hwnd)
{
	HWND child = getItem(hwnd, IDC_HOTKEY_HOTKEYS);
	bool active = (m_activeRuleIndex != (UInt32)-1);

	child = getItem(hwnd, IDC_HOTKEY_ACTIONS);
	bool selected =
		(active && (SendMessage(child, LB_GETCURSEL, 0, 0) != LB_ERR));

	enableItem(hwnd, IDC_HOTKEY_ADD_ACTION, active);
	enableItem(hwnd, IDC_HOTKEY_EDIT_ACTION, selected);
	enableItem(hwnd, IDC_HOTKEY_REMOVE_ACTION, selected);
}

void
CHotkeyOptions::addAction(HWND hwnd)
{
	CInputFilter::CAction* action = NULL;
	bool onActivate = true;
	if (editAction(hwnd, action, onActivate)) {
		m_activeRule.adoptAction(action, onActivate);

		UInt32 actionIndex = m_activeRule.getNumActions(true) - 1;
		if (!onActivate) {
			actionIndex += m_activeRule.getNumActions(false);
		}
		fillActions(hwnd, actionIndex);
	}
	else {
		delete action;
	}
}

void
CHotkeyOptions::removeAction(HWND hwnd)
{
	HWND child = getItem(hwnd, IDC_HOTKEY_ACTIONS);
	LRESULT index = SendMessage(child, LB_GETCURSEL, 0, 0);
	if (index != LB_ERR) {
		UInt32 actionIndex =
			(UInt32)SendMessage(child, LB_GETITEMDATA, index, 0);
		bool onActivate = ((actionIndex & 0x80000000u) == 0);
		actionIndex    &= ~0x80000000u;

		m_activeRule.removeAction(onActivate, actionIndex);

		actionIndex = static_cast<UInt32>(index);
		UInt32 n = m_activeRule.getNumActions(true) +
						m_activeRule.getNumActions(false);
		if (n > 0 && actionIndex >= n) {
			actionIndex = n - 1;
		}
		fillActions(hwnd, actionIndex);
	}
}

void
CHotkeyOptions::editAction(HWND hwnd)
{
	HWND child = getItem(hwnd, IDC_HOTKEY_ACTIONS);
	LRESULT index = SendMessage(child, LB_GETCURSEL, 0, 0);
	if (index != LB_ERR) {
		UInt32 actionIndex =
			(UInt32)SendMessage(child, LB_GETITEMDATA, index, 0);
		bool onActivate = ((actionIndex & 0x80000000u) == 0);
		actionIndex    &= ~0x80000000u;

		CInputFilter::CAction* action =
			m_activeRule.getAction(onActivate, actionIndex).clone();
		bool newOnActivate = onActivate;
		if (editAction(hwnd, action, newOnActivate)) {
			if (onActivate == newOnActivate) {
				m_activeRule.replaceAction(action, onActivate, actionIndex);
				actionIndex = static_cast<UInt32>(index);
			}
			else {
				m_activeRule.removeAction(onActivate, actionIndex);
				m_activeRule.adoptAction(action, newOnActivate);
				actionIndex = m_activeRule.getNumActions(true) - 1;
				if (!newOnActivate) {
					actionIndex += m_activeRule.getNumActions(false);
				}
			}
			fillActions(hwnd, actionIndex);
		}
		else {
			delete action;
		}
	}
}

bool
CHotkeyOptions::editCondition(HWND hwnd, CInputFilter::CCondition*& condition)
{
	return CConditionDialog::doModal(hwnd, condition);
}

bool
CHotkeyOptions::editAction(HWND hwnd, CInputFilter::CAction*& action,
				bool& onActivate)
{
	return CActionDialog::doModal(hwnd, m_config, action, onActivate);
}

void
CHotkeyOptions::openRule(HWND hwnd)
{
	// get the active rule and copy it, merging down/up pairs of keystroke
	// and mouse button actions into single actions for the convenience of
	// of the user.
	HWND rules = getItem(hwnd, IDC_HOTKEY_HOTKEYS);
	LRESULT index = SendMessage(rules, LB_GETCURSEL, 0, 0);
	if (index != LB_ERR) {
		// copy the rule as is
		m_activeRuleIndex = (SInt32)index;
		m_activeRule = m_inputFilter->getRule(m_activeRuleIndex);

		// look for actions to combine
		for (UInt32 i = 0, n = m_activeRule.getNumActions(true); i < n; ++i) {
			// get next activate action
			const CInputFilter::CAction* action =
				&m_activeRule.getAction(true, i);

			// check if it's a key or mouse action
			const CInputFilter::CKeystrokeAction* keyAction =
				dynamic_cast<const CInputFilter::CKeystrokeAction*>(action);
			const CInputFilter::CMouseButtonAction* mouseAction =
				dynamic_cast<const CInputFilter::CMouseButtonAction*>(action);
			if (keyAction == NULL && mouseAction == NULL) {
				continue;
			}

			// check for matching deactivate action
			UInt32 j = (UInt32)-1;
			CInputFilter::CAction* newAction = NULL;
			if (keyAction != NULL) {
				j = findMatchingAction(keyAction);
				if (j != (UInt32)-1) {
					// found a match
					const IPlatformScreen::CKeyInfo* oldInfo =
						keyAction->getInfo();
					IPlatformScreen::CKeyInfo* newInfo =
						IKeyState::CKeyInfo::alloc(*oldInfo);
					newAction = new CKeystrokeDownUpAction(newInfo);
				}
			}
			else if (mouseAction != NULL) {
				j = findMatchingAction(mouseAction);
				if (j != (UInt32)-1) {
					// found a match
					const IPlatformScreen::CButtonInfo* oldInfo =
						mouseAction->getInfo();
					IPlatformScreen::CButtonInfo* newInfo =
						IPrimaryScreen::CButtonInfo::alloc(*oldInfo);
					newAction = new CMouseButtonDownUpAction(newInfo);
				}
			}

			// perform merge
			if (newAction != NULL) {
				m_activeRule.replaceAction(newAction, true, i);
				m_activeRule.removeAction(false, j);
			}
		}
	}
	else {
		m_activeRuleIndex = (UInt32)-1;
	}
	fillActions(hwnd);
}

void
CHotkeyOptions::closeRule(HWND)
{
	// copy rule back to input filter, expanding merged actions into the
	// two component actions.
	if (m_activeRuleIndex != (UInt32)-1) {
		// expand merged rules
		for (UInt32 i = 0, n = m_activeRule.getNumActions(true); i < n; ++i) {
			// get action
			const CInputFilter::CAction* action =
				&m_activeRule.getAction(true, i);

			// check if it's a merged key or mouse action
			const CKeystrokeDownUpAction* keyAction =
				dynamic_cast<const CKeystrokeDownUpAction*>(action);
			const CMouseButtonDownUpAction* mouseAction =
				dynamic_cast<const CMouseButtonDownUpAction*>(action);
			if (keyAction == NULL && mouseAction == NULL) {
				continue;
			}

			// expand
			if (keyAction != NULL) {
				const IPlatformScreen::CKeyInfo* oldInfo =
					keyAction->getInfo();
				IPlatformScreen::CKeyInfo* newInfo =
					IKeyState::CKeyInfo::alloc(*oldInfo);
				CInputFilter::CKeystrokeAction* downAction =
					new CInputFilter::CKeystrokeAction(newInfo, true);
				newInfo = IKeyState::CKeyInfo::alloc(*oldInfo);
				CInputFilter::CKeystrokeAction* upAction =
					new CInputFilter::CKeystrokeAction(newInfo, false);
				m_activeRule.replaceAction(downAction, true, i);
				m_activeRule.adoptAction(upAction, false);
			}
			else if (mouseAction != NULL) {
				const IPlatformScreen::CButtonInfo* oldInfo =
					mouseAction->getInfo();
				IPlatformScreen::CButtonInfo* newInfo =
					IPrimaryScreen::CButtonInfo::alloc(*oldInfo);
				CInputFilter::CMouseButtonAction* downAction =
					new CInputFilter::CMouseButtonAction(newInfo, true);
				newInfo = IPrimaryScreen::CButtonInfo::alloc(*oldInfo);
				CInputFilter::CMouseButtonAction* upAction =
					new CInputFilter::CMouseButtonAction(newInfo, false);
				m_activeRule.replaceAction(downAction, true, i);
				m_activeRule.adoptAction(upAction, false);
			}
		}

		// copy it back
		m_inputFilter->getRule(m_activeRuleIndex) = m_activeRule;
	}
	m_activeRuleIndex = (UInt32)-1;
}

UInt32
CHotkeyOptions::findMatchingAction(
				const CInputFilter::CKeystrokeAction* src) const
{
	for (UInt32 i = 0, n = m_activeRule.getNumActions(false); i < n; ++i) {
		const CInputFilter::CKeystrokeAction* dst =
			dynamic_cast<const CInputFilter::CKeystrokeAction*>(
				&m_activeRule.getAction(false, i));
		if (dst != NULL &&
			IKeyState::CKeyInfo::equal(src->getInfo(), dst->getInfo())) {
			return i;
		}
	}
	return (UInt32)-1;
}

UInt32
CHotkeyOptions::findMatchingAction(
				const CInputFilter::CMouseButtonAction* src) const
{
	for (UInt32 i = 0, n = m_activeRule.getNumActions(false); i < n; ++i) {
		const CInputFilter::CMouseButtonAction* dst =
			dynamic_cast<const CInputFilter::CMouseButtonAction*>(
				&m_activeRule.getAction(false, i));
		if (dst != NULL &&
			IPrimaryScreen::CButtonInfo::equal(
								src->getInfo(), dst->getInfo())) {
			return i;
		}
	}
	return (UInt32)-1;
}

BOOL
CHotkeyOptions::doDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM)
{
	switch (message) {
	case WM_INITDIALOG:
		doInit(hwnd);
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
		case IDCANCEL:
			closeRule(hwnd);
			EndDialog(hwnd, 0);
			return TRUE;

		case IDC_HOTKEY_HOTKEYS:
			switch (HIWORD(wParam)) {
			case LBN_DBLCLK:
				editHotkey(hwnd);
				return TRUE;

			case LBN_SELCHANGE: {
				HWND rules    = getItem(hwnd, IDC_HOTKEY_HOTKEYS);
				LRESULT index = SendMessage(rules, LB_GETCURSEL, 0, 0);
				if (m_activeRuleIndex != (UInt32)index) {
					closeRule(hwnd);
					updateHotkeysControls(hwnd);
					openRule(hwnd);
				}
				return TRUE;
			}
			}
			break;

		case IDC_HOTKEY_ADD_HOTKEY:
			addHotkey(hwnd);
			return TRUE;

		case IDC_HOTKEY_REMOVE_HOTKEY:
			removeHotkey(hwnd);
			return TRUE;

		case IDC_HOTKEY_EDIT_HOTKEY:
			editHotkey(hwnd);
			return TRUE;

		case IDC_HOTKEY_ACTIONS:
			switch (HIWORD(wParam)) {
			case LBN_DBLCLK:
				editAction(hwnd);
				return TRUE;

			case LBN_SELCHANGE:
				updateActionsControls(hwnd);
				return TRUE;
			}
			break;

		case IDC_HOTKEY_ADD_ACTION:
			addAction(hwnd);
			return TRUE;

		case IDC_HOTKEY_REMOVE_ACTION:
			removeAction(hwnd);
			return TRUE;

		case IDC_HOTKEY_EDIT_ACTION:
			editAction(hwnd);
			return TRUE;
		}
		break;

	default:
		break;
	}

	return FALSE;
}

BOOL CALLBACK
CHotkeyOptions::dlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return s_singleton->doDlgProc(hwnd, message, wParam, lParam);
}


//
// CHotkeyOptions::CConditionDialog
//

CInputFilter::CCondition*
				CHotkeyOptions::CConditionDialog::s_condition         = NULL;
CInputFilter::CCondition*
				CHotkeyOptions::CConditionDialog::s_lastGoodCondition = NULL;
WNDPROC			CHotkeyOptions::CConditionDialog::s_editWndProc       = NULL;

bool
CHotkeyOptions::CConditionDialog::doModal(HWND parent,
				CInputFilter::CCondition*& condition)
{
	s_condition = condition;
	if (s_condition != NULL) {
		s_lastGoodCondition = s_condition->clone();
	}
	else {
		s_lastGoodCondition = NULL;
	}
	int n = DialogBox(s_instance, MAKEINTRESOURCE(IDD_HOTKEY_CONDITION),
								parent, dlgProc);

	condition           = s_condition;
	delete s_lastGoodCondition;
	s_condition         = NULL;
	s_lastGoodCondition = NULL;

	return (n == 1);
}

void
CHotkeyOptions::CConditionDialog::doInit(HWND hwnd)
{
	// subclass edit control
	HWND child = getItem(hwnd, IDC_HOTKEY_CONDITION_HOTKEY);
	s_editWndProc = (WNDPROC)GetWindowLong(child, GWL_WNDPROC);
	SetWindowLong(child, GWL_WNDPROC, (LONG)editProc);

	// fill control
	fillHotkey(hwnd);
}

void
CHotkeyOptions::CConditionDialog::fillHotkey(HWND hwnd)
{
	HWND child = getItem(hwnd, IDC_HOTKEY_CONDITION_HOTKEY);
	if (s_condition != NULL) {
		setWindowText(child, s_condition->format().c_str());
	}
	else {
		setWindowText(child, "");
	}
}

void
CHotkeyOptions::CConditionDialog::onButton(HWND hwnd, ButtonID button)
{
	delete s_condition;
	s_condition =
		new CInputFilter::CMouseButtonCondition(button, getModifiers());

	fillHotkey(GetParent(hwnd));
}

void
CHotkeyOptions::CConditionDialog::onKey(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	// ignore key repeats
	if ((lParam & 0xc0000000u) == 0x40000000u) {
		return;
	}

	// ignore key releases if the condition is complete and for the tab
	// key (in case we were just tabbed to)
	if ((lParam & 0x80000000u) != 0) {
		if (isGoodCondition() || wParam == VK_TAB) {
			return;
		}
	}

	KeyID key            = kKeyNone;
	KeyModifierMask mask = getModifiers();
	switch (wParam) {
	case VK_SHIFT:
	case VK_LSHIFT:
	case VK_RSHIFT:
	case VK_CONTROL:
	case VK_LCONTROL:
	case VK_RCONTROL:
	case VK_MENU:
	case VK_LMENU:
	case VK_RMENU:
	case VK_LWIN:
	case VK_RWIN:
		break;

	case VK_TAB:
		// allow tabbing out of control
		if ((mask & (KeyModifierControl |
							KeyModifierAlt | KeyModifierSuper)) == 0) {
			HWND next = hwnd;
			if ((mask & KeyModifierShift) == 0) {
				do {
					next = GetWindow(next, GW_HWNDNEXT);
					if (next == NULL) {
						next = GetWindow(hwnd, GW_HWNDFIRST);
					}
				} while (next != hwnd &&
					(!IsWindowVisible(next) ||
					(GetWindowLong(next, GWL_STYLE) & WS_TABSTOP) == 0));
			}
			else {
				do {
					next = GetWindow(next, GW_HWNDPREV);
					if (next == NULL) {
						next = GetWindow(hwnd, GW_HWNDLAST);
					}
				} while (next != hwnd &&
					(!IsWindowVisible(next) ||
					(GetWindowLong(next, GWL_STYLE) & WS_TABSTOP) == 0));
			}
			SetFocus(next);
			return;
		}
		// fall through

	default:
		key = CMSWindowsKeyState::getKeyID(wParam,
						static_cast<KeyButton>((lParam & 0x1ff0000u) >> 16));
		switch (key) {
		case kKeyNone:
			// could be a character
			key = getChar(wParam, lParam);
			if (key == kKeyNone) {
				return;
			}
			break;

		case kKeyShift_L:
		case kKeyShift_R:
		case kKeyControl_L:
		case kKeyControl_R:
		case kKeyAlt_L:
		case kKeyAlt_R:
		case kKeyMeta_L:
		case kKeyMeta_R:
		case kKeySuper_L:
		case kKeySuper_R:
		case kKeyCapsLock:
		case kKeyNumLock:
		case kKeyScrollLock:
			// bogus
			return;
		}
		break;
	}

	delete s_condition;
	s_condition = new CInputFilter::CKeystrokeCondition(key, mask);

	fillHotkey(GetParent(hwnd));
}

KeyID
CHotkeyOptions::CConditionDialog::getChar(WPARAM wParam, LPARAM lParam)
{
	BYTE keyState[256];
	UINT virtualKey = (UINT)wParam;
	UINT scanCode   = (UINT)((lParam & 0x0ff0000u) >> 16);
	GetKeyboardState(keyState);

	// reset modifier state
	keyState[VK_SHIFT]    = 0;
	keyState[VK_LSHIFT]   = 0;
	keyState[VK_RSHIFT]   = 0;
	keyState[VK_CONTROL]  = 0;
	keyState[VK_LCONTROL] = 0;
	keyState[VK_RCONTROL] = 0;
	keyState[VK_MENU]     = 0;
	keyState[VK_LMENU]    = 0;
	keyState[VK_RMENU]    = 0;
	keyState[VK_LWIN]     = 0;
	keyState[VK_RWIN]     = 0;

	// translate virtual key to character
	int n;
	KeyID id;
	if (CArchMiscWindows::isWindows95Family()) {
		// XXX -- how do we get characters not in Latin-1?
		WORD ascii;
		n  = ToAscii(virtualKey, scanCode, keyState, &ascii, 0);
		id = static_cast<KeyID>(ascii & 0xffu);
	}
	else {
		typedef int (WINAPI *ToUnicode_t)(UINT wVirtKey,
											UINT wScanCode,
											PBYTE lpKeyState,
											LPWSTR pwszBuff,
											int cchBuff,
											UINT wFlags);
		ToUnicode_t s_ToUnicode = NULL;
		if (s_ToUnicode == NULL) {
			HMODULE userModule = GetModuleHandle("user32.dll");
			s_ToUnicode =
				(ToUnicode_t)GetProcAddress(userModule, "ToUnicode");
		}

		WCHAR unicode[2];
		n  = s_ToUnicode(virtualKey, scanCode, keyState,
								unicode, sizeof(unicode) / sizeof(unicode[0]),
								0);
		id = static_cast<KeyID>(unicode[0]);
	}
	switch (n) {
	case -1:
		// no hot keys on dead keys
		return kKeyNone;

	default:
	case 0:
		// unmapped
		return kKeyNone;

	case 1:
		return id;
	}
}

KeyModifierMask
CHotkeyOptions::CConditionDialog::getModifiers()
{
	KeyModifierMask mask = 0;
	if ((GetKeyState(VK_SHIFT) & 0x8000) != 0) {
		mask |= KeyModifierShift;
	}
	if ((GetKeyState(VK_CONTROL) & 0x8000) != 0) {
		mask |= KeyModifierControl;
	}
	if ((GetKeyState(VK_MENU) & 0x8000) != 0) {
		mask |= KeyModifierAlt;
	}
	if ((GetKeyState(VK_LWIN) & 0x8000) != 0 ||
		(GetKeyState(VK_RWIN) & 0x8000) != 0) {
		mask |= KeyModifierSuper;
	}
	return mask;
}

bool
CHotkeyOptions::CConditionDialog::isGoodCondition()
{
	CInputFilter::CKeystrokeCondition* keyCondition =
		dynamic_cast<CInputFilter::CKeystrokeCondition*>(s_condition);
	return (keyCondition == NULL || keyCondition->getKey() != kKeyNone);
}

BOOL CALLBACK
CHotkeyOptions::CConditionDialog::dlgProc(HWND hwnd,
				UINT message, WPARAM wParam, LPARAM)
{
	switch (message) {
	case WM_INITDIALOG:
		doInit(hwnd);
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			EndDialog(hwnd, 1);
			return TRUE;

		case IDCANCEL:
			EndDialog(hwnd, 0);
			return TRUE;
		}
		break;

	default:
		break;
	}

	return FALSE;
}

LRESULT CALLBACK
CHotkeyOptions::CConditionDialog::editProc(HWND hwnd,
				UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_LBUTTONDOWN:
		if (GetFocus() == hwnd) {
			onButton(hwnd, kButtonLeft);
		}
		else {
			SetFocus(hwnd);
		}
		return 0;

	case WM_MBUTTONDOWN:
		if (GetFocus() == hwnd) {
			onButton(hwnd, kButtonMiddle);
		}
		return 0;

	case WM_RBUTTONDOWN:
		if (GetFocus() == hwnd) {
			onButton(hwnd, kButtonRight);
		}
		return 0;

	case WM_XBUTTONDOWN:
		if (GetFocus() == hwnd) {
			switch (HIWORD(wParam)) {
			case XBUTTON1:
				onButton(hwnd, kButtonExtra0 + 0);
				break;

			case XBUTTON2:
				onButton(hwnd, kButtonExtra0 + 1);
				break;
			}
		}
		return 0;

	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYUP:
		onKey(hwnd, wParam, lParam);
		return 0;

	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	case WM_XBUTTONUP:
	case WM_CHAR:
	case WM_SYSCHAR:
	case WM_DEADCHAR:
	case WM_SYSDEADCHAR:
		return 0;

	case WM_SETFOCUS:
		if (s_condition != NULL) {
			delete s_lastGoodCondition;
			s_lastGoodCondition = s_condition->clone();
		}
		break;

	case WM_KILLFOCUS:
		if (!isGoodCondition()) {
			delete s_condition;
			if (s_lastGoodCondition != NULL) {
				s_condition = s_lastGoodCondition->clone();
			}
			else {
				s_condition = NULL;
			}
		}
		fillHotkey(GetParent(hwnd));
		break;

	case WM_GETDLGCODE:
		return DLGC_WANTALLKEYS;

	default:
		break;
	}
	return CallWindowProc(s_editWndProc, hwnd, message, wParam, lParam);
}


//
// CHotkeyOptions::CActionDialog
//

CConfig*		CHotkeyOptions::CActionDialog::s_config         = NULL;
bool			CHotkeyOptions::CActionDialog::s_onActivate     = false;
CInputFilter::CAction*
				CHotkeyOptions::CActionDialog::s_action         = NULL;
CInputFilter::CAction*
				CHotkeyOptions::CActionDialog::s_lastGoodAction = NULL;
WNDPROC			CHotkeyOptions::CActionDialog::s_editWndProc    = NULL;

bool
CHotkeyOptions::CActionDialog::doModal(HWND parent, CConfig* config,
				CInputFilter::CAction*& action, bool& onActivate)
{
	s_config     = config;
	s_onActivate = onActivate;
	s_action     = action;
	if (s_action != NULL) {
		s_lastGoodAction = s_action->clone();
	}
	else {
		s_lastGoodAction = NULL;
	}

	int n = DialogBox(s_instance, MAKEINTRESOURCE(IDD_HOTKEY_ACTION),
								parent, dlgProc);

	onActivate       = s_onActivate;
	action           = s_action;
	delete s_lastGoodAction;
	s_action         = NULL;
	s_lastGoodAction = NULL;

	return (n == 1);
}

void
CHotkeyOptions::CActionDialog::doInit(HWND hwnd)
{
	// subclass edit control
	HWND child = getItem(hwnd, IDC_HOTKEY_ACTION_HOTKEY);
	s_editWndProc = (WNDPROC)GetWindowLong(child, GWL_WNDPROC);
	SetWindowLong(child, GWL_WNDPROC, (LONG)editProc);
	setWindowText(getItem(hwnd, IDC_HOTKEY_ACTION_HOTKEY), "");
	fillHotkey(hwnd);

	// fill screens
	child = getItem(hwnd, IDC_HOTKEY_ACTION_SWITCH_TO_LIST);
	SendMessage(child, CB_RESETCONTENT, 0, 0);
	for (CConfig::const_iterator index = s_config->begin();
							index != s_config->end(); ) {
		const CString& name = *index;
		++index;
		if (index != s_config->end()) {
			SendMessage(child, CB_INSERTSTRING,
							(WPARAM)-1, (LPARAM)name.c_str());
		}
		else {
			SendMessage(child, CB_ADDSTRING, 0, (LPARAM)name.c_str());
		}
	}
	SendMessage(child, CB_SETCURSEL, 0, 0);

	// fill directions
	child = getItem(hwnd, IDC_HOTKEY_ACTION_SWITCH_IN_LIST);
	SendMessage(child, CB_ADDSTRING, 0,
							(LPARAM)getString(IDS_EDGE_LEFT).c_str());
	SendMessage(child, CB_ADDSTRING, 0,
							(LPARAM)getString(IDS_EDGE_RIGHT).c_str());
	SendMessage(child, CB_ADDSTRING, 0,
							(LPARAM)getString(IDS_EDGE_TOP).c_str());
	SendMessage(child, CB_ADDSTRING, 0,
							(LPARAM)getString(IDS_EDGE_BOTTOM).c_str());
	SendMessage(child, CB_SETCURSEL, 0, 0);

	// fill lock modes
	child = getItem(hwnd, IDC_HOTKEY_ACTION_LOCK_LIST);
	SendMessage(child, CB_ADDSTRING, 0,
							(LPARAM)getString(IDS_LOCK_MODE_OFF).c_str());
	SendMessage(child, CB_ADDSTRING, 0,
							(LPARAM)getString(IDS_LOCK_MODE_ON).c_str());
	SendMessage(child, CB_ADDSTRING, 0,
							(LPARAM)getString(IDS_LOCK_MODE_TOGGLE).c_str());
	SendMessage(child, CB_SETCURSEL, 0, 0);

	// select when
	if (s_onActivate) {
		child = getItem(hwnd, IDC_HOTKEY_ACTION_ON_ACTIVATE);
	}
	else {
		child = getItem(hwnd, IDC_HOTKEY_ACTION_ON_DEACTIVATE);
	}
	setItemChecked(child, true);

	// select mode
	child = NULL;
	CInputFilter::CKeystrokeAction* keyAction =
		dynamic_cast<CInputFilter::CKeystrokeAction*>(s_action);
	CInputFilter::CMouseButtonAction* mouseAction =
		dynamic_cast<CInputFilter::CMouseButtonAction*>(s_action);
	CInputFilter::CLockCursorToScreenAction* lockAction =
		dynamic_cast<CInputFilter::CLockCursorToScreenAction*>(s_action);
	CInputFilter::CSwitchToScreenAction* switchToAction =
		dynamic_cast<CInputFilter::CSwitchToScreenAction*>(s_action);
	CInputFilter::CSwitchInDirectionAction* switchInAction =
		dynamic_cast<CInputFilter::CSwitchInDirectionAction*>(s_action);
	if (keyAction != NULL) {
		if (dynamic_cast<CKeystrokeDownUpAction*>(s_action) != NULL) {
			child = getItem(hwnd, IDC_HOTKEY_ACTION_DOWNUP);
		}
		else if (keyAction->isOnPress()) {
			child = getItem(hwnd, IDC_HOTKEY_ACTION_DOWN);
		}
		else {
			child = getItem(hwnd, IDC_HOTKEY_ACTION_UP);
		}
	}
	else if (mouseAction != NULL) {
		if (dynamic_cast<CMouseButtonDownUpAction*>(s_action) != NULL) {
			child = getItem(hwnd, IDC_HOTKEY_ACTION_DOWNUP);
		}
		else if (keyAction->isOnPress()) {
			child = getItem(hwnd, IDC_HOTKEY_ACTION_DOWN);
		}
		else {
			child = getItem(hwnd, IDC_HOTKEY_ACTION_UP);
		}
	}
	else if (lockAction != NULL) {
		child = getItem(hwnd, IDC_HOTKEY_ACTION_LOCK_LIST);
		SendMessage(child, CB_SETCURSEL, lockAction->getMode(), 0);
		child = getItem(hwnd, IDC_HOTKEY_ACTION_LOCK);
	}
	else if (switchToAction != NULL) {
		child = getItem(hwnd, IDC_HOTKEY_ACTION_SWITCH_TO_LIST);
		DWORD i = SendMessage(child, CB_FINDSTRINGEXACT, (WPARAM)-1,
								(LPARAM)switchToAction->getScreen().c_str());
		if (i == CB_ERR) {
			i = 0;
		}
		SendMessage(child, CB_SETCURSEL, i, 0);
		child = getItem(hwnd, IDC_HOTKEY_ACTION_SWITCH_TO);
	}
	else if (switchInAction != NULL) {
		child = getItem(hwnd, IDC_HOTKEY_ACTION_SWITCH_IN_LIST);
		SendMessage(child, CB_SETCURSEL,
							switchInAction->getDirection() - kLeft, 0);
		child = getItem(hwnd, IDC_HOTKEY_ACTION_SWITCH_IN);
	}
	if (child != NULL) {
		setItemChecked(child, true);
	}

	updateControls(hwnd);
}

void
CHotkeyOptions::CActionDialog::fillHotkey(HWND hwnd)
{
	HWND child = getItem(hwnd, IDC_HOTKEY_ACTION_HOTKEY);
	CInputFilter::CKeystrokeAction* keyAction =
		dynamic_cast<CInputFilter::CKeystrokeAction*>(s_action);
	CInputFilter::CMouseButtonAction* mouseAction =
		dynamic_cast<CInputFilter::CMouseButtonAction*>(s_action);
	if (keyAction != NULL || mouseAction != NULL) {
		setWindowText(child, s_action->format().c_str());
	}
	else {
		setWindowText(child, "");
	}

	// can only set screens in key actions
	enableItem(hwnd, IDC_HOTKEY_ACTION_SCREENS, keyAction != NULL);
}

void
CHotkeyOptions::CActionDialog::updateControls(HWND hwnd)
{
	// determine which mode we're in
	UInt32 mode = 0;
	if (isItemChecked(getItem(hwnd, IDC_HOTKEY_ACTION_DOWNUP)) ||
		isItemChecked(getItem(hwnd, IDC_HOTKEY_ACTION_DOWN)) ||
		isItemChecked(getItem(hwnd, IDC_HOTKEY_ACTION_UP))) {
		mode = 1;
	}
	else if (isItemChecked(getItem(hwnd, IDC_HOTKEY_ACTION_SWITCH_TO))) {
		mode = 2;
	}
	else if (isItemChecked(getItem(hwnd, IDC_HOTKEY_ACTION_SWITCH_IN))) {
		mode = 3;
	}
	else if (isItemChecked(getItem(hwnd, IDC_HOTKEY_ACTION_LOCK))) {
		mode = 4;
	}

	// enable/disable all mode specific controls
	enableItem(hwnd, IDC_HOTKEY_ACTION_HOTKEY, mode == 1);
	enableItem(hwnd, IDC_HOTKEY_ACTION_SWITCH_TO_LIST, mode == 2);
	enableItem(hwnd, IDC_HOTKEY_ACTION_SWITCH_IN_LIST, mode == 3);
	enableItem(hwnd, IDC_HOTKEY_ACTION_LOCK_LIST, mode == 4);

	// can only set screens in key actions
	CInputFilter::CKeystrokeAction* keyAction =
		dynamic_cast<CInputFilter::CKeystrokeAction*>(s_action);
	enableItem(hwnd, IDC_HOTKEY_ACTION_SCREENS, keyAction != NULL);
}

void
CHotkeyOptions::CActionDialog::onButton(HWND hwnd, ButtonID button)
{
	IPlatformScreen::CButtonInfo* info =
		IPrimaryScreen::CButtonInfo::alloc(button, getModifiers());
	delete s_action;
	HWND parent = GetParent(hwnd);
	if (isItemChecked(getItem(parent, IDC_HOTKEY_ACTION_DOWNUP))) {
		s_action = new CMouseButtonDownUpAction(info);
	}
	else if (isItemChecked(getItem(parent, IDC_HOTKEY_ACTION_DOWN))) {
		s_action = new CInputFilter::CMouseButtonAction(info, true);
	}
	else if (isItemChecked(getItem(parent, IDC_HOTKEY_ACTION_UP))) {
		s_action = new CInputFilter::CMouseButtonAction(info, false);
	}
	else {
		s_action = NULL;
	}

	fillHotkey(parent);
}

void
CHotkeyOptions::CActionDialog::onKey(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	// ignore key repeats
	if ((lParam & 0xc0000000u) == 0x40000000u) {
		return;
	}

	// ignore key releases if the action is complete and for the tab
	// key (in case we were just tabbed to)
	if ((lParam & 0x80000000u) != 0) {
		if (isGoodAction() || wParam == VK_TAB) {
			return;
		}
	}

	KeyID key            = kKeyNone;
	KeyModifierMask mask = getModifiers();
	switch (wParam) {
	case VK_SHIFT:
	case VK_LSHIFT:
	case VK_RSHIFT:
	case VK_CONTROL:
	case VK_LCONTROL:
	case VK_RCONTROL:
	case VK_MENU:
	case VK_LMENU:
	case VK_RMENU:
	case VK_LWIN:
	case VK_RWIN:
		break;

	case VK_TAB:
		// allow tabbing out of control
		if ((mask & (KeyModifierControl |
							KeyModifierAlt | KeyModifierSuper)) == 0) {
			HWND next = hwnd;
			if ((mask & KeyModifierShift) == 0) {
				do {
					next = GetWindow(next, GW_HWNDNEXT);
					if (next == NULL) {
						next = GetWindow(hwnd, GW_HWNDFIRST);
					}
				} while (next != hwnd &&
					(!IsWindowVisible(next) ||
					(GetWindowLong(next, GWL_STYLE) & WS_TABSTOP) == 0));
			}
			else {
				do {
					next = GetWindow(next, GW_HWNDPREV);
					if (next == NULL) {
						next = GetWindow(hwnd, GW_HWNDLAST);
					}
				} while (next != hwnd &&
					(!IsWindowVisible(next) ||
					(GetWindowLong(next, GWL_STYLE) & WS_TABSTOP) == 0));
			}
			SetFocus(next);
			return;
		}
		// fall through

	default:
		key = CMSWindowsKeyState::getKeyID(wParam,
						static_cast<KeyButton>((lParam & 0x1ff0000u) >> 16));
		switch (key) {
		case kKeyNone:
			// could be a character
			key = getChar(wParam, lParam);
			if (key == kKeyNone) {
				return;
			}
			break;

		case kKeyShift_L:
		case kKeyShift_R:
		case kKeyControl_L:
		case kKeyControl_R:
		case kKeyAlt_L:
		case kKeyAlt_R:
		case kKeyMeta_L:
		case kKeyMeta_R:
		case kKeySuper_L:
		case kKeySuper_R:
		case kKeyCapsLock:
		case kKeyNumLock:
		case kKeyScrollLock:
			// bogus
			return;
		}
		break;
	}

	// get old screen list
	std::set<CString> screens;
	CInputFilter::CKeystrokeAction* keyAction =
		dynamic_cast<CInputFilter::CKeystrokeAction*>(s_action);
	if (keyAction == NULL) {
		keyAction =
			dynamic_cast<CInputFilter::CKeystrokeAction*>(s_lastGoodAction);
	}
	if (keyAction != NULL) {
		IKeyState::CKeyInfo::split(keyAction->getInfo()->m_screens, screens);
	}

	// create new action
	IPlatformScreen::CKeyInfo* info =
		IKeyState::CKeyInfo::alloc(key, mask, 0, 0, screens);
	delete s_action;
	HWND parent = GetParent(hwnd);
	if (isItemChecked(getItem(parent, IDC_HOTKEY_ACTION_DOWNUP))) {
		s_action = new CKeystrokeDownUpAction(info);
	}
	else if (isItemChecked(getItem(parent, IDC_HOTKEY_ACTION_DOWN))) {
		s_action = new CInputFilter::CKeystrokeAction(info, true);
	}
	else if (isItemChecked(getItem(parent, IDC_HOTKEY_ACTION_UP))) {
		s_action = new CInputFilter::CKeystrokeAction(info, false);
	}
	else {
		s_action = NULL;
	}

	fillHotkey(parent);
}

void
CHotkeyOptions::CActionDialog::onLockAction(HWND hwnd)
{
	HWND child = getItem(hwnd, IDC_HOTKEY_ACTION_LOCK_LIST);
	LRESULT index = SendMessage(child, CB_GETCURSEL, 0, 0);
	if (index != CB_ERR) {
		delete s_action;
		s_action = new CInputFilter::CLockCursorToScreenAction(
			(CInputFilter::CLockCursorToScreenAction::Mode)index);
	}
}

void
CHotkeyOptions::CActionDialog::onSwitchToAction(HWND hwnd)
{
	HWND child = getItem(hwnd, IDC_HOTKEY_ACTION_SWITCH_TO_LIST);
	CString screen = getWindowText(child);
	delete s_action;
	s_action = new CInputFilter::CSwitchToScreenAction(screen);
}

void
CHotkeyOptions::CActionDialog::onSwitchInAction(HWND hwnd)
{
	HWND child = getItem(hwnd, IDC_HOTKEY_ACTION_SWITCH_IN_LIST);
	LRESULT index = SendMessage(child, CB_GETCURSEL, 0, 0);
	if (index != CB_ERR) {
		delete s_action;
		s_action = new CInputFilter::CSwitchInDirectionAction(
							(EDirection)(index + kLeft));
	}
}

KeyID
CHotkeyOptions::CActionDialog::getChar(WPARAM wParam, LPARAM lParam)
{
	BYTE keyState[256];
	UINT virtualKey = (UINT)wParam;
	UINT scanCode   = (UINT)((lParam & 0x0ff0000u) >> 16);
	GetKeyboardState(keyState);

	// reset modifier state
	keyState[VK_SHIFT]    = 0;
	keyState[VK_LSHIFT]   = 0;
	keyState[VK_RSHIFT]   = 0;
	keyState[VK_CONTROL]  = 0;
	keyState[VK_LCONTROL] = 0;
	keyState[VK_RCONTROL] = 0;
	keyState[VK_MENU]     = 0;
	keyState[VK_LMENU]    = 0;
	keyState[VK_RMENU]    = 0;
	keyState[VK_LWIN]     = 0;
	keyState[VK_RWIN]     = 0;

	// translate virtual key to character
	int n;
	KeyID id;
	if (CArchMiscWindows::isWindows95Family()) {
		// XXX -- how do we get characters not in Latin-1?
		WORD ascii;
		n  = ToAscii(virtualKey, scanCode, keyState, &ascii, 0);
		id = static_cast<KeyID>(ascii & 0xffu);
	}
	else {
		typedef int (WINAPI *ToUnicode_t)(UINT wVirtKey,
											UINT wScanCode,
											PBYTE lpKeyState,
											LPWSTR pwszBuff,
											int cchBuff,
											UINT wFlags);
		ToUnicode_t s_ToUnicode = NULL;
		if (s_ToUnicode == NULL) {
			HMODULE userModule = GetModuleHandle("user32.dll");
			s_ToUnicode =
				(ToUnicode_t)GetProcAddress(userModule, "ToUnicode");
		}

		WCHAR unicode[2];
		n  = s_ToUnicode(virtualKey, scanCode, keyState,
								unicode, sizeof(unicode) / sizeof(unicode[0]),
								0);
		id = static_cast<KeyID>(unicode[0]);
	}
	switch (n) {
	case -1:
		// no hot keys on dead keys
		return kKeyNone;

	default:
	case 0:
		// unmapped
		return kKeyNone;

	case 1:
		return id;
	}
}

KeyModifierMask
CHotkeyOptions::CActionDialog::getModifiers()
{
	KeyModifierMask mask = 0;
	if ((GetKeyState(VK_SHIFT) & 0x8000) != 0) {
		mask |= KeyModifierShift;
	}
	if ((GetKeyState(VK_CONTROL) & 0x8000) != 0) {
		mask |= KeyModifierControl;
	}
	if ((GetKeyState(VK_MENU) & 0x8000) != 0) {
		mask |= KeyModifierAlt;
	}
	if ((GetKeyState(VK_LWIN) & 0x8000) != 0 ||
		(GetKeyState(VK_RWIN) & 0x8000) != 0) {
		mask |= KeyModifierSuper;
	}
	return mask;
}

bool
CHotkeyOptions::CActionDialog::isGoodAction()
{
	CInputFilter::CMouseButtonAction* mouseAction =
		dynamic_cast<CInputFilter::CMouseButtonAction*>(s_action);
	CInputFilter::CKeystrokeAction* keyAction =
		dynamic_cast<CInputFilter::CKeystrokeAction*>(s_action);
	return (mouseAction == NULL || keyAction == NULL ||
			keyAction->getInfo()->m_key != kKeyNone);
}

void
CHotkeyOptions::CActionDialog::convertAction(HWND hwnd)
{
	if (s_lastGoodAction != NULL) {
		CInputFilter::CMouseButtonAction* mouseAction =
			dynamic_cast<CInputFilter::CMouseButtonAction*>(s_lastGoodAction);
		CInputFilter::CKeystrokeAction* keyAction =
			dynamic_cast<CInputFilter::CKeystrokeAction*>(s_lastGoodAction);
		if (mouseAction != NULL) {
			IPlatformScreen::CButtonInfo* info =
				IPrimaryScreen::CButtonInfo::alloc(*mouseAction->getInfo());
			delete s_action;
			if (isItemChecked(getItem(hwnd, IDC_HOTKEY_ACTION_DOWNUP))) {
				s_action = new CMouseButtonDownUpAction(info);
			}
			else if (isItemChecked(getItem(hwnd, IDC_HOTKEY_ACTION_DOWN))) {
				s_action = new CInputFilter::CMouseButtonAction(info, true);
			}
			else if (isItemChecked(getItem(hwnd, IDC_HOTKEY_ACTION_UP))) {
				s_action = new CInputFilter::CMouseButtonAction(info, false);
			}
			else {
				free(info);
				s_action = NULL;
			}
		}
		else if (keyAction != NULL) {
			IPlatformScreen::CKeyInfo* info =
				IKeyState::CKeyInfo::alloc(*keyAction->getInfo());
			delete s_action;
			if (isItemChecked(getItem(hwnd, IDC_HOTKEY_ACTION_DOWNUP))) {
				s_action = new CKeystrokeDownUpAction(info);
			}
			else if (isItemChecked(getItem(hwnd, IDC_HOTKEY_ACTION_DOWN))) {
				s_action = new CInputFilter::CKeystrokeAction(info, true);
			}
			else if (isItemChecked(getItem(hwnd, IDC_HOTKEY_ACTION_UP))) {
				s_action = new CInputFilter::CKeystrokeAction(info, false);
			}
			else {
				free(info);
				s_action = NULL;
			}
		}
	}
}

bool
CHotkeyOptions::CActionDialog::isDownUpAction()
{
	return (dynamic_cast<CKeystrokeDownUpAction*>(s_action) != NULL ||
			dynamic_cast<CMouseButtonDownUpAction*>(s_action) != NULL);
}

BOOL CALLBACK
CHotkeyOptions::CActionDialog::dlgProc(HWND hwnd,
				UINT message, WPARAM wParam, LPARAM)
{
	switch (message) {
	case WM_INITDIALOG:
		doInit(hwnd);
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			if (isDownUpAction()) {
				s_onActivate = true;
			}
			EndDialog(hwnd, 1);
			return TRUE;

		case IDCANCEL:
			EndDialog(hwnd, 0);
			return TRUE;

		case IDC_HOTKEY_ACTION_ON_ACTIVATE:
			s_onActivate = true;
			return TRUE;

		case IDC_HOTKEY_ACTION_ON_DEACTIVATE:
			s_onActivate = false;
			return TRUE;

		case IDC_HOTKEY_ACTION_DOWNUP:
		case IDC_HOTKEY_ACTION_DOWN:
		case IDC_HOTKEY_ACTION_UP:
			convertAction(hwnd);
			fillHotkey(hwnd);
			updateControls(hwnd);
			return TRUE;

		case IDC_HOTKEY_ACTION_LOCK:
			onLockAction(hwnd);
			updateControls(hwnd);
			return TRUE;

		case IDC_HOTKEY_ACTION_SWITCH_TO:
			onSwitchToAction(hwnd);
			updateControls(hwnd);
			return TRUE;

		case IDC_HOTKEY_ACTION_SWITCH_IN:
			onSwitchInAction(hwnd);
			updateControls(hwnd);
			return TRUE;

		case IDC_HOTKEY_ACTION_LOCK_LIST:
			switch (HIWORD(wParam)) {
			case LBN_SELCHANGE:
				onLockAction(hwnd);
				return TRUE;
			}
			break;

		case IDC_HOTKEY_ACTION_SWITCH_TO_LIST:
			switch (HIWORD(wParam)) {
			case LBN_SELCHANGE:
				onSwitchToAction(hwnd);
				return TRUE;
			}
			break;

		case IDC_HOTKEY_ACTION_SWITCH_IN_LIST:
			switch (HIWORD(wParam)) {
			case LBN_SELCHANGE:
				onSwitchInAction(hwnd);
				return TRUE;
			}
			break;

		case IDC_HOTKEY_ACTION_SCREENS:
			CScreensDialog::doModal(hwnd, s_config,
				dynamic_cast<CInputFilter::CKeystrokeAction*>(s_action));
			fillHotkey(hwnd);
			return TRUE;
		}
		break;

	default:
		break;
	}

	return FALSE;
}

LRESULT CALLBACK
CHotkeyOptions::CActionDialog::editProc(HWND hwnd,
				UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_LBUTTONDOWN:
		if (GetFocus() == hwnd) {
			onButton(hwnd, kButtonLeft);
		}
		else {
			SetFocus(hwnd);
		}
		return 0;

	case WM_MBUTTONDOWN:
		if (GetFocus() == hwnd) {
			onButton(hwnd, kButtonMiddle);
		}
		return 0;

	case WM_RBUTTONDOWN:
		if (GetFocus() == hwnd) {
			onButton(hwnd, kButtonRight);
		}
		return 0;

	case WM_XBUTTONDOWN:
		if (GetFocus() == hwnd) {
			switch (HIWORD(wParam)) {
			case XBUTTON1:
				onButton(hwnd, kButtonExtra0 + 0);
				break;

			case XBUTTON2:
				onButton(hwnd, kButtonExtra0 + 1);
				break;
			}
		}
		return 0;

	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYUP:
		onKey(hwnd, wParam, lParam);
		return 0;

	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	case WM_XBUTTONUP:
	case WM_CHAR:
	case WM_SYSCHAR:
	case WM_DEADCHAR:
	case WM_SYSDEADCHAR:
		return 0;

	case WM_SETFOCUS:
		if (s_action != NULL) {
			delete s_lastGoodAction;
			s_lastGoodAction = s_action->clone();
		}
		break;

	case WM_KILLFOCUS:
		if (!isGoodAction()) {
			delete s_action;
			if (s_lastGoodAction != NULL) {
				s_action = s_lastGoodAction->clone();
			}
			else {
				s_action = NULL;
			}
		}
		else if (s_action != NULL) {
			delete s_lastGoodAction;
			s_lastGoodAction = s_action->clone();
		}
		fillHotkey(GetParent(hwnd));
		break;

	case WM_GETDLGCODE:
		return DLGC_WANTALLKEYS;

	default:
		break;
	}
	return CallWindowProc(s_editWndProc, hwnd, message, wParam, lParam);
}


//
// CHotkeyOptions::CScreensDialog
//

CConfig*				CHotkeyOptions::CScreensDialog::s_config = NULL;
CInputFilter::CKeystrokeAction*
						CHotkeyOptions::CScreensDialog::s_action = NULL;
CHotkeyOptions::CScreensDialog::CScreens
						CHotkeyOptions::CScreensDialog::s_nonTargets;
CHotkeyOptions::CScreensDialog::CScreens
						CHotkeyOptions::CScreensDialog::s_targets;
CString				CHotkeyOptions::CScreensDialog::s_allScreens;

void
CHotkeyOptions::CScreensDialog::doModal(HWND parent, CConfig* config,
				CInputFilter::CKeystrokeAction* action)
{
	s_allScreens = getString(IDS_ALL_SCREENS);
	s_config = config;
	s_action = action;
	DialogBox(s_instance, MAKEINTRESOURCE(IDD_HOTKEY_SCREENS),
								parent, dlgProc);
	s_config = NULL;
	s_action = NULL;
}

void
CHotkeyOptions::CScreensDialog::doInit(HWND hwnd)
{
	s_nonTargets.clear();
	s_targets.clear();

	// get screens from config
	s_nonTargets.insert("*");
	for (CConfig::const_iterator i = s_config->begin();
							i != s_config->end(); ++i) {
		s_nonTargets.insert(*i);
	}

	// get screens in action
	IKeyState::CKeyInfo::split(s_action->getInfo()->m_screens, s_targets);

	// remove screens in action from screens in config
	for (CScreens::const_iterator i = s_targets.begin();
								i != s_targets.end(); ++i) {
		s_nonTargets.erase(*i);
	}

	// fill dialog
	fillScreens(hwnd);
	updateControls(hwnd);
}

void
CHotkeyOptions::CScreensDialog::doFini(HWND)
{
	// put screens into action
	const IPlatformScreen::CKeyInfo* oldInfo = s_action->getInfo();
	IPlatformScreen::CKeyInfo* newInfo =
		IKeyState::CKeyInfo::alloc(oldInfo->m_key,
								oldInfo->m_mask, 0, 0, s_targets);
	s_action->adoptInfo(newInfo);
}

void
CHotkeyOptions::CScreensDialog::fillScreens(HWND hwnd)
{
	HWND child = getItem(hwnd, IDC_HOTKEY_SCREENS_SRC);
	SendMessage(child, LB_RESETCONTENT, 0, 0);
	for (CScreens::const_iterator i = s_nonTargets.begin();
								i != s_nonTargets.end(); ++i) {
		CString name = *i;
		if (name == "*") {
			name = s_allScreens;
		}
		SendMessage(child, LB_INSERTSTRING, (WPARAM)-1,
							(LPARAM)name.c_str());
	}

	child = getItem(hwnd, IDC_HOTKEY_SCREENS_DST);
	SendMessage(child, LB_RESETCONTENT, 0, 0);
	for (CScreens::const_iterator i = s_targets.begin();
								i != s_targets.end(); ++i) {
		CString name = *i;
		if (name == "*") {
			name = s_allScreens;
		}
		SendMessage(child, LB_INSERTSTRING, (WPARAM)-1,
							(LPARAM)name.c_str());
	}
	if (s_targets.empty()) {
		// if no targets then add a special item so the user knows
		// what'll happen
		CString activeScreenLabel = getString(IDS_ACTIVE_SCREEN);
		SendMessage(child, LB_INSERTSTRING, (WPARAM)-1,
							(LPARAM)activeScreenLabel.c_str());
	}
}

void
CHotkeyOptions::CScreensDialog::updateControls(HWND hwnd)
{
	HWND child     = getItem(hwnd, IDC_HOTKEY_SCREENS_SRC);
	bool canAdd    = (SendMessage(child, LB_GETSELCOUNT, 0, 0) != 0);
	child          = getItem(hwnd, IDC_HOTKEY_SCREENS_DST);
	bool canRemove = (!s_targets.empty() &&
						(SendMessage(child, LB_GETSELCOUNT, 0, 0) != 0));

	enableItem(hwnd, IDC_HOTKEY_SCREENS_ADD, canAdd);
	enableItem(hwnd, IDC_HOTKEY_SCREENS_REMOVE, canRemove);
}

void
CHotkeyOptions::CScreensDialog::add(HWND hwnd)
{
	CScreens selected;
	getSelected(hwnd, IDC_HOTKEY_SCREENS_SRC, s_nonTargets, selected);
	for (CScreens::const_iterator i = selected.begin();
								i != selected.end(); ++i) {
		s_targets.insert(*i);
		s_nonTargets.erase(*i);
	}
	fillScreens(hwnd);
	updateControls(hwnd);
}

void
CHotkeyOptions::CScreensDialog::remove(HWND hwnd)
{
	CScreens selected;
	getSelected(hwnd, IDC_HOTKEY_SCREENS_DST, s_targets, selected);
	for (CScreens::const_iterator i = selected.begin();
								i != selected.end(); ++i) {
		s_nonTargets.insert(*i);
		s_targets.erase(*i);
	}
	fillScreens(hwnd);
	updateControls(hwnd);
}

void
CHotkeyOptions::CScreensDialog::getSelected(HWND hwnd, UINT id,
				const CScreens& inScreens, CScreens& outScreens)
{
	// get the selected item indices
	HWND child = getItem(hwnd, id);
	UInt32 n   = (UInt32)SendMessage(child, LB_GETSELCOUNT, 0, 0);
	int* index = new int[n];
	SendMessage(child, LB_GETSELITEMS, (WPARAM)n, (LPARAM)index);

	// get the items in a vector
	std::vector<CString> tmpList;
	for (CScreens::const_iterator i = inScreens.begin();
								i != inScreens.end(); ++i) {
		tmpList.push_back(*i);
	}

	// get selected items into the output set
	outScreens.clear();
	for (UInt32 i = 0; i < n; ++i) {
		outScreens.insert(tmpList[index[i]]);
	}

	// clean up
	delete[] index;
}

BOOL CALLBACK
CHotkeyOptions::CScreensDialog::dlgProc(HWND hwnd,
				UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_INITDIALOG:
		doInit(hwnd);
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			doFini(hwnd);
			EndDialog(hwnd, 0);
			return TRUE;

		case IDCANCEL:
			EndDialog(hwnd, 0);
			return TRUE;

		case IDC_HOTKEY_SCREENS_ADD:
			add(hwnd);
			return TRUE;

		case IDC_HOTKEY_SCREENS_REMOVE:
			remove(hwnd);
			return TRUE;

		case IDC_HOTKEY_SCREENS_SRC:
		case IDC_HOTKEY_SCREENS_DST:
			switch (HIWORD(wParam)) {
			case LBN_SELCANCEL:
			case LBN_SELCHANGE:
				updateControls(hwnd);
				return TRUE;
			}
			break;
		}
		break;

	case WM_CTLCOLORLISTBOX:
		if (s_targets.empty() &&
			(HWND)lParam == getItem(hwnd, IDC_HOTKEY_SCREENS_DST)) {
			// override colors
			HDC dc = (HDC)wParam;
			SetTextColor(dc, GetSysColor(COLOR_GRAYTEXT));
			return (BOOL)GetSysColorBrush(COLOR_WINDOW);
		}
		break;

	default:
		break;
	}

	return FALSE;
}
