#ifndef COUTPUTSTREAMFILTER_H
#define COUTPUTSTREAMFILTER_H

#include "IOutputStream.h"

class COutputStreamFilter : public IOutputStream {
public:
	COutputStreamFilter(IOutputStream*, bool adoptStream = true);
	~COutputStreamFilter();

	// manipulators

	// accessors

	// IOutputStream overrides
	virtual void		close() = 0;
	virtual UInt32		write(const void*, UInt32 count) = 0;
	virtual void		flush() = 0;

protected:
	IOutputStream*		getStream() const;

private:
	IOutputStream*		m_stream;
	bool				m_adopted;
};

#endif
