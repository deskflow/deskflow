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

#ifndef IARCHMULTITHREAD_H
#define IARCHMULTITHREAD_H

#include "IInterface.h"

class CArchCondImpl;
class CArchMutexImpl;
class CArchThreadImpl;
typedef CArchCondImpl* CArchCond;
typedef CArchMutexImpl* CArchMutex;
typedef CArchThreadImpl* CArchThread;

//! Interface for architecture dependent multithreading
/*!
This interface defines the multithreading operations required by
synergy.  Each architecture must implement this interface.
*/
class IArchMultithread : public IInterface {
public:
	typedef void* (*ThreadFunc)(void*);
	typedef unsigned int ThreadID;

	//! @name manipulators
	//@{

	//
	// condition variable methods
	//

	virtual CArchCond	newCondVar() = 0;
	virtual void		closeCondVar(CArchCond) = 0;
	virtual void		signalCondVar(CArchCond) = 0;
	virtual void		broadcastCondVar(CArchCond) = 0;
	virtual bool		waitCondVar(CArchCond, CArchMutex, double timeout) = 0;

	//
	// mutex methods
	//

	virtual CArchMutex	newMutex() = 0;
	virtual void		closeMutex(CArchMutex) = 0;
	virtual void		lockMutex(CArchMutex) = 0;
	virtual void		unlockMutex(CArchMutex) = 0;

	//
	// thread methods
	//

	virtual CArchThread	newThread(ThreadFunc, void*) = 0;
	virtual CArchThread	newCurrentThread() = 0;
	virtual CArchThread	copyThread(CArchThread) = 0;
	virtual void		closeThread(CArchThread) = 0;
	virtual void		cancelThread(CArchThread) = 0;
	virtual void		setPriorityOfThread(CArchThread, int n) = 0;
	virtual void		testCancelThread() = 0;
	virtual bool		wait(CArchThread, double timeout) = 0;
	virtual bool		waitForEvent(double timeout) = 0;
	virtual bool		isSameThread(CArchThread, CArchThread) = 0;
	virtual bool		isExitedThread(CArchThread) = 0;
	virtual void*		getResultOfThread(CArchThread) = 0;
	virtual ThreadID	getIDOfThread(CArchThread) = 0;

	//@}
};

#endif
