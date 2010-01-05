/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman
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

#include "CConfig.h"
#include "KeyTypes.h"
#include "OptionTypes.h"
#include "ProtocolTypes.h"
#include "CStringUtil.h"
#include "CArch.h"
#include "CAddScreen.h"
#include "LaunchUtil.h"
#include "resource.h"

struct CModifierInfo {
public:
	int				m_ctrlID;
	const char*		m_name;
	KeyModifierID	m_modifierID;
	OptionID		m_optionID;
};

static const CModifierInfo s_modifiers[] = {
	{ IDC_ADD_MOD_SHIFT, "Shift",
		kKeyModifierIDShift,    kOptionModifierMapForShift   },
	{ IDC_ADD_MOD_CTRL,  "Ctrl",
		kKeyModifierIDControl,  kOptionModifierMapForControl },
	{ IDC_ADD_MOD_ALT,   "Alt",
		kKeyModifierIDAlt,      kOptionModifierMapForAlt     },
	{ IDC_ADD_MOD_META,  "Meta",
		kKeyModifierIDMeta,     kOptionModifierMapForMeta    },
	{ IDC_ADD_MOD_SUPER, "Super",
		kKeyModifierIDSuper,    kOptionModifierMapForSuper   }
};

static const KeyModifierID baseModifier = kKeyModifierIDShift;

//
// CAddScreen
//

CAddScreen*			CAddScreen::s_singleton = NULL;

CAddScreen::CAddScreen(HWND parent, CConfig* config, const CString& name) :
	m_parent(parent),
	m_config(config),
	m_name(name)
{
	assert(s_singleton == NULL);
	s_singleton = this;
}

CAddScreen::~CAddScreen()
{
	s_singleton = NULL;
}

bool
CAddScreen::doModal()
{
	// do dialog
	return (DialogBoxParam(s_instance, MAKEINTRESOURCE(IDD_ADD),
								m_parent, (DLGPROC)dlgProc, (LPARAM)this) != 0);
}

CString
CAddScreen::getName() const
{
	return m_name;
}

void
CAddScreen::init(HWND hwnd)
{
	// set title
	CString title;
	if (m_name.empty()) {
		title = getString(IDS_ADD_SCREEN);
	}
	else {
		title = CStringUtil::format(
							getString(IDS_EDIT_SCREEN).c_str(),
							m_name.c_str());
	}
	SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)title.c_str());

	// fill in screen name
	HWND child = getItem(hwnd, IDC_ADD_SCREEN_NAME_EDIT);
	SendMessage(child, WM_SETTEXT, 0, (LPARAM)m_name.c_str());

	// fill in aliases
	CString aliases;
	for (CConfig::all_const_iterator index = m_config->beginAll();
								index != m_config->endAll(); ++index) {
		if (CStringUtil::CaselessCmp::equal(index->second, m_name) &&
			!CStringUtil::CaselessCmp::equal(index->second, index->first)) {
			if (!aliases.empty()) {
				aliases += "\r\n";
			}
			aliases += index->first;
		}
	}
	child = getItem(hwnd, IDC_ADD_ALIASES_EDIT);
	SendMessage(child, WM_SETTEXT, 0, (LPARAM)aliases.c_str());

	// set options
	CConfig::CScreenOptions options;
	getOptions(options);
	CConfig::CScreenOptions::const_iterator index;
	child = getItem(hwnd, IDC_ADD_HD_CAPS_CHECK);
	index = options.find(kOptionHalfDuplexCapsLock);
	setItemChecked(child, (index != options.end() && index->second != 0));
	child = getItem(hwnd, IDC_ADD_HD_NUM_CHECK);
	index = options.find(kOptionHalfDuplexNumLock);
	setItemChecked(child, (index != options.end() && index->second != 0));
	child = getItem(hwnd, IDC_ADD_HD_SCROLL_CHECK);
	index = options.find(kOptionHalfDuplexScrollLock);
	setItemChecked(child, (index != options.end() && index->second != 0));

	// modifier options
	for (UInt32 i = 0; i < sizeof(s_modifiers) /
								sizeof(s_modifiers[0]); ++i) {
		child = getItem(hwnd, s_modifiers[i].m_ctrlID);

		// fill in options
		for (UInt32 j = 0; j < sizeof(s_modifiers) /
									sizeof(s_modifiers[0]); ++j) {
			SendMessage(child, CB_ADDSTRING, 0,
								(LPARAM)s_modifiers[j].m_name);
		}

		// choose current value
		index            = options.find(s_modifiers[i].m_optionID);
		KeyModifierID id = s_modifiers[i].m_modifierID;
		if (index != options.end()) {
			id = index->second;
		}
		SendMessage(child, CB_SETCURSEL, id - baseModifier, 0);
	}

	// dead corners
	UInt32 corners = 0;
	index = options.find(kOptionScreenSwitchCorners);
	if (index != options.end()) {
		corners = index->second;
	}
	child = getItem(hwnd, IDC_ADD_DC_TOP_LEFT);
	setItemChecked(child, (corners & kTopLeftMask) != 0);
	child = getItem(hwnd, IDC_ADD_DC_TOP_RIGHT);
	setItemChecked(child, (corners & kTopRightMask) != 0);
	child = getItem(hwnd, IDC_ADD_DC_BOTTOM_LEFT);
	setItemChecked(child, (corners & kBottomLeftMask) != 0);
	child = getItem(hwnd, IDC_ADD_DC_BOTTOM_RIGHT);
	setItemChecked(child, (corners & kBottomRightMask) != 0);
	index = options.find(kOptionScreenSwitchCornerSize);
	SInt32 size = 0;
	if (index != options.end()) {
		size = index->second;
	}
	char buffer[20];
	sprintf(buffer, "%d", size);
	child = getItem(hwnd, IDC_ADD_DC_SIZE);
	SendMessage(child, WM_SETTEXT, 0, (LPARAM)buffer);
}

