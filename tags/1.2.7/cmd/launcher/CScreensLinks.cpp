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
#include "ProtocolTypes.h"
#include "CStringUtil.h"
#include "CArch.h"
#include "CScreensLinks.h"
#include "CAddScreen.h"
#include "LaunchUtil.h"
#include "resource.h"

//
// CScreensLinks
//

CScreensLinks*		CScreensLinks::s_singleton = NULL;

CScreensLinks::CScreensLinks(HWND parent, CConfig* config) :
	m_parent(parent),
	m_mainConfig(config),
	m_config(&m_scratchConfig)
{
	assert(s_singleton == NULL);
	s_singleton = this;
}

CScreensLinks::~CScreensLinks()
{
	s_singleton = NULL;
}

void
CScreensLinks::doModal()
{
	// do dialog
	DialogBoxParam(s_instance, MAKEINTRESOURCE(IDD_SCREENS_LINKS),
								m_parent, dlgProc, (LPARAM)this);
}

void
CScreensLinks::init(HWND hwnd)
{
	// get initial config
	m_scratchConfig = *m_mainConfig;

	// fill side list box (in EDirection order)
	HWND child = getItem(hwnd, IDC_SCREENS_SRC_SIDE);
	SendMessage(child, CB_ADDSTRING, 0, (LPARAM)TEXT("---"));
	SendMessage(child, CB_ADDSTRING, 0,
							(LPARAM)getString(IDS_EDGE_LEFT).c_str());
	SendMessage(child, CB_ADDSTRING, 0,
							(LPARAM)getString(IDS_EDGE_RIGHT).c_str());
	SendMessage(child, CB_ADDSTRING, 0,
							(LPARAM)getString(IDS_EDGE_TOP).c_str());
	SendMessage(child, CB_ADDSTRING, 0,
							(LPARAM)getString(IDS_EDGE_BOTTOM).c_str());

	updateScreens(hwnd, "");
	updateScreensControls(hwnd);
	updateLinksControls(hwnd);
}

bool
CScreensLinks::save(HWND /*hwnd*/)
{
	*m_mainConfig = m_scratchConfig;
	return true;
}

BOOL
CScreensLinks::doDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_INITDIALOG:
		init(hwnd);
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			if (save(hwnd)) {
				EndDialog(hwnd, 0);
			}
			return TRUE;

		case IDCANCEL:
			EndDialog(hwnd, 0);
			return TRUE;

		case IDC_SCREENS_SCREENS:
			switch (HIWORD(wParam)) {
			case LBN_DBLCLK:
				editScreen(hwnd);
				return TRUE;

			case LBN_SELCHANGE:
				updateScreensControls(hwnd);
				updateLinksControls(hwnd);
				return TRUE;

			case LBN_SELCANCEL:
				updateScreensControls(hwnd);
				updateLinksControls(hwnd);
				return TRUE;
			}
			break;

		case IDC_SCREENS_ADD_SCREEN:
			addScreen(hwnd);
			return TRUE;

		case IDC_SCREENS_REMOVE_SCREEN:
			removeScreen(hwnd);
			return TRUE;

		case IDC_SCREENS_EDIT_SCREEN:
			editScreen(hwnd);
			return TRUE;

		case IDC_SCREENS_LEFT_COMBO:
			if (HIWORD(wParam) == CBN_SELENDOK) {
				changeNeighbor(hwnd, (HWND)lParam, kLeft);
				return TRUE;
			}
			break;

		case IDC_SCREENS_RIGHT_COMBO:
			if (HIWORD(wParam) == CBN_SELENDOK) {
				changeNeighbor(hwnd, (HWND)lParam, kRight);
				return TRUE;
			}
			break;

		case IDC_SCREENS_TOP_COMBO:
			if (HIWORD(wParam) == CBN_SELENDOK) {
				changeNeighbor(hwnd, (HWND)lParam, kTop);
				return TRUE;
			}
			break;

		case IDC_SCREENS_BOTTOM_COMBO:
			if (HIWORD(wParam) == CBN_SELENDOK) {
				changeNeighbor(hwnd, (HWND)lParam, kBottom);
				return TRUE;
			}
			break;
		}

		break;

	default:
		break;
	}

	return FALSE;
}

BOOL CALLBACK
CScreensLinks::dlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return s_singleton->doDlgProc(hwnd, message, wParam, lParam);
}

CString
CScreensLinks::getSelectedScreen(HWND hwnd) const
{
	HWND child    = getItem(hwnd, IDC_SCREENS_SCREENS);
	LRESULT index = SendMessage(child, LB_GETCURSEL, 0, 0);
	if (index == LB_ERR) {
		return CString();
	}

	LRESULT size = SendMessage(child, LB_GETTEXTLEN, index, 0);
	char* buffer = new char[size + 1];
	SendMessage(child, LB_GETTEXT, index, (LPARAM)buffer);
	buffer[size] = '\0';
	CString result(buffer);
	delete[] buffer;
	return result;
}

