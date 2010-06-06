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

#include "CLog.h"
#include "ILogOutputter.h"
#include "CArch.h"
#include "CStringUtil.h"
#include "XArch.h"
#include "CAutoStart.h"
#include "LaunchUtil.h"
#include "resource.h"

static const char* CLIENT_DAEMON_NAME = "Synergy+ Client";
static const char* SERVER_DAEMON_NAME = "Synergy+ Server";
static const char* CLIENT_DAEMON_INFO = "Uses a shared mouse and keyboard.";
static const char* SERVER_DAEMON_INFO = "Shares this system's mouse and keyboard with others.";

//
// CAutoStartOutputter
//
// This class detects a message above a certain level and saves it
//

class CAutoStartOutputter : public ILogOutputter {
public:
	CAutoStartOutputter(CString* msg) : m_msg(msg) { }
	virtual ~CAutoStartOutputter() { }

	// ILogOutputter overrides
	virtual void		open(const char*) { }
	virtual void		close() { }
	virtual void		show(bool) { }
	virtual bool		write(ELevel level, const char* message);
	virtual const char*	getNewline() const { return ""; }

private:
	CString*			m_msg;
};

bool
CAutoStartOutputter::write(ELevel level, const char* message)
{
	if (level <= CLog::kERROR) {
		*m_msg = message;
	}
	return false;
}


//
// CAutoStart
//

CAutoStart*				CAutoStart::s_singleton = NULL;

CAutoStart::CAutoStart(HWND parent, bool isServer, const CString& cmdLine) :
	m_parent(parent),
	m_isServer(isServer),
	m_cmdLine(cmdLine),
	m_name(isServer ? SERVER_DAEMON_NAME : CLIENT_DAEMON_NAME)
{
	assert(s_singleton == NULL);
	s_singleton = this;
}

CAutoStart::~CAutoStart()
{
	s_singleton = NULL;
}

void
CAutoStart::doModal()
{
	// install our log outputter
	CLOG->insert(new CAutoStartOutputter(&m_errorMessage));

	// do dialog
	DialogBoxParam(s_instance, MAKEINTRESOURCE(IDD_AUTOSTART),
								m_parent, (DLGPROC)dlgProc, (LPARAM)this);

	// remove log outputter
	CLOG->pop_front();
}

void
CAutoStart::reinstallDaemon(bool isClient, const CString& cmdLine)
{
	// get installation state
	const char* name = (isClient ? CLIENT_DAEMON_NAME : SERVER_DAEMON_NAME);
	bool installedSystem = ARCH->isDaemonInstalled(name, true);
	bool installedUser   = ARCH->isDaemonInstalled(name, false);

	// reinstall if anything is installed
	if (installedSystem || installedUser) {
		ARCH->installDaemon(name,
					isClient ? CLIENT_DAEMON_INFO : SERVER_DAEMON_INFO,
					getAppPath(isClient ? CLIENT_APP : SERVER_APP).c_str(),
					cmdLine.c_str(),
					NULL,
					installedSystem);
	}
}

void
CAutoStart::uninstallDaemons(bool client)
{
	if (client) {
		try {
			ARCH->uninstallDaemon(CLIENT_DAEMON_NAME, true);
		}
		catch (...) {
		}
		try {
			ARCH->uninstallDaemon(CLIENT_DAEMON_NAME, false);
		}
		catch (...) {
		}
	}
	else {
		try {
			ARCH->uninstallDaemon(SERVER_DAEMON_NAME, true);
		}
		catch (...) {
		}
		try {
			ARCH->uninstallDaemon(SERVER_DAEMON_NAME, false);
		}
		catch (...) {
		}
	}
}

bool
CAutoStart::startDaemon()
{
	const char* name = NULL;
	if (ARCH->isDaemonInstalled(CLIENT_DAEMON_NAME, true)) {
		name = CLIENT_DAEMON_NAME;
	}
	else if (ARCH->isDaemonInstalled(SERVER_DAEMON_NAME, true)) {
		name = SERVER_DAEMON_NAME;
	}
	if (name == NULL) {
		return false;
	}

	// open service manager
	SC_HANDLE mgr = OpenSCManager(NULL, NULL, GENERIC_READ);
	if (mgr == NULL) {
		return false;
	}

	// open the service
	SC_HANDLE service = OpenService(mgr, name, SERVICE_START);
	if (service == NULL) {
		CloseServiceHandle(mgr);
		return false;
	}

	// start the service
	BOOL okay = StartService(service, 0, NULL);

	// clean up
	CloseServiceHandle(service);
	CloseServiceHandle(mgr);

	return (okay != 0);
}

bool
CAutoStart::isDaemonInstalled()
{
	return (ARCH->isDaemonInstalled(CLIENT_DAEMON_NAME, false) ||
			ARCH->isDaemonInstalled(CLIENT_DAEMON_NAME, true) ||
			ARCH->isDaemonInstalled(SERVER_DAEMON_NAME, false) ||
			ARCH->isDaemonInstalled(SERVER_DAEMON_NAME, true));
}

