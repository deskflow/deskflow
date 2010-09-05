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
#include "CGlobalOptions.h"
#include "LaunchUtil.h"
#include "resource.h"

static const int	s_defaultDelay     = 250;
static const int	s_defaultHeartbeat = 5000;

//
// CGlobalOptions
//

CGlobalOptions*		CGlobalOptions::s_singleton = NULL;

CGlobalOptions::CGlobalOptions(HWND parent, CConfig* config) :
	m_parent(parent),
	m_config(config),
	m_delayTime(s_defaultDelay),
	m_twoTapTime(s_defaultDelay),
	m_heartbeatTime(s_defaultHeartbeat)
{
	assert(s_singleton == NULL);
	s_singleton = this;
}

CGlobalOptions::~CGlobalOptions()
{
	s_singleton = NULL;
}

void
CGlobalOptions::doModal()
{
	// do dialog
	DialogBoxParam(s_instance, MAKEINTRESOURCE(IDD_GLOBAL_OPTIONS),
								m_parent, dlgProc, (LPARAM)this);
}

void
CGlobalOptions::init(HWND hwnd)
{
	HWND child;
	char buffer[30];

	// reset options
	sprintf(buffer, "%d", m_delayTime);
	child = getItem(hwnd, IDC_GLOBAL_DELAY_CHECK);
	setItemChecked(child, false);
	child = getItem(hwnd, IDC_GLOBAL_DELAY_TIME);
	setWindowText(child, buffer);
	sprintf(buffer, "%d", m_twoTapTime);
	child = getItem(hwnd, IDC_GLOBAL_TWO_TAP_CHECK);
	setItemChecked(child, false);
	child = getItem(hwnd, IDC_GLOBAL_TWO_TAP_TIME);
	setWindowText(child, buffer);
	sprintf(buffer, "%d", m_heartbeatTime);
	child = getItem(hwnd, IDC_GLOBAL_HEARTBEAT_CHECK);
	setItemChecked(child, false);
	child = getItem(hwnd, IDC_GLOBAL_HEARTBEAT_TIME);
	setWindowText(child, buffer);
	child = getItem(hwnd, IDC_GLOBAL_SCREENSAVER_SYNC);
	setItemChecked(child, true);
	child = getItem(hwnd, IDC_GLOBAL_RELATIVE_MOVES);
	setItemChecked(child, false);

	// get the global options
	const CConfig::CScreenOptions* options = m_config->getOptions("");
	if (options != NULL) {
		for (CConfig::CScreenOptions::const_iterator index = options->begin();
										index != options->end(); ++index) {
			const OptionID id       = index->first;
			const OptionValue value = index->second;
			if (id == kOptionScreenSwitchDelay) {
				if (value > 0) {
					sprintf(buffer, "%d", value);
					child = getItem(hwnd, IDC_GLOBAL_DELAY_CHECK);
					setItemChecked(child, true);
					child = getItem(hwnd, IDC_GLOBAL_DELAY_TIME);
					setWindowText(child, buffer);
				}
			}
			else if (id == kOptionScreenSwitchTwoTap) {
				if (value > 0) {
					sprintf(buffer, "%d", value);
					child = getItem(hwnd, IDC_GLOBAL_TWO_TAP_CHECK);
					setItemChecked(child, true);
					child = getItem(hwnd, IDC_GLOBAL_TWO_TAP_TIME);
					setWindowText(child, buffer);
				}
			}
			else if (id == kOptionHeartbeat) {
				if (value > 0) {
					sprintf(buffer, "%d", value);
					child = getItem(hwnd, IDC_GLOBAL_HEARTBEAT_CHECK);
					setItemChecked(child, true);
					child = getItem(hwnd, IDC_GLOBAL_HEARTBEAT_TIME);
					setWindowText(child, buffer);
				}
			}
			else if (id == kOptionScreenSaverSync) {
				child = getItem(hwnd, IDC_GLOBAL_SCREENSAVER_SYNC);
				setItemChecked(child, (value != 0));
			}
			else if (id == kOptionRelativeMouseMoves) {
				child = getItem(hwnd, IDC_GLOBAL_RELATIVE_MOVES);
				setItemChecked(child, (value != 0));
			}
		}
	}
}

