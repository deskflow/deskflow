#ifndef CMUTEX_H
#define CMUTEX_H

#include "common.h"

// recursive mutex class
class CMutex {
public:
	// copy c'tor is equivalent to default c'tor.  it's here to
	// allow copying of objects that have mutexes.
	CMutex();
	CMutex(const CMutex&);
	~CMutex();

	// manipulators

	// this has no effect.  it's only here to allow assignment of
	// objects that have mutexes.
	CMutex&				operator=(const CMutex&);

	// accessors

	void				lock() const;
	void				unlock() const;

private:
	void				init();
	void				fini();

private:
	friend class CCondVarBase;
	void*				m_mutex;
};

#endif
