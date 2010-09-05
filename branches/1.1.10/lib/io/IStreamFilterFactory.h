/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

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
