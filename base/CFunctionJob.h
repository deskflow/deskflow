#ifndef CFUNCTIONJOB_H
#define CFUNCTIONJOB_H

#include "IJob.h"

class CFunctionJob : public IJob {
  public:
	CFunctionJob(void (*func)(void*), void* arg = NULL);

	// IJob overrides
	virtual void		run();

  private:
	void				(*m_func)(void*);
	void*				m_arg;
};

#endif
