/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2003 Chris Schoeneman
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

#ifndef ISCREENFACTORY_H
#define ISCREENFACTORY_H

#include "IInterface.h"

class IPrimaryScreenReceiver;
class IPlatformScreen;
class IScreenReceiver;

//! Primary screen factory interface
/*!
This interface provides factory methods to create primary and
secondary screens.
*/
class IScreenFactory : public IInterface {
public:
	//! Create screen
	/*!
	Create and return a screen.  The caller must delete the returned
	object.  The screen is a primary screen iff the IPrimaryScreenReceiver
	is not NULL.
	*/
	virtual IPlatformScreen*
						create(IScreenReceiver*, IPrimaryScreenReceiver*) = 0;
};

#endif
