/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CFUNCTIONEVENTJOB_H
#define CFUNCTIONEVENTJOB_H

#include "IEventJob.h"

//! Use a function as an event job
/*!
An event job class that invokes a function.
*/
class CFunctionEventJob : public IEventJob {
public:
	//! run() invokes \c func(arg)
	CFunctionEventJob(void (*func)(const CEvent&, void*), void* arg = NULL);
	virtual ~CFunctionEventJob();

	// IEventJob overrides
	virtual void		run(const CEvent&);

private:
	void				(*m_func)(const CEvent&, void*);
	void*				m_arg;
};

#endif