bool
CAddScreen::save(HWND hwnd)
{
	// get the old aliases and options
	CStringList oldAliases;
	getAliases(oldAliases);
	CConfig::CScreenOptions options;
	getOptions(options);

	// extract name and aliases
	CString newName;
	HWND child = getItem(hwnd, IDC_ADD_SCREEN_NAME_EDIT);
	newName = getWindowText(child);
	CStringList newAliases;
	child = getItem(hwnd, IDC_ADD_ALIASES_EDIT);
	tokenize(newAliases, getWindowText(child));

	// name must be valid
	if (!m_config->isValidScreenName(newName)) {
		showError(hwnd, CStringUtil::format(
						getString(IDS_INVALID_SCREEN_NAME).c_str(),
						newName.c_str()));
		return false;
	}

	// aliases must be valid
	for (CStringList::const_iterator index = newAliases.begin();
						index != newAliases.end(); ++index) {
		if (!m_config->isValidScreenName(*index)) {
			showError(hwnd, CStringUtil::format(
						getString(IDS_INVALID_SCREEN_NAME).c_str(),
						index->c_str()));
			return false;
		}
	}

	// new name may not be in the new alias list
	if (isNameInList(newAliases, newName)) {
		showError(hwnd, CStringUtil::format(
						getString(IDS_SCREEN_NAME_IS_ALIAS).c_str(),
						newName.c_str()));
		return false;
	}

	// name must not exist in config but allow same name.  also
	// allow name if it exists in the old alias list but not the
	// new one.
	if (m_config->isScreen(newName) &&
		!CStringUtil::CaselessCmp::equal(newName, m_name) &&
		!isNameInList(oldAliases, newName)) {
		showError(hwnd, CStringUtil::format(
						getString(IDS_DUPLICATE_SCREEN_NAME).c_str(),
						newName.c_str()));
		return false;
	}

	// aliases must not exist in config but allow same aliases and
	// allow an alias to be the old name.
	for (CStringList::const_iterator index = newAliases.begin();
						index != newAliases.end(); ++index) {
		if (m_config->isScreen(*index) &&
			!CStringUtil::CaselessCmp::equal(*index, m_name) &&
			!isNameInList(oldAliases, *index)) {
			showError(hwnd, CStringUtil::format(
						getString(IDS_DUPLICATE_SCREEN_NAME).c_str(),
						index->c_str()));
			return false;
		}
	}

	// dead corner size must be non-negative
	child = getItem(hwnd, IDC_ADD_DC_SIZE);
	CString valueString = getWindowText(child);
	int cornerSize = atoi(valueString.c_str());
	if (cornerSize < 0) {
		showError(hwnd, CStringUtil::format(
							getString(IDS_INVALID_CORNER_SIZE).c_str(),
							valueString.c_str()));
		SetFocus(child);
		return false;
	}

	// collect options
	child = getItem(hwnd, IDC_ADD_HD_CAPS_CHECK);
	if (isItemChecked(child)) {
		options[kOptionHalfDuplexCapsLock] = 1;
	}
	else {
		options.erase(kOptionHalfDuplexCapsLock);
	}
	child = getItem(hwnd, IDC_ADD_HD_NUM_CHECK);
	if (isItemChecked(child)) {
		options[kOptionHalfDuplexNumLock] = 1;
	}
	else {
		options.erase(kOptionHalfDuplexNumLock);
	}
	child = getItem(hwnd, IDC_ADD_HD_SCROLL_CHECK);
	if (isItemChecked(child)) {
		options[kOptionHalfDuplexScrollLock] = 1;
	}
	else {
		options.erase(kOptionHalfDuplexScrollLock);
	}

	// save modifier options
	for (UInt32 i = 0; i < sizeof(s_modifiers) /
								sizeof(s_modifiers[0]); ++i) {
		child            = getItem(hwnd, s_modifiers[i].m_ctrlID);
		KeyModifierID id = static_cast<KeyModifierID>(
							SendMessage(child, CB_GETCURSEL, 0, 0) +
								baseModifier);
		if (id != s_modifiers[i].m_modifierID) {
			options[s_modifiers[i].m_optionID] = id;
		}
		else {
			options.erase(s_modifiers[i].m_optionID);
		}
	}

	// save dead corner options
	UInt32 corners = 0;
	if (isItemChecked(getItem(hwnd, IDC_ADD_DC_TOP_LEFT))) {
		corners |= kTopLeftMask;
	}
	if (isItemChecked(getItem(hwnd, IDC_ADD_DC_TOP_RIGHT))) {
		corners |= kTopRightMask;
	}
	if (isItemChecked(getItem(hwnd, IDC_ADD_DC_BOTTOM_LEFT))) {
		corners |= kBottomLeftMask;
	}
	if (isItemChecked(getItem(hwnd, IDC_ADD_DC_BOTTOM_RIGHT))) {
		corners |= kBottomRightMask;
	}
	options[kOptionScreenSwitchCorners]    = corners;
	options[kOptionScreenSwitchCornerSize] = cornerSize;

	// save new data to config
	if (m_name.empty()) {
		// added screen
		m_config->addScreen(newName);
	}
	else {
		// edited screen
		m_config->removeAliases(m_name);
		m_config->removeOptions(m_name);
		m_config->renameScreen(m_name, newName);
	}
	m_name = newName;
	for (CStringList::const_iterator index = newAliases.begin();
							index != newAliases.end(); ++index) {
		m_config->addAlias(m_name, *index);
	}
	for (CConfig::CScreenOptions::const_iterator
							index  = options.begin();
							index != options.end(); ++index) {
		m_config->addOption(m_name, index->first, index->second);
	}

	return true;
}

