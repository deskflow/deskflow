#ifndef ISTREAMFILTERFACTORY_H
#define ISTREAMFILTERFACTORY_H

#include "IInterface.h"

class CInputStreamFilter;
class COutputStreamFilter;
class IInputStream;
class IOutputStream;

//! Stream filter factory interface
/*!
This interface provides factory methods to create stream filters.
*/
class IStreamFilterFactory : public IInterface {
public:
	//! Create input filter
	/*!
	Create and return an input stream filter.  The caller must delete the
	returned object.
	*/
	virtual CInputStreamFilter*
						createInput(IInputStream*, bool adoptStream) = 0;

	//! Create output filter
	/*!
	Create and return an output stream filter.  The caller must delete the
	returned object.
	*/
	virtual COutputStreamFilter*
						createOutput(IOutputStream*, bool adoptStream) = 0;
};

#endif
