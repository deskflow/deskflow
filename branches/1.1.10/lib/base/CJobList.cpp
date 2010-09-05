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

#include "CJobList.h"
#include "IJob.h"
#include "CArch.h"

//
// CJobList
//

CJobList::CJobList()
{
	m_mutex = ARCH->newMutex();
}

CJobList::~CJobList()
{
	ARCH->closeMutex(m_mutex);
}

void
CJobList::addJob(IJob* job)
{
	// ignore bogus job
	if (job == NULL) {
		return;
	}

	// make a temporary list with the job.  later we'll splice this
	// list into our jobs list.  since splice() never throws we
	// don't have to try/catch to unlock the mutex.
	CJobs tmpList;
	tmpList.push_front(job);

	// add job to list
	ARCH->lockMutex(m_mutex);
	m_jobs.splice(m_jobs.begin(), tmpList);
	ARCH->unlockMutex(m_mutex);
}

void
CJobList::removeJob(IJob* job)
{
	if (job != NULL) {
		ARCH->lockMutex(m_mutex);
		m_jobs.remove(job);
		ARCH->unlockMutex(m_mutex);
	}
}

void
CJobList::runJobs() const
{
	// this is a little tricky.  to allow any number of threads to
	// traverse the list while jobs are added and removed while
	// not holding the mutex when running a job (so other threads
	// or the running job can add and remove jobs), we insert a
	// new element into the list.  this element has a NULL job and
	// is a "safe place" while we traverse the list.  the safe place
	// is inserted at the start of the list (with the mutex locked)
	// then, for each job, we lock the mutex and move the safe place
	// past the next job, unlock the mutex, run the job and repeat
	// until there are no more jobs.  the safe place will not be
	// removed by any other thread and is after every job that has
	// been run and before every job that still needs to run.  when
	// all the jobs have been run we remove the safe place.

	// add the safe place
	CJobs tmpList;
	tmpList.push_front(NULL);
	ARCH->lockMutex(m_mutex);
	m_jobs.splice(m_jobs.begin(), tmpList);
	CJobs::iterator safePlace = m_jobs.begin();

	// find the next non-NULL job (NULL jobs are safe places)
	CJobs::iterator next = safePlace;
	while (next != m_jobs.end() && *next == NULL)
		++next;
	while (next != m_jobs.end()) {
		// found a job.  run it without holding a lock.  note the
		// race condition here:  we release the lock, allowing
		// removeJob() to remove this job before we run the job.
		// therefore the caller cannot safely destroy the job
		// until all runJobs() complete.
		IJob* job = *next;
		++next;
		m_jobs.splice(next, m_jobs, safePlace);
		ARCH->unlockMutex(m_mutex);
		job->run();

		// next real job
		ARCH->lockMutex(m_mutex);
		next = safePlace;
		while (next != m_jobs.end() && *next == NULL)
			++next;
	}

	// remove the safe place
	m_jobs.erase(safePlace);
	ARCH->unlockMutex(m_mutex);
}
