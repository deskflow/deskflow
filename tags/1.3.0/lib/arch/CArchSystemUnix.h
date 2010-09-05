/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2004 Chris Schoeneman
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

#ifndef CARCHSYSTEMUNIX_H
#define CARCHSYSTEMUNIX_H

#include "IArchSystem.h"

#define ARCH_SYSTEM CArchSystemUnix

//! Unix implementation of IArchString
class CArchSystemUnix : public IArchSystem {
public:
	CArchSystemUnix();
	virtual ~CArchSystemUnix();

	// IArchSystem overrides
	virtual std::string	getOSName() const;
};

#endif
