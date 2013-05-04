/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#pragma once

#include "IInterface.h"
#include "IApp.h"

class IAppUtil : public IInterface {
public:
	virtual bool parseArg(const int& argc, const char* const* argv, int& i) = 0;
	virtual void adoptApp(IApp* app) = 0;
	virtual IApp& app() const = 0;
	virtual int run(int argc, char** argv) = 0;
	virtual void beforeAppExit() = 0;
	virtual void startNode() = 0;
};
