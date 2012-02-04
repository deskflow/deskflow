/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
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

#ifndef CARCHFILEUNIX_H
#define CARCHFILEUNIX_H

#include "IArchFile.h"

#define ARCH_FILE CArchFileUnix

//! Unix implementation of IArchFile
class CArchFileUnix : public IArchFile {
public:
	CArchFileUnix();
	virtual ~CArchFileUnix();

	// IArchFile overrides
	virtual const char*	getBasename(const char* pathname);
	virtual std::string	getUserDirectory();
	virtual std::string	getSystemDirectory();
	virtual std::string	concatPath(const std::string& prefix,
							const std::string& suffix);
};

#endif
