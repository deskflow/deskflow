#ifndef CLOCK_H
#define CLOCK_H

class CMutex;
class CCondVarBase;

class CLock {
public:
	CLock(const CMutex* mutex);
	CLock(const CCondVarBase* cv);
	~CLock();

private:
	// not implemented
	CLock(const CLock&);
	CLock& operator=(const CLock&);

private:
	const CMutex*		m_mutex;
};

#endif
