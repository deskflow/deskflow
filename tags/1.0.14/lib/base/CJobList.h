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

#ifndef CJOBLIST_H
#define CJOBLIST_H

#include "IArchMultithread.h"
#include "stdlist.h"

class IJob;

class CJobList {
public:
	CJobList();
	~CJobList();

	//! @name manipulators
	//@{

	//! Add a job
	/*!
	Add a job to the list.  The client keeps ownership of the job.
	Jobs can be safely added while \c runJobs() is executing.
	*/
	void				addJob(IJob*);

	//! Remove a job
	/*!
	Remove a job from the list.  The client keeps ownership of the job.
	Jobs can be safely removed while \c runJobs() is executing.
	*/
	void				removeJob(IJob*);

	//@}
	//! @name accessors
	//@{

	//! Run all jobs
	/*!
	Run all jobs in the list.  Any number of threads can call
	\c runJobs() at once.  Jobs can be added and removed while
	\c runJobs() is executing in the same or another thread.
	Any job added after \c runJobs() starts will not be run
	by that call to runJobs().  Destroying a removed job
	while \c runJobs() is executing is not safe unless the
	removed completed before \c runJobs() started.
	*/
	void				runJobs() const;

	//@}

private:
	typedef std::list<IJob*> CJobs;
	typedef CJobs::iterator iterator;

	CArchMutex			m_mutex;
	mutable CJobs		m_jobs;
};

#endif

