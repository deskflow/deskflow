#ifndef CLOCK_H
#define CLOCK_H

class CMutex;
class CCondVarBase;

//! Mutual exclusion lock utility
/*!
This class locks a mutex or condition variable in the c'tor and unlocks
it in the d'tor.  It's easier and safer than manually locking and
unlocking since unlocking must usually be done no matter how a function
exits (including by unwinding due to an exception).
*/
class CLock {
public:
	//! Lock the mutex \c mutex
	CLock(const CMutex* mutex);
	//! Lock the condition variable \c cv
	CLock(const CCondVarBase* cv);
	//! Unlock the mutex or condition variable
	~CLock();

private:
	// not implemented
	CLock(const CLock&);
	CLock& operator=(const CLock&);

private:
	const CMutex*		m_mutex;
};

#endif
