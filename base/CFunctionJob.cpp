#include "CFunctionJob.h"

//
// CFunctionJob
//

CFunctionJob::CFunctionJob(void (*func)(void*), void* arg) :
	m_func(func),
	m_arg(arg)
{
	// do nothing
}

CFunctionJob::~CFunctionJob()
{
	// do nothing
}

void
CFunctionJob::run()
{
	if (m_func != NULL) {
		m_func(m_arg);
	}
}
