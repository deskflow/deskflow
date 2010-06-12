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
#include "IArchMultithread.h"

enum EServerState {
	kUninitialized,
	kInitializing,
	kInitializingToStart,
	kInitialized,
	kStarting,
	kStarted
};

class CServer;
class CScreen;
class CClientListener;
class CEventQueueTimer;
class ILogOutputter;

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

	CServerApp(CreateTaskBarReceiverFunc createTaskBarReceiver);
	virtual ~CServerApp();
	
	// Parse server specific command line arguments.
	void parseArgs(int argc, const char* const* argv);

	// Prints help specific to server.
	void help();

	// Returns arguments that are common and for server.
	CArgs& args() const { return (CArgs&)argsBase(); }

	const char* daemonName() const;
	const char* daemonInfo() const;

	// TODO: Document these functions.
	static void reloadSignalHandler(CArch::ESignal, void*);
	static CEvent::Type getReloadConfigEvent();

	void reloadConfig(const CEvent&, void*);
	void loadConfig();
	bool loadConfig(const CString& pathname);
	void forceReconnect(const CEvent&, void*);
	CEvent::Type getForceReconnectEvent();
	void resetServer(const CEvent&, void*);
	CEvent::Type getResetServerEvent();
	void handleClientConnected(const CEvent&, void* vlistener);
	void handleClientsDisconnected(const CEvent&, void*);
	void closeServer(CServer* server);
	void stopRetryTimer();
	void updateStatus();
	void updateStatus(const CString& msg);
	void closeClientListener(CClientListener* listen);
	void stopServer();
	void closePrimaryClient(CPrimaryClient* primaryClient);
	void closeServerScreen(CScreen* screen);
	void cleanupServer();
	bool initServer();
	void retryHandler(const CEvent&, void*);
	CScreen* openServerScreen();
	CScreen* createScreen();
	CPrimaryClient* openPrimaryClient(const CString& name, CScreen* screen);
	void handleScreenError(const CEvent&, void*);
	void handleSuspend(const CEvent&, void*);
	void handleResume(const CEvent&, void*);
	CClientListener* openClientListener(const CNetworkAddress& address);
	CServer* openServer(const CConfig& config, CPrimaryClient* primaryClient);
	void handleNoClients(const CEvent&, void*);
	bool startServer();
	int mainLoop();
	int runInner(int argc, char** argv, ILogOutputter* outputter, StartupFunc startup);
	int standardStartup(int argc, char** argv);
	int foregroundStartup(int argc, char** argv);
	void startNode();

	static CServerApp& instance() { return (CServerApp&)CApp::instance(); }

	// TODO: change s_ to m_
	CServer* s_server;
	static CEvent::Type s_reloadConfigEvent;
	CEvent::Type s_forceReconnectEvent;
	CEvent::Type s_resetServerEvent;
	EServerState s_serverState;
	CScreen* s_serverScreen;
	CPrimaryClient* s_primaryClient;
	CClientListener* s_listener;
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
