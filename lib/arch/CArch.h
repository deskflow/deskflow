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
#include "IArchSystem.h"
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
				public IArchSystem,
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
							const char* dependencies,
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
	virtual bool		isSameThread(CArchThread, CArchThread);
	virtual bool		isExitedThread(CArchThread);
	virtual void*		getResultOfThread(CArchThread);
	virtual ThreadID	getIDOfThread(CArchThread);
	virtual void		setSignalHandler(ESignal, SignalFunc, void*);
	virtual void		raiseSignal(ESignal);

	// IArchNetwork overrides
	virtual CArchSocket	newSocket(EAddressFamily, ESocketType);
	virtual CArchSocket	copySocket(CArchSocket s);
	virtual void		closeSocket(CArchSocket s);
	virtual void		closeSocketForRead(CArchSocket s);
	virtual void		closeSocketForWrite(CArchSocket s);
	virtual void		bindSocket(CArchSocket s, CArchNetAddress addr);
	virtual void		listenOnSocket(CArchSocket s);
	virtual CArchSocket	acceptSocket(CArchSocket s, CArchNetAddress* addr);
	virtual bool		connectSocket(CArchSocket s, CArchNetAddress name);
	virtual int			pollSocket(CPollEntry[], int num, double timeout);
	virtual void		unblockPollSocket(CArchThread thread);
	virtual size_t		readSocket(CArchSocket s, void* buf, size_t len);
	virtual size_t		writeSocket(CArchSocket s,
							const void* buf, size_t len);
	virtual void		throwErrorOnSocket(CArchSocket);
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
	virtual bool			isEqualAddr(CArchNetAddress, CArchNetAddress);

	// IArchSleep overrides
	virtual void		sleep(double timeout);

	// IArchString overrides
	virtual int			vsnprintf(char* str,
							int size, const char* fmt, va_list ap);
	virtual int			convStringMBToWC(wchar_t*,
							const char*, UInt32 n, bool* errors);
	virtual int			convStringWCToMB(char*,
							const wchar_t*, UInt32 n, bool* errors);
	virtual EWideCharEncoding
						getWideCharEncoding();

	// IArchSystem overrides
	virtual std::string	getOSName() const;

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
	IArchSystem*		m_system;
	IArchTaskBar*		m_taskbar;
	IArchTime*			m_time;
};

//! Convenience object to lock/unlock an arch mutex
class CArchMutexLock {
public:
	CArchMutexLock(CArchMutex mutex) : m_mutex(mutex)
	{
		ARCH->lockMutex(m_mutex);
	}
	~CArchMutexLock()
	{
		ARCH->unlockMutex(m_mutex);
	}

private:
	CArchMutex			m_mutex;
};

#endif
