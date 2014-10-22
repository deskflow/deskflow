/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2014 Synergy Si, Inc.
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
 
#pragma once

#include "base/String.h"
#include "common/stdvector.h"

class CArgsBase;

class CArgParser {

public:
	bool				parsePlatformArg(CArgsBase& argsBase, const int& argc, const char* const* argv, int& i);

	static	bool		isArg(int argi, int argc, const char* const* argv,
							const char* name1, const char* name2,
							int minRequiredParameters = 0);
private:
	static CArgsBase&	argsBase() { return *m_argsBase; }

private:
	static CArgsBase*	m_argsBase;
};
