#ifndef XTHREAD_H
#define XTHREAD_H

#include "common.h"

// generic thread exception
class XThread { };

// thrown by CThread::Exit() to exit a thread.  clients of CThread
// must not throw this type but must rethrow it if caught (by
// XThreadExit, XThread, or ...).
class XThreadExit : public XThread {
public:
	XThreadExit(void* result) : m_result(result) { }
	~XThreadExit() { }

public:
	void*				m_result;
};

// thrown to cancel a thread.  clients must not throw this type, but
// must rethrow it if caught (by XThreadCancel, XThread, or ...).
class XThreadCancel : public XThread { };

// convenience macro to rethrow an XThread exception but ignore other
// exceptions.  put this in your catch (...) handler after necessary
// cleanup but before leaving or returning from the handler.
#define RETHROW_XTHREAD \
	try { throw; } catch (XThread&) { throw; } catch (...) { }

#endif
