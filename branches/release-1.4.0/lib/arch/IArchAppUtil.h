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
 
#pragma once

#include "IInterface.h"

// TODO: replace with forward declaration if possible
// we need to decouple these classes!
#include "CApp.h"

class IArchAppUtil : public IInterface {
public:
	virtual bool parseArg(const int& argc, const char* const* argv, int& i) = 0;
	virtual void adoptApp(CApp* app) = 0;
	virtual CApp& app() const = 0;
	virtual int run(int argc, char** argv, CreateTaskBarReceiverFunc createTaskBarReceiver) = 0;
	virtual void beforeAppExit() = 0;
	virtual void startNode() = 0;
};
