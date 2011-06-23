/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CARCHMULTITHREADWINDOWS_H
#define CARCHMULTITHREADWINDOWS_H

#define WIN32_LEAN_AND_MEAN

#include "IArchMultithread.h"
#include "stdlist.h"
#include <windows.h>

#define ARCH_MULTITHREAD CArchMultithreadWindows

class CArchCondImpl {
public:
	enum { kSignal = 0, kBroadcast };

	HANDLE				m_events[2];
	mutable int			m_waitCount;
	CArchMutex			m_waitCountMutex;
};

class CArchMutexImpl {
public:
	CRITICAL_SECTION	m_mutex;
};

//! Win32 implementation of IArchMultithread
class CArchMultithreadWindows : public IArchMultithread {
public:
	CArchMultithreadWindows();
	virtual ~CArchMultithreadWindows();

	//! @name manipulators
	//@{

	void				setNetworkDataForCurrentThread(void*);

	//@}
	//! @name accessors
	//@{

	HANDLE				getCancelEventForCurrentThread();

	void*				getNetworkDataForThread(CArchThread);

	static CArchMultithreadWindows*	getInstance();

	//@}

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

private:
	CArchThreadImpl*	find(DWORD id);
	CArchThreadImpl*	findNoRef(DWORD id);
	CArchThreadImpl*	findNoRefOrCreate(DWORD id);
	void				insert(CArchThreadImpl* thread);
	void				erase(CArchThreadImpl* thread);

	void				refThread(CArchThreadImpl* rep);
	void				testCancelThreadImpl(CArchThreadImpl* rep);

	void				doThreadFunc(CArchThread thread);
	static unsigned int __stdcall	threadFunc(void* vrep);

private:
	typedef std::list<CArchThread> CThreadList;

	static CArchMultithreadWindows*	s_instance;

	CArchMutex			m_threadMutex;

	CThreadList			m_threadList;
	CArchThread			m_mainThread;

	SignalFunc			m_signalFunc[kNUM_SIGNALS];
	void*				m_signalUserData[kNUM_SIGNALS];
};

#endif
