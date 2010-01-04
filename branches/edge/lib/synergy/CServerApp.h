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

#pragma once

#include "CApp.h"
#include "CString.h"
#include "CConfig.h"
#include "CNetworkAddress.h"

class CAppUtil;

class CServerApp : public CApp {
public:
	class CArgs : public CApp::CArgsBase {
	public:
		CArgs();
		~CArgs();

	public:
		bool				m_backend;
		bool				m_restartable;
		bool				m_daemon;
		CString		 		m_configFile;
		const char* 		m_logFilter;
		const char*			m_logFile;
		const char*			m_display;
		CString 			m_name;
		CNetworkAddress*	m_synergyAddress;
		CConfig*			m_config;
	};

	CServerApp(CAppUtil* util);
	virtual ~CServerApp();
	
	void parse(int argc, const char* const* argv);
	void help();
	void version();
	CArgs& args() const { return (CArgs&)argsBase(); }

private:
	virtual bool parseArg(const int& argc, const char* const* argv, int& i);
};

// configuration file name
#if SYSAPI_WIN32
#define USR_CONFIG_NAME "synergy.sgc"
#define SYS_CONFIG_NAME "synergy.sgc"
#elif SYSAPI_UNIX
#define USR_CONFIG_NAME ".synergy.conf"
#define SYS_CONFIG_NAME "synergy.conf"
#endif
