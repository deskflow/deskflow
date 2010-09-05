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

#ifndef IPRIMARYSCREENFACTORY_H
#define IPRIMARYSCREENFACTORY_H

#include "IInterface.h"

class CPrimaryScreen;
class IPrimaryScreenReceiver;
class IScreenReceiver;

//! Primary screen factory interface
/*!
This interface provides factory methods to create primary screens.
*/
class IPrimaryScreenFactory : public IInterface {
public:
	//! Create screen
	/*!
	Create and return a primary screen.  The caller must delete the
	returned object.
	*/
	virtual CPrimaryScreen*
						create(IScreenReceiver*, IPrimaryScreenReceiver*) = 0;
};

#endif
