/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2004 Chris Schoeneman
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

#include "CSocketMultiplexer.h"
#include "ISocketMultiplexerJob.h"
#include "CCondVar.h"
#include "CLock.h"
#include "CMutex.h"
#include "CThread.h"
#include "CLog.h"
#include "TMethodJob.h"
#include "CArch.h"
#include "XArch.h"
#include "stdvector.h"

//
// CSocketMultiplexer
//

CSocketMultiplexer*		CSocketMultiplexer::s_instance = NULL;

CSocketMultiplexer::CSocketMultiplexer() :
	m_mutex(new CMutex),
	m_pollable(new CCondVar<bool>(m_mutex, false)),
	m_polling(new CCondVar<bool>(m_mutex, false)),
	m_thread(NULL),
	m_update(false)
{
	assert(s_instance == NULL);

	// this pointer just has to be unique and not NULL.  it will
	// never be dereferenced.  it's used to identify cursor nodes
	// in the jobs list.
	m_cursorMark = reinterpret_cast<ISocketMultiplexerJob*>(this);

	// start thread
	m_thread = new CThread(new TMethodJob<CSocketMultiplexer>(
								this, &CSocketMultiplexer::serviceThread));

	s_instance = this;
}

CSocketMultiplexer::~CSocketMultiplexer()
{
	m_thread->cancel();
	m_thread->unblockPollSocket();
	m_thread->wait();
	delete m_thread;
	delete m_polling;
	delete m_pollable;
	delete m_mutex;

	// clean up jobs
	for (CSocketJobMap::iterator i = m_socketJobMap.begin();
						i != m_socketJobMap.end(); ++i) {
		delete *(i->second);
	}

	s_instance = NULL;
}

CSocketMultiplexer*
CSocketMultiplexer::getInstance()
{
	return s_instance;
}

void
CSocketMultiplexer::addSocket(ISocket* socket, ISocketMultiplexerJob* job)
{
	assert(socket != NULL);
	assert(job    != NULL);

	CLock lock(m_mutex);

	// prevent service thread from restarting poll
	*m_pollable = false;

	// break thread out of poll
	m_thread->unblockPollSocket();

	// wait for poll to finish
	while (*m_polling) {
		m_polling->wait();
	}

	// insert/replace job
	CSocketJobMap::iterator i = m_socketJobMap.find(socket);
	if (i == m_socketJobMap.end()) {
		// we *must* put the job at the end so the order of jobs in
		// the list continue to match the order of jobs in pfds in
		// serviceThread().
		CJobCursor j = m_socketJobs.insert(m_socketJobs.end(), job);
		m_update     = true;
		m_socketJobMap.insert(std::make_pair(socket, j));
	}
	else {
		CJobCursor j = i->second;
		if (*j != job) {
			delete *j;
			*j = job;
		}
		m_update = true;
	}

	// there must be at least one socket so we can poll now
	*m_pollable = true;
	m_pollable->broadcast();
}

void
CSocketMultiplexer::removeSocket(ISocket* socket)
{
	assert(socket != NULL);

	CLock lock(m_mutex);

	// prevent service thread from restarting poll
	*m_pollable = false;

	// break thread out of poll
	m_thread->unblockPollSocket();

	// wait until thread finishes poll
	while (*m_polling) {
		m_polling->wait();
	}

	// remove job.  rather than removing it from the map we put NULL
	// in the list instead so the order of jobs in the list continues
	// to match the order of jobs in pfds in serviceThread().
	CSocketJobMap::iterator i = m_socketJobMap.find(socket);
	if (i != m_socketJobMap.end()) {
		if (*(i->second) != NULL) {
			delete *(i->second);
			*(i->second) = NULL;
			m_update     = true;
		}
	}

	*m_pollable = true;
	m_pollable->broadcast();
}

