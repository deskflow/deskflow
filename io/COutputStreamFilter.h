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
	virtual void		close() throw(XIO) = 0;
	virtual UInt32		write(const void*, UInt32 count) throw(XIO) = 0;
	virtual void		flush() throw(XIO) = 0;

  protected:
	IOutputStream*		getStream() const throw();

  private:
	IOutputStream*		m_stream;
	bool				m_adopted;
};

#endif