void
CScreensLinks::addScreen(HWND hwnd)
{
	CAddScreen dialog(hwnd, m_config, "");
	if (dialog.doModal()) {
		updateScreens(hwnd, dialog.getName());
		updateScreensControls(hwnd);
		updateLinksControls(hwnd);
	}
}

void
CScreensLinks::editScreen(HWND hwnd)
{
	CString oldName = getSelectedScreen(hwnd);
	CAddScreen dialog(hwnd, m_config, oldName);
	if (dialog.doModal()) {
		CString newName = dialog.getName();
		updateScreens(hwnd, newName);
		updateScreensControls(hwnd);
		updateLinksControls(hwnd);
	}
}

void
CScreensLinks::removeScreen(HWND hwnd)
{
	// remove screen from config (this also removes aliases)
	m_config->removeScreen(getSelectedScreen(hwnd));

	// update dialog
	updateScreens(hwnd, "");
	updateScreensControls(hwnd);
	updateLinksControls(hwnd);
}

void
CScreensLinks::changeNeighbor(HWND hwnd, HWND combo, EDirection direction)
{
	// get selected screen
	CString screen = getSelectedScreen(hwnd);

	// get selected neighbor
	LRESULT index = SendMessage(combo, CB_GETCURSEL, 0, 0);

	// remove old connection
	m_config->disconnect(screen, direction);

	// add new connection
	if (index != LB_ERR && index != 0) {
		LRESULT size = SendMessage(combo, CB_GETLBTEXTLEN, index, 0);
		char* neighbor = new char[size + 1];
		SendMessage(combo, CB_GETLBTEXT, index, (LPARAM)neighbor);
		neighbor[size] = '\0';
		m_config->connect(screen, direction, CString(neighbor));
		delete[] neighbor;
	}
}

void
CScreensLinks::updateScreens(HWND hwnd, const CString& selectName)
{
	HWND child;

	// set screen list
	child = getItem(hwnd, IDC_SCREENS_SCREENS);
	SendMessage(child, LB_RESETCONTENT, 0, 0);
	for (CConfig::const_iterator index = m_config->begin();
							index != m_config->end(); ) {
		const CString& name = *index;
		++index;
		if (index != m_config->end()) {
			SendMessage(child, LB_INSERTSTRING,
							(WPARAM)-1, (LPARAM)name.c_str());
		}
		else {
			SendMessage(child, LB_ADDSTRING, 0, (LPARAM)name.c_str());
		}
	}

	// find the named screen
	if (!selectName.empty()) {
		DWORD i = SendMessage(child, LB_FINDSTRINGEXACT,
							(UINT)-1, (LPARAM)selectName.c_str());
		if (i != LB_ERR) {
			SendMessage(child, LB_SETSEL, TRUE, i);
		}
	}
}

void
CScreensLinks::updateScreensControls(HWND hwnd)
{
	HWND child    = getItem(hwnd, IDC_SCREENS_SCREENS);
	bool screenSelected = (SendMessage(child, LB_GETCURSEL, 0, 0) != LB_ERR);

	enableItem(hwnd, IDC_SCREENS_ADD_SCREEN, TRUE);
	enableItem(hwnd, IDC_SCREENS_EDIT_SCREEN, screenSelected);
	enableItem(hwnd, IDC_SCREENS_REMOVE_SCREEN, screenSelected);
}

void
CScreensLinks::updateLinksControls(HWND hwnd)
{
	// get selected screen name or empty string if no selection
	CString screen = getSelectedScreen(hwnd);;

	// set neighbor combo boxes
	HWND child;
	child = getItem(hwnd, IDC_SCREENS_LEFT_COMBO);
	updateLink(child, screen, kLeft);
	child = getItem(hwnd, IDC_SCREENS_RIGHT_COMBO);
	updateLink(child, screen, kRight);
	child = getItem(hwnd, IDC_SCREENS_TOP_COMBO);
	updateLink(child, screen, kTop);
	child = getItem(hwnd, IDC_SCREENS_BOTTOM_COMBO);
	updateLink(child, screen, kBottom);
}

void
CScreensLinks::updateLink(HWND hwnd,
				const CString& screen, EDirection direction)
{
	// remove all neighbors from combo box
	SendMessage(hwnd, CB_RESETCONTENT, 0, 0);

	// add all screens to combo box
	if (!screen.empty()) {
		for (CConfig::const_iterator index  = m_config->begin();
									 index != m_config->end(); ++index) {
			SendMessage(hwnd, CB_INSERTSTRING,
								(WPARAM)-1, (LPARAM)index->c_str());
		}
	}

	// add empty neighbor to combo box
	SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)TEXT("---"));

	// select neighbor in combo box
	LRESULT index = 0;
	if (!screen.empty()) {
		const CString& neighbor = m_config->getNeighbor(screen, direction);
		if (!neighbor.empty()) {
			index = SendMessage(hwnd, CB_FINDSTRINGEXACT,
								0, (LPARAM)neighbor.c_str());
			if (index == LB_ERR) {
				index = 0;
			}
		}
	}
	SendMessage(hwnd, CB_SETCURSEL, index, 0);
}
