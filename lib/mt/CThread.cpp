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

#include "CThread.h"
#include "XMT.h"
#include "XThread.h"
#include "CLog.h"
#include "IJob.h"
#include "CArch.h"

//
// CThread
//

CThread::CThread(IJob* job)
{
	m_thread = ARCH->newThread(&CThread::threadFunc, job);
	if (m_thread == NULL) {
		// couldn't create thread
		delete job;
		throw XMTThreadUnavailable();
	}
}

CThread::CThread(const CThread& thread)
{
	m_thread = ARCH->copyThread(thread.m_thread);
}

CThread::CThread(CArchThread adoptedThread)
{
	m_thread = adoptedThread;
}

CThread::~CThread()
{
	ARCH->closeThread(m_thread);
}

CThread&
CThread::operator=(const CThread& thread)
{
	// copy given thread and release ours
	CArchThread copy = ARCH->copyThread(thread.m_thread);
	ARCH->closeThread(m_thread);

	// cut over
	m_thread = copy;

	return *this;
}

void
CThread::exit(void* result)
{
	throw XThreadExit(result);
}

void
CThread::cancel()
{
	ARCH->cancelThread(m_thread);
}

void
CThread::setPriority(int n)
{
	ARCH->setPriorityOfThread(m_thread, n);
}

void
CThread::unblockPollSocket()
{
	ARCH->unblockPollSocket(m_thread);
}

CThread
CThread::getCurrentThread()
{
	return CThread(ARCH->newCurrentThread());
}

void
CThread::testCancel()
{
	ARCH->testCancelThread();
}

bool
CThread::wait(double timeout) const
{
	return ARCH->wait(m_thread, timeout);
}

void*
CThread::getResult() const
{
	if (wait())
		return ARCH->getResultOfThread(m_thread);
	else
		return NULL;
}

IArchMultithread::ThreadID
CThread::getID() const
{
	return ARCH->getIDOfThread(m_thread);
}

bool
CThread::operator==(const CThread& thread) const
{
	return ARCH->isSameThread(m_thread, thread.m_thread);
}

bool
CThread::operator!=(const CThread& thread) const
{
	return !ARCH->isSameThread(m_thread, thread.m_thread);
}

void*
CThread::threadFunc(void* vjob)
{
	// get this thread's id for logging
	IArchMultithread::ThreadID id;
	{
		CArchThread thread = ARCH->newCurrentThread();
		id = ARCH->getIDOfThread(thread);
		ARCH->closeThread(thread);
	}

	// get job
	IJob* job = reinterpret_cast<IJob*>(vjob);

	// run job
	void* result = NULL;
	try {
		// go
		LOG((CLOG_DEBUG1 "thread 0x%08x entry", id));
		job->run();
		LOG((CLOG_DEBUG1 "thread 0x%08x exit", id));
	}

	catch (XThreadCancel&) {
		// client called cancel()
		LOG((CLOG_DEBUG1 "caught cancel on thread 0x%08x", id));
		delete job;
		throw;
	}
	catch (XThreadExit& e) {
		// client called exit()
		result = e.m_result;
		LOG((CLOG_DEBUG1 "caught exit on thread 0x%08x, result %p", id, result));
	}
	catch (XBase& e) {
		LOG((CLOG_ERR "exception on thread 0x%08x: %s", id, e.what()));
		delete job;
		throw;
	}
	catch (...) {
		LOG((CLOG_ERR "exception on thread 0x%08x: <unknown>", id));
		delete job;
		throw;
	}

	// done with job
	delete job;

	// return exit result
	return result;
}