void
CSocketMultiplexer::serviceThread(void*)
{
	std::vector<IArchNetwork::CPollEntry> pfds;
	IArchNetwork::CPollEntry pfd;

	// service the connections
	for (;;) {
		CThread::testCancel();
		{
			CLock lock(m_mutex);

			// wait until pollable
			while (!(bool)*m_pollable) {
				m_pollable->wait();
			}

			// now we're polling
			*m_polling = true;
			m_polling->broadcast();
		}

		// we're now the only thread that can access m_sockets and
		// m_update because m_polling and m_pollable are both true.
		// therefore, we don't need to hold the mutex.

		// collect poll entries
		if (m_update) {
			m_update = false;
			pfds.clear();
			pfds.reserve(m_socketJobMap.size());

			CJobCursor cursor    = newCursor();
			CJobCursor jobCursor = nextCursor(cursor);
			while (jobCursor != m_socketJobs.end()) {
				ISocketMultiplexerJob* job = *jobCursor;
				if (job != NULL) {
					pfd.m_socket = job->getSocket();
					pfd.m_events = 0;
					if (job->isReadable()) {
						pfd.m_events |= IArchNetwork::kPOLLIN;
					}
					if (job->isWritable()) {
						pfd.m_events |= IArchNetwork::kPOLLOUT;
					}
					pfds.push_back(pfd);
				}				
				jobCursor = nextCursor(cursor);
			}
			deleteCursor(cursor);
		}

		int status;
		try {
			// check for status
			status = ARCH->pollSocket(&pfds[0], pfds.size(), -1);
		}
		catch (XArchNetwork& e) {
			LOG((CLOG_WARN "error in socket multiplexer: %s", e.what().c_str()));
			status = 0;
		}

		if (status != 0) {
			// iterate over socket jobs, invoking each and saving the
			// new job.
			UInt32 i             = 0;
			CJobCursor cursor    = newCursor();
			CJobCursor jobCursor = nextCursor(cursor);
			while (i < pfds.size() && jobCursor != m_socketJobs.end()) {
				if (*jobCursor != NULL) {
					// get poll state
					unsigned short revents = pfds[i].m_revents;
					bool read  = ((revents & IArchNetwork::kPOLLIN) != 0);
					bool write = ((revents & IArchNetwork::kPOLLOUT) != 0);
					bool error = ((revents & (IArchNetwork::kPOLLERR |
											  IArchNetwork::kPOLLNVAL)) != 0);

					// run job
					ISocketMultiplexerJob* job    = *jobCursor;
					ISocketMultiplexerJob* newJob = job->run(read, write, error);

					// save job, if different
					if (newJob != job) {
						CLock lock(m_mutex);
						delete job;
						*jobCursor = newJob;
						m_update   = true;
					}
					++i;
				}

				// next job
				jobCursor = nextCursor(cursor);
			}
			deleteCursor(cursor);
		}

		// delete any removed socket jobs
		CLock lock(m_mutex);
		for (CSocketJobMap::iterator i = m_socketJobMap.begin();
							i != m_socketJobMap.end();) {
			if (*(i->second) == NULL) {
				m_socketJobMap.erase(i++);
				m_update = true;
			}
			else {
				++i;
			}
		}
		if (m_socketJobMap.empty()) {
			*m_pollable = false;
		}

		// done polling
		*m_polling = false;
		m_polling->broadcast();
	}
}

CSocketMultiplexer::CJobCursor
CSocketMultiplexer::newCursor()
{
	CLock lock(m_mutex);
	return m_socketJobs.insert(m_socketJobs.begin(), m_cursorMark);
}

CSocketMultiplexer::CJobCursor
CSocketMultiplexer::nextCursor(CJobCursor cursor)
{
	CLock lock(m_mutex);
	CJobCursor j = m_socketJobs.end();
	CJobCursor i = cursor;
	while (++i != m_socketJobs.end()) {
		if (*i != m_cursorMark) {
			// found a real job (as opposed to a cursor)
			j = i;

			// move our cursor just past the job
			m_socketJobs.splice(++i, m_socketJobs, cursor);
			break;
		}
	}
	return j;
}

void
CSocketMultiplexer::deleteCursor(CJobCursor cursor)
{
	CLock lock(m_mutex);
	m_socketJobs.erase(cursor);
}
