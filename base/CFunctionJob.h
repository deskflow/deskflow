#ifndef CFUNCTIONJOB_H
#define CFUNCTIONJOB_H

#include "IJob.h"

//! Use a function as a job
/*!
A job class that invokes a function.
*/
class CFunctionJob : public IJob {
public:
	//! run() invokes \c func(arg)
	CFunctionJob(void (*func)(void*), void* arg = NULL);
	virtual ~CFunctionJob();

	// IJob overrides
	virtual void		run();

private:
	void				(*m_func)(void*);
	void*				m_arg;
};

#endif
