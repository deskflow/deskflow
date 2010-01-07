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
#include "CArch.h"

enum EServerState {
	kUninitialized,
	kInitializing,
	kInitializingToStart,
	kInitialized,
	kStarting,
	kStarted
};

class CAppUtil;
class CServer;
class CScreen;
class CClientListener;
class CServerTaskBarReceiver;
class CEventQueueTimer;

class CServerApp : public CApp {
public:
	class CArgs : public CApp::CArgsBase {
	public:
		CArgs();
		~CArgs();

	public:
		CString	m_configFile;
		CNetworkAddress* m_synergyAddress;
		CConfig* m_config;
	};

	CServerApp(CAppUtil* util);
	virtual ~CServerApp();
	
	// Parse server specific command line arguments.
	void parseArgs(int argc, const char* const* argv);

	// Prints help specific to server.
	void help();

	// Returns arguments that are common and for server.
	CArgs& args() const { return (CArgs&)argsBase(); }

	// TODO: Document these functions.
	void reloadSignalHandler(CArch::ESignal, void*);
	void reloadConfig(const CEvent&, void*);
	void loadConfig();
	bool loadConfig(const CString& pathname);
	CEvent::Type getReloadConfigEvent();
	void forceReconnect(const CEvent&, void*);
	CEvent::Type getForceReconnectEvent();
	void resetServer(const CEvent&, void*);
	CEvent::Type getResetServerEvent();
	void cleanupServer();
	void updateStatus();
	int mainLoop();

	CServer* s_server;
	CEvent::Type s_reloadConfigEvent;
	CEvent::Type s_forceReconnectEvent;
	CEvent::Type s_resetServerEvent;
	EServerState s_serverState;
	CScreen* s_serverScreen;
	CPrimaryClient* s_primaryClient;
	CClientListener* s_listener;
	CServerTaskBarReceiver* s_taskBarReceiver;
	bool s_suspended;
	CEventQueueTimer* s_timer;

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

typedef int (*StartupFunc)(int, char**);
