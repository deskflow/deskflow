/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Synergy Si Ltd.
 * Copyright (C) 2012 Nick Bolton
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the OpenSSL
 * library.
 * You must obey the GNU General Public License in all respects for all of
 * the code used other than OpenSSL. If you modify file(s) with this
 * exception, you may extend this exception to your version of the file(s),
 * but you are not obligated to do so. If you do not wish to do so, delete
 * this exception statement from your version. If you delete this exception
 * statement from all source files in the program, then also delete it here.
 */

#pragma once

#include "base/String.h"

class ArgsBase {
public:
	ArgsBase();
	virtual ~ArgsBase();

public:
	bool				m_daemon;
	bool				m_backend;
	bool				m_restartable;
	bool				m_noHooks;
	const char*			m_pname;
	const char*			m_logFilter;
	const char*			m_logFile;
	const char*			m_display;
	String				m_name;
	bool				m_disableTray;
	bool				m_enableIpc;
	bool				m_enableDragDrop;
#if SYSAPI_WIN32
	bool				m_debugServiceWait;
	bool				m_pauseOnExit;
	bool				m_stopOnDeskSwitch;
#endif
#if WINAPI_XWINDOWS
	bool				m_disableXInitThreads;
#endif
	bool				m_shouldExit;
	String				m_synergyAddress;
	bool				m_enableCrypto;
	String				m_profileDirectory;
	String				m_pluginDirectory;
};
