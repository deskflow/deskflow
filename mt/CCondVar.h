#ifndef CCONDVAR_H
#define CCONDVAR_H

#include "CMutex.h"
#include "BasicTypes.h"

class CStopwatch;

class CCondVarBase {
  public:
	// mutex must be supplied.  all condition variables have an
	// associated mutex.  the copy c'tor uses the same mutex as the
	// argument and is otherwise like the default c'tor.
	CCondVarBase(CMutex* mutex);
	~CCondVarBase();

	// manipulators

	// lock/unlock the mutex.  see CMutex.
	void				lock() const throw();
	void				unlock() const throw();

	// signal the condition.  Signal() wakes one waiting thread.
	// Broadcast() wakes all waiting threads.
	void				signal() throw();
	void				broadcast() throw();

	// accessors

	// wait on the condition.  if timeout < 0 then wait until signalled,
	// otherwise up to timeout seconds or until signalled, whichever
	// comes first.  since clients normally wait on condition variables
	// in a loop, clients can provide a CStopwatch that acts as the
	// timeout clock.  using it, clients don't have to recalculate the
	// timeout on each iteration.  passing a stopwatch with a negative
	// timeout is pointless but permitted.
	//
	// returns true if the object was signalled during the wait, false
	// otherwise.
	//
	// (cancellation point)
	bool				wait(double timeout = -1.0) const;
	bool				wait(CStopwatch&, double timeout) const;

	// get the mutex passed to the c'tor
	CMutex*				getMutex() const throw();

  private:
	void				init();
	void				fini();

	// not implemented
	CCondVarBase(const CCondVarBase&);
	CCondVarBase&		operator=(const CCondVarBase&);

  private:
	CMutex*				m_mutex;
	void*				m_cond;

#if defined(CONFIG_PLATFORM_WIN32)
	enum { kSignal, kBroadcast };
	mutable UInt32		m_waitCount;
	CMutex				m_waitCountMutex;
#endif
};

template <class T>
class CCondVar : public CCondVarBase {
  public:
	CCondVar(CMutex* mutex, const T&);
	CCondVar(const CCondVar&);
	~CCondVar();

	// manipulators

	// assigns the value of the variable
	CCondVar&			operator=(const CCondVar&);

	// assign the value
	CCondVar&			operator=(const T&);

	// accessors

	// get the const value.  this object should be locked before
	// calling this method.
						operator const T&() const throw();

  private:
	T					m_data;
};

template <class T>
inline
CCondVar<T>::CCondVar(CMutex* mutex, const T& data) :
								CCondVarBase(mutex), m_data(data)
{
	// do nothing
}

template <class T>
inline
CCondVar<T>::CCondVar(const CCondVar& cv) :
								CCondVarBase(cv.getMutex()),
								m_data(cv.m_data)
{
	// do nothing
}

template <class T>
inline
CCondVar<T>::~CCondVar()
{
	// do nothing
}

template <class T>
inline
CCondVar<T>&			CCondVar<T>::operator=(const CCondVar<T>& cv)
{
	m_data = cv.m_data;
	return *this;
}

template <class T>
inline
CCondVar<T>&			CCondVar<T>::operator=(const T& data)
{
	m_data = data;
	return *this;
}

template <class T>
inline
CCondVar<T>::operator const T&() const throw()
{
	return m_data;
}


// force instantiation of these common types
template class CCondVar<bool>;
template class CCondVar<SInt32>;

#endif