void
CAutoStart::update()
{
	// get installation state
	const bool installedSystem = ARCH->isDaemonInstalled(
										m_name.c_str(), true);
	const bool installedUser   = ARCH->isDaemonInstalled(
										m_name.c_str(), false);

	// get user's permissions
	const bool canInstallSystem = ARCH->canInstallDaemon(
										m_name.c_str(), true);
	const bool canInstallUser   = ARCH->canInstallDaemon(
										m_name.c_str(), false);

	// update messages
	CString msg, label;
	if (canInstallSystem) {
		if (canInstallUser) {
			msg = getString(IDS_AUTOSTART_PERMISSION_ALL);
		}
		else {
			msg = getString(IDS_AUTOSTART_PERMISSION_SYSTEM);
		}
	}
	else if (canInstallUser) {
		msg = getString(IDS_AUTOSTART_PERMISSION_USER);
	}
	else {
		msg = getString(IDS_AUTOSTART_PERMISSION_NONE);
	}
	setWindowText(getItem(m_hwnd, IDC_AUTOSTART_PERMISSION_MSG), msg);
	if (installedSystem) {
		msg   = getString(IDS_AUTOSTART_INSTALLED_SYSTEM);
		label = getString(IDS_UNINSTALL_LABEL);
	}
	else if (installedUser) {
		msg = getString(IDS_AUTOSTART_INSTALLED_USER);
		label = getString(IDS_UNINSTALL_LABEL);
	}
	else {
		msg = getString(IDS_AUTOSTART_INSTALLED_NONE);
		label = getString(IDS_INSTALL_LABEL);
	}
	setWindowText(getItem(m_hwnd, IDC_AUTOSTART_INSTALLED_MSG), msg);

	// update buttons
	setWindowText(getItem(m_hwnd, IDC_AUTOSTART_INSTALL_SYSTEM), label);
	setWindowText(getItem(m_hwnd, IDC_AUTOSTART_INSTALL_USER), label);
	if (installedSystem) {
		enableItem(m_hwnd, IDC_AUTOSTART_INSTALL_SYSTEM, canInstallSystem);
		enableItem(m_hwnd, IDC_AUTOSTART_INSTALL_USER, false);
		m_install = false;
	}
	else if (installedUser) {
		enableItem(m_hwnd, IDC_AUTOSTART_INSTALL_SYSTEM, false);
		enableItem(m_hwnd, IDC_AUTOSTART_INSTALL_USER, canInstallUser);
		m_install = false;
	}
	else {
		enableItem(m_hwnd, IDC_AUTOSTART_INSTALL_SYSTEM, canInstallSystem);
		enableItem(m_hwnd, IDC_AUTOSTART_INSTALL_USER, canInstallUser);
		m_install = true;
	}
}

bool
CAutoStart::onInstall(bool allUsers)
{
	if (!m_install) {
		return onUninstall(allUsers);
	}

	// get the app path
	CString appPath = getAppPath(m_isServer ? SERVER_APP : CLIENT_APP);

	// clear error message
	m_errorMessage = "";

	// install
	try {
		ARCH->installDaemon(m_name.c_str(),
					m_isServer ? SERVER_DAEMON_INFO : CLIENT_DAEMON_INFO,
					appPath.c_str(), m_cmdLine.c_str(),
					NULL, allUsers);
		askOkay(m_hwnd, getString(IDS_INSTALL_TITLE),
								getString(allUsers ?
									IDS_INSTALLED_SYSTEM :
									IDS_INSTALLED_USER));
		return true;
	}
	catch (XArchDaemon& e) {
		if (m_errorMessage.empty()) {
			m_errorMessage = CStringUtil::format(
								getString(IDS_INSTALL_GENERIC_ERROR).c_str(),
								e.what().c_str());
		}
		showError(m_hwnd, m_errorMessage);
		return false;
	}
}

bool
CAutoStart::onUninstall(bool allUsers)
{
	// clear error message
	m_errorMessage = "";

	// uninstall
	try {
		ARCH->uninstallDaemon(m_name.c_str(), allUsers);
		askOkay(m_hwnd, getString(IDS_UNINSTALL_TITLE),
								getString(allUsers ?
									IDS_UNINSTALLED_SYSTEM :
									IDS_UNINSTALLED_USER));
		return true;
	}
	catch (XArchDaemon& e) {
		if (m_errorMessage.empty()) {
			m_errorMessage = CStringUtil::format(
								getString(IDS_UNINSTALL_GENERIC_ERROR).c_str(),
								e.what().c_str());
		}
		showError(m_hwnd, m_errorMessage);
		return false;
	}
}

BOOL
CAutoStart::doDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM)
{
	switch (message) {
	case WM_INITDIALOG:
		// save our hwnd
		m_hwnd = hwnd;

		// update the controls
		update();

		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_AUTOSTART_INSTALL_SYSTEM:
			onInstall(true);
			update();
			return TRUE;

		case IDC_AUTOSTART_INSTALL_USER:
			onInstall(false);
			update();
			return TRUE;

		case IDCANCEL:
			EndDialog(hwnd, 0);
			m_hwnd = NULL;
			return TRUE;
		}
		break;

	default:
		break;
	}

	return FALSE;
}

BOOL CALLBACK
CAutoStart::dlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return s_singleton->doDlgProc(hwnd, message, wParam, lParam);
}
