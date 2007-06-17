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

#include "ProtocolTypes.h"
#include "CStringUtil.h"
#include "Version.h"
#include "CArch.h"
#include "CInfo.h"
#include "LaunchUtil.h"
#include "resource.h"

//
// CInfo
//

CInfo*					CInfo::s_singleton = NULL;

CInfo::CInfo(HWND parent) :
	m_parent(parent)
{
	assert(s_singleton == NULL);
	s_singleton = this;
}

CInfo::~CInfo()
{
	s_singleton = NULL;
}

void
CInfo::doModal()
{
	// do dialog
	DialogBoxParam(s_instance, MAKEINTRESOURCE(IDD_INFO),
								m_parent, dlgProc, (LPARAM)this);
}

void
CInfo::init(HWND hwnd)
{
	// collect info
	CString version    =
		CStringUtil::format(getString(IDS_TITLE).c_str(), VERSION);
	CString hostname   = ARCH->getHostName();
	CString address    = ARCH->addrToString(ARCH->nameToAddr(hostname));
	CString userConfig = ARCH->getUserDirectory();
	if (!userConfig.empty()) {
		userConfig = ARCH->concatPath(userConfig, CONFIG_NAME);
	}
	CString sysConfig  = ARCH->getSystemDirectory();
	if (!sysConfig.empty()) {
		sysConfig = ARCH->concatPath(sysConfig, CONFIG_NAME);
	}

	// set info
	HWND child;
	child = getItem(hwnd, IDC_INFO_VERSION);
	setWindowText(child, version);
	child = getItem(hwnd, IDC_INFO_HOSTNAME);
	setWindowText(child, hostname);
	child = getItem(hwnd, IDC_INFO_IP_ADDRESS);
	setWindowText(child, address);
	child = getItem(hwnd, IDC_INFO_USER_CONFIG);
	setWindowText(child, userConfig);
	child = getItem(hwnd, IDC_INFO_SYS_CONFIG);
	setWindowText(child, sysConfig);

	// focus on okay button
	SetFocus(getItem(hwnd, IDOK));
}

BOOL
CInfo::doDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM)
{
	switch (message) {
	case WM_INITDIALOG:
		init(hwnd);
		return FALSE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
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
CInfo::dlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return s_singleton->doDlgProc(hwnd, message, wParam, lParam);
}
