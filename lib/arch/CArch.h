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

#ifndef CARCH_H
#define CARCH_H

#include "IArchConsole.h"
#include "IArchDaemon.h"
#include "IArchFile.h"
#include "IArchLog.h"
#include "IArchMultithread.h"
#include "IArchNetwork.h"
#include "IArchSleep.h"
#include "IArchString.h"
#include "IArchTaskBar.h"
#include "IArchTime.h"

/*!
\def ARCH
This macro evaluates to the singleton CArch object.
*/
#define ARCH	(CArch::getInstance())

#define ARCH_ARGS void

//! Delegating mplementation of architecture dependent interfaces
/*!
This class is a centralized interface to all architecture dependent
interface implementations (except miscellaneous functions).  It
instantiates an implementation of each interface and delegates calls
to each method to those implementations.  Clients should use the
\c ARCH macro to access this object.  Clients must also instantiate
exactly one of these objects before attempting to call any method,
typically at the beginning of \c main().
*/
class CArch : public IArchConsole,
				public IArchDaemon,
				public IArchFile,
				public IArchLog,
				public IArchMultithread,
				public IArchNetwork,
				public IArchSleep,
				public IArchString,
				public IArchTaskBar,
				public IArchTime {
public:
	CArch(ARCH_ARGS* args = NULL);
	~CArch();

	//
	// accessors
	//

	//! Return the singleton instance
	/*!
	The client must have instantiated exactly once CArch object before
	calling this function.
	*/
	static CArch*		getInstance();

	// IArchConsole overrides
	virtual void		openConsole(const char*);
	virtual void		closeConsole();
	virtual void		writeConsole(const char*);
	virtual const char*	getNewlineForConsole();

	// IArchDaemon overrides
	virtual void		installDaemon(const char* name,
							const char* description,
							const char* pathname,
							const char* commandLine,
							bool allUsers);
	virtual void		uninstallDaemon(const char* name, bool allUsers);
	virtual int			daemonize(const char* name, DaemonFunc func);
	virtual bool		canInstallDaemon(const char* name, bool allUsers);
	virtual bool		isDaemonInstalled(const char* name, bool allUsers);

	// IArchFile overrides
	virtual const char*	getBasename(const char* pathname);
	virtual std::string	getUserDirectory();
	virtual std::string	getSystemDirectory();
	virtual std::string	concatPath(const std::string& prefix,
							const std::string& suffix);

	// IArchLog overrides
	virtual void		openLog(const char*);
	virtual void		closeLog();
	virtual void		writeLog(ELevel, const char*);

	// IArchMultithread overrides
	virtual CArchCond	newCondVar();
	virtual void		closeCondVar(CArchCond);
	virtual void		signalCondVar(CArchCond);
	virtual void		broadcastCondVar(CArchCond);
	virtual bool		waitCondVar(CArchCond, CArchMutex, double timeout);
	virtual CArchMutex	newMutex();
	virtual void		closeMutex(CArchMutex);
	virtual void		lockMutex(CArchMutex);
	virtual void		unlockMutex(CArchMutex);
	virtual CArchThread	newThread(ThreadFunc, void*);
	virtual CArchThread	newCurrentThread();
	virtual CArchThread	copyThread(CArchThread);
	virtual void		closeThread(CArchThread);
	virtual void		cancelThread(CArchThread);
	virtual void		setPriorityOfThread(CArchThread, int n);
	virtual void		testCancelThread();
	virtual bool		wait(CArchThread, double timeout);
	virtual EWaitResult	waitForEvent(CArchThread, double timeout);
	virtual bool		isSameThread(CArchThread, CArchThread);
	virtual bool		isExitedThread(CArchThread);
	virtual void*		getResultOfThread(CArchThread);
	virtual ThreadID	getIDOfThread(CArchThread);

	// IArchNetwork overrides
	virtual CArchSocket	newSocket(EAddressFamily, ESocketType);
	virtual CArchSocket	copySocket(CArchSocket s);
	virtual void		closeSocket(CArchSocket s);
	virtual void		closeSocketForRead(CArchSocket s);
	virtual void		closeSocketForWrite(CArchSocket s);
	virtual void		bindSocket(CArchSocket s, CArchNetAddress addr);
	virtual void		listenOnSocket(CArchSocket s);
	virtual CArchSocket	acceptSocket(CArchSocket s, CArchNetAddress* addr);
	virtual void		connectSocket(CArchSocket s, CArchNetAddress name);
	virtual int			pollSocket(CPollEntry[], int num, double timeout);
	virtual size_t		readSocket(CArchSocket s, void* buf, size_t len);
	virtual size_t		writeSocket(CArchSocket s,
							const void* buf, size_t len);
	virtual void		throwErrorOnSocket(CArchSocket);
	virtual bool		setBlockingOnSocket(CArchSocket, bool blocking);
	virtual bool		setNoDelayOnSocket(CArchSocket, bool noDelay);
	virtual std::string		getHostName();
	virtual CArchNetAddress	newAnyAddr(EAddressFamily);
	virtual CArchNetAddress	copyAddr(CArchNetAddress);
	virtual CArchNetAddress	nameToAddr(const std::string&);
	virtual void			closeAddr(CArchNetAddress);
	virtual std::string		addrToName(CArchNetAddress);
	virtual std::string		addrToString(CArchNetAddress);
	virtual EAddressFamily	getAddrFamily(CArchNetAddress);
	virtual void			setAddrPort(CArchNetAddress, int port);
	virtual int				getAddrPort(CArchNetAddress);
	virtual bool			isAnyAddr(CArchNetAddress);

	// IArchSleep overrides
	virtual void		sleep(double timeout);

	// IArchString overrides
	virtual int			vsnprintf(char* str,
							int size, const char* fmt, va_list ap);
	virtual CArchMBState	newMBState();
	virtual void		closeMBState(CArchMBState);
	virtual void		initMBState(CArchMBState);
	virtual bool		isInitMBState(CArchMBState);
	virtual int			convMBToWC(wchar_t*, const char*, int, CArchMBState);
	virtual int			convWCToMB(char*, wchar_t, CArchMBState);
	virtual EWideCharEncoding
						getWideCharEncoding();

	// IArchTaskBar
	virtual void		addReceiver(IArchTaskBarReceiver*);
	virtual void		removeReceiver(IArchTaskBarReceiver*);
	virtual void		updateReceiver(IArchTaskBarReceiver*);

	// IArchTime overrides
	virtual double		time();

private:
	static CArch*		s_instance;

	IArchConsole*		m_console;
	IArchDaemon*		m_daemon;
	IArchFile*			m_file;
	IArchLog*			m_log;
	IArchMultithread*	m_mt;
	IArchNetwork*		m_net;
	IArchSleep*			m_sleep;
	IArchString*		m_string;
	IArchTaskBar*		m_taskbar;
	IArchTime*			m_time;
};

#endif
