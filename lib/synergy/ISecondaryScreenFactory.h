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

#ifndef ISECONDARYSCREENFACTORY_H
#define ISECONDARYSCREENFACTORY_H

#include "IInterface.h"

class CSecondaryScreen;
class IScreenReceiver;

//! Secondary screen factory interface
/*!
This interface provides factory methods to create secondary screens.
*/
class ISecondaryScreenFactory : public IInterface {
public:
	//! Create screen
	/*!
	Create and return a secondary screen.  The caller must delete the
	returned object.
	*/
	virtual CSecondaryScreen*
						create(IScreenReceiver*) = 0;
};

#endif
