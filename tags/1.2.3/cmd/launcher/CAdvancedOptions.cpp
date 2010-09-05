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
#include "CArchMiscWindows.h"
#include "CAdvancedOptions.h"
#include "LaunchUtil.h"
#include "XArch.h"
#include "resource.h"

//
// CAdvancedOptions
//

CAdvancedOptions*		CAdvancedOptions::s_singleton = NULL;

CAdvancedOptions::CAdvancedOptions(HWND parent, CConfig* config) :
	m_parent(parent),
	m_config(config),
	m_isClient(false),
	m_screenName(ARCH->getHostName()),
	m_port(kDefaultPort),
	m_interface()
{
	assert(s_singleton == NULL);
	s_singleton = this;
	init();
}

CAdvancedOptions::~CAdvancedOptions()
{
	s_singleton = NULL;
}

void
CAdvancedOptions::doModal(bool isClient)
{
	// save state
	m_isClient = isClient;

	// do dialog
	DialogBoxParam(s_instance, MAKEINTRESOURCE(IDD_ADVANCED_OPTIONS),
								m_parent, dlgProc, (LPARAM)this);
}

CString
CAdvancedOptions::getScreenName() const
{
	return m_screenName;
}

int
CAdvancedOptions::getPort() const
{
	return m_port;
}

CString
CAdvancedOptions::getInterface() const
{
	return m_interface;
}

CString
CAdvancedOptions::getCommandLine(bool isClient, const CString& serverName) const
{
	CString cmdLine;

	// screen name
	if (!m_screenName.empty()) {
		cmdLine += " --name ";
		cmdLine += m_screenName;
	}

	// port
	char portString[20];
	sprintf(portString, "%d", m_port);
	if (isClient) {
		cmdLine += " ";
		cmdLine += serverName;
		cmdLine += ":";
		cmdLine += portString;
	}
	else {
		cmdLine += " --address ";
		if (!m_interface.empty()) {
			cmdLine += m_interface;
		}
		cmdLine += ":";
		cmdLine += portString;
	}

	return cmdLine;
}

void
CAdvancedOptions::init()
{
	// get values from registry
	HKEY key = CArchMiscWindows::openKey(HKEY_CURRENT_USER, getSettingsPath());
	if (key != NULL) {
		DWORD newPort        = CArchMiscWindows::readValueInt(key, "port");
		CString newName      = CArchMiscWindows::readValueString(key, "name");
		CString newInterface =
			CArchMiscWindows::readValueString(key, "interface");
		if (newPort != 0) {
			m_port = static_cast<int>(newPort);
		}
		if (!newName.empty()) {
			m_screenName = newName;
		}
		if (!newInterface.empty()) {
			m_interface = newInterface;
		}
		CArchMiscWindows::closeKey(key);
	}
}

void
CAdvancedOptions::doInit(HWND hwnd)
{
	// set values in GUI
	HWND child;
	char buffer[20];
	sprintf(buffer, "%d", m_port);
	child = getItem(hwnd, IDC_ADVANCED_PORT_EDIT);
	SendMessage(child, WM_SETTEXT, 0, (LPARAM)buffer);

	child = getItem(hwnd, IDC_ADVANCED_NAME_EDIT);
	SendMessage(child, WM_SETTEXT, 0, (LPARAM)m_screenName.c_str());

	child = getItem(hwnd, IDC_ADVANCED_INTERFACE_EDIT);
	SendMessage(child, WM_SETTEXT, 0, (LPARAM)m_interface.c_str());
}

bool
CAdvancedOptions::save(HWND hwnd)
{
	SetCursor(LoadCursor(NULL, IDC_WAIT));

	HWND child = getItem(hwnd, IDC_ADVANCED_NAME_EDIT);
	CString name = getWindowText(child);
	if (!m_config->isValidScreenName(name)) {
		showError(hwnd, CStringUtil::format(
								getString(IDS_INVALID_SCREEN_NAME).c_str(),
								name.c_str()));
		SetFocus(child);
		return false;
	}
	if (!m_isClient && !m_config->isScreen(name)) {
		showError(hwnd, CStringUtil::format(
								getString(IDS_UNKNOWN_SCREEN_NAME).c_str(),
								name.c_str()));
		SetFocus(child);
		return false;
	}

	child = getItem(hwnd, IDC_ADVANCED_INTERFACE_EDIT);
	CString iface = getWindowText(child);
	if (!m_isClient) {
		try {
			if (!iface.empty()) {
				ARCH->nameToAddr(iface);
			}
		}
		catch (XArchNetworkName& e) {
			showError(hwnd, CStringUtil::format(
								getString(IDS_INVALID_INTERFACE_NAME).c_str(),
								iface.c_str(), e.what().c_str()));
			SetFocus(child);
			return false;
		}
	}

	// get and verify port
	child = getItem(hwnd, IDC_ADVANCED_PORT_EDIT);
	CString portString = getWindowText(child);
	int port = atoi(portString.c_str());
	if (port < 1 || port > 65535) {
		CString defaultPortString = CStringUtil::print("%d", kDefaultPort);
		showError(hwnd, CStringUtil::format(
								getString(IDS_INVALID_PORT).c_str(),
								portString.c_str(),
								defaultPortString.c_str()));
		SetFocus(child);
		return false;
	}

	// save state
	m_screenName = name;
	m_port       = port;
	m_interface  = iface;

	// save values to registry
	HKEY key = CArchMiscWindows::openKey(HKEY_CURRENT_USER, getSettingsPath());
	if (key != NULL) {
		CArchMiscWindows::setValue(key, "port", m_port);
		CArchMiscWindows::setValue(key, "name", m_screenName);
		CArchMiscWindows::setValue(key, "interface", m_interface);
		CArchMiscWindows::closeKey(key);
	}

	return true;
}

void
CAdvancedOptions::setDefaults(HWND hwnd)
{
	// restore defaults
	m_screenName = ARCH->getHostName();
	m_port       = kDefaultPort;
	m_interface  = "";

	// update GUI
	doInit(hwnd);
}

BOOL
CAdvancedOptions::doDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM)
{
	switch (message) {
	case WM_INITDIALOG:
		doInit(hwnd);
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

		case IDC_ADVANCED_DEFAULTS:
			setDefaults(hwnd);
			return TRUE;
		}
		break;

	default:
		break;
	}

	return FALSE;
}

BOOL CALLBACK
CAdvancedOptions::dlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return s_singleton->doDlgProc(hwnd, message, wParam, lParam);
}
