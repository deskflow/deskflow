#ifndef CINPUTSTREAMFILTER_H
#define CINPUTSTREAMFILTER_H

#include "IInputStream.h"

class CInputStreamFilter : public IInputStream {
public:
	CInputStreamFilter(IInputStream*, bool adoptStream = true);
	~CInputStreamFilter();

	// manipulators

	// accessors

	// IInputStream overrides
	virtual void		close() = 0;
	virtual UInt32		read(void*, UInt32 maxCount) = 0;
	virtual UInt32		getSize() const = 0;

protected:
	IInputStream*		getStream() const;

private:
	IInputStream*		m_stream;
	bool				m_adopted;
};

#endif