BOOL
CAddScreen::doDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM)
{
	switch (message) {
	case WM_INITDIALOG:
		init(hwnd);
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			if (save(hwnd)) {
				EndDialog(hwnd, 1);
			}
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

BOOL CALLBACK
CAddScreen::dlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return s_singleton->doDlgProc(hwnd, message, wParam, lParam);
}

void
CAddScreen::getAliases(CStringList& aliases) const
{
	for (CConfig::all_const_iterator index = m_config->beginAll();
								index != m_config->endAll(); ++index) {
		if (CStringUtil::CaselessCmp::equal(index->second, m_name) &&
			!CStringUtil::CaselessCmp::equal(index->second, index->first)) {
			aliases.push_back(index->first);
		}
	}
}

void
CAddScreen::getOptions(CConfig::CScreenOptions& optionsOut) const
{
	const CConfig::CScreenOptions* options = m_config->getOptions(m_name);
	if (options == NULL) {
		optionsOut = CConfig::CScreenOptions();
	}
	else {
		optionsOut = *options;
	}
}

void
CAddScreen::tokenize(CStringList& tokens, const CString& src)
{
	// find first non-whitespace
	CString::size_type x = src.find_first_not_of(" \t\r\n");
	if (x == CString::npos) {
		return;
	}

	// find next whitespace
	do {
		CString::size_type y = src.find_first_of(" \t\r\n", x);
		if (y == CString::npos) {
			y = src.size();
		}
		tokens.push_back(src.substr(x, y - x));
		x = src.find_first_not_of(" \t\r\n", y);
	} while (x != CString::npos);
}

bool
CAddScreen::isNameInList(const CStringList& names, const CString& name)
{
	for (CStringList::const_iterator index = names.begin();
								index != names.end(); ++index) {
		if (CStringUtil::CaselessCmp::equal(name, *index)) {
			return true;
		}
	}
	return false;
}
