#ifndef CLOCK_H
#define CLOCK_H

#include "common.h"

class CMutex;
class CCondVarBase;

class CLock {
  public:
	CLock(const CMutex* mutex) throw();
	CLock(const CCondVarBase* cv) throw();
	~CLock() throw();

  private:
	// not implemented
	CLock(const CLock&);
	CLock& operator=(const CLock&);

  private:
	const CMutex*		m_mutex;
};

#endif