bool
CGlobalOptions::save(HWND hwnd)
{
	HWND child;
	int newDelayTime     = 0;
	int newTwoTapTime    = 0;
	int newHeartbeatTime = 0;

	// get requested options
	child = getItem(hwnd, IDC_GLOBAL_DELAY_CHECK);
	if (isItemChecked(child)) {
		child         = getItem(hwnd, IDC_GLOBAL_DELAY_TIME);
		newDelayTime  = getTime(hwnd, child, true);
		if (newDelayTime == 0) {
			return false;
		}
	}
	else {
		child         = getItem(hwnd, IDC_GLOBAL_DELAY_TIME);
		newDelayTime  = getTime(hwnd, child, false);
		if (newDelayTime == 0) {
			newDelayTime = s_defaultDelay;
		}
	}
	child = getItem(hwnd, IDC_GLOBAL_TWO_TAP_CHECK);
	if (isItemChecked(child)) {
		child         = getItem(hwnd, IDC_GLOBAL_TWO_TAP_TIME);
		newTwoTapTime = getTime(hwnd, child, true);
		if (newTwoTapTime == 0) {
			return false;
		}
	}
	else {
		child         = getItem(hwnd, IDC_GLOBAL_TWO_TAP_TIME);
		newTwoTapTime = getTime(hwnd, child, false);
		if (newTwoTapTime == 0) {
			newTwoTapTime = s_defaultDelay;
		}
	}
	child = getItem(hwnd, IDC_GLOBAL_HEARTBEAT_CHECK);
	if (isItemChecked(child)) {
		child            = getItem(hwnd, IDC_GLOBAL_HEARTBEAT_TIME);
		newHeartbeatTime = getTime(hwnd, child, true);
		if (newHeartbeatTime == 0) {
			return false;
		}
	}
	else {
		child            = getItem(hwnd, IDC_GLOBAL_HEARTBEAT_TIME);
		newHeartbeatTime = getTime(hwnd, child, false);
		if (newHeartbeatTime == 0) {
			newHeartbeatTime = s_defaultHeartbeat;
		}
	}

	// remove existing config options
	m_config->removeOption("", kOptionScreenSwitchDelay);
	m_config->removeOption("", kOptionScreenSwitchTwoTap);
	m_config->removeOption("", kOptionHeartbeat);
	m_config->removeOption("", kOptionScreenSaverSync);
	m_config->removeOption("", kOptionRelativeMouseMoves);

	// add requested options
	child = getItem(hwnd, IDC_GLOBAL_DELAY_CHECK);
	if (isItemChecked(child)) {
		m_config->addOption("", kOptionScreenSwitchDelay, newDelayTime);
	}
	child = getItem(hwnd, IDC_GLOBAL_TWO_TAP_CHECK);
	if (isItemChecked(child)) {
		m_config->addOption("", kOptionScreenSwitchTwoTap, newTwoTapTime);
	}
	child = getItem(hwnd, IDC_GLOBAL_HEARTBEAT_CHECK);
	if (isItemChecked(child)) {
		m_config->addOption("", kOptionHeartbeat, newHeartbeatTime);
	}
	child = getItem(hwnd, IDC_GLOBAL_SCREENSAVER_SYNC);
	if (!isItemChecked(child)) {
		m_config->addOption("", kOptionScreenSaverSync, 0);
	}
	child = getItem(hwnd, IDC_GLOBAL_RELATIVE_MOVES);
	if (isItemChecked(child)) {
		m_config->addOption("", kOptionRelativeMouseMoves, 1);
	}

	// save last values
	m_delayTime     = newDelayTime;
	m_twoTapTime    = newTwoTapTime;
	m_heartbeatTime = newHeartbeatTime;
	return true;
}

int
CGlobalOptions::getTime(HWND hwnd, HWND child, bool reportError)
{
	CString valueString = getWindowText(child);
	int value = atoi(valueString.c_str());
	if (value < 1) {
		if (reportError) {
			showError(hwnd, CStringUtil::format(
								getString(IDS_INVALID_TIME).c_str(),
								valueString.c_str()));
			SetFocus(child);
		}
		return 0;
	}
	return value;
}

BOOL
CGlobalOptions::doDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM)
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
		}
		break;

	default:
		break;
	}

	return FALSE;
}

BOOL CALLBACK
CGlobalOptions::dlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return s_singleton->doDlgProc(hwnd, message, wParam, lParam);
}
