#include "CInputStreamFilter.h"
#include <assert.h>

//
// CInputStreamFilter
//

CInputStreamFilter::CInputStreamFilter(IInputStream* stream, bool adopted) :
								m_stream(stream),
								m_adopted(adopted)
{
	assert(m_stream != NULL);
}

CInputStreamFilter::~CInputStreamFilter()
{
	if (m_adopted) {
		delete m_stream;
	}
}

IInputStream*			CInputStreamFilter::getStream() const throw()
{
	return m_stream;
}
