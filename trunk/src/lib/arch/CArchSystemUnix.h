/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
	virtual std::string getPlatformName() const;
	virtual std::string setting(const std::string&) const;
	virtual void setting(const std::string&, const std::string&) const;
};

#endif
