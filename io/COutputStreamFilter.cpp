#include "COutputStreamFilter.h"

//
// COutputStreamFilter
//

COutputStreamFilter::COutputStreamFilter(
	IOutputStream* stream,
	bool adopted) :
	m_stream(stream),
	m_adopted(adopted)
{
	assert(m_stream != NULL);
}

COutputStreamFilter::~COutputStreamFilter()
{
	if (m_adopted) {
		delete m_stream;
	}
}

IOutputStream*
COutputStreamFilter::getStream() const
{
	return m_stream;
}
