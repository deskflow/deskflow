#ifndef TMETHODJOB_H
#define	TMETHODJOB_H

#include "IJob.h"

template <class T>
class TMethodJob : public IJob {
  public:
	typedef void (T::*Method)();

	TMethodJob(T* object, Method method) :
								m_object(object), m_method(method) { }
	virtual ~TMethodJob() { }

	// IJob overrides
	virtual void		run() { (m_object->*m_method)(); }

  private:
	T*					m_object;
	Method				m_method;
};

#endif
