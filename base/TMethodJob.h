#ifndef CMETHODJOB_H
#define CMETHODJOB_H

#include "IJob.h"

template <class T>
class TMethodJob : public IJob {
  public:
	TMethodJob(T* object, void (T::*method)(void*), void* arg = NULL);

	// IJob overrides
	virtual void		run();

  private:
	T*					m_object;
	void				(T::*m_method)(void*);
	void*				m_arg;
};

template <class T>
inline
TMethodJob<T>::TMethodJob(T* object, void (T::*method)(void*), void* arg) :
								m_object(object),
								m_method(method),
								m_arg(arg)
{
	// do nothing
}

template <class T>
inline
void					TMethodJob<T>::run()
{
	if (m_object != NULL) {
		(m_object->*m_method)(m_arg);
	}
}

#endif
