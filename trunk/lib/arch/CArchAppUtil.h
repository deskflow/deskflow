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

#include "IArchAppUtil.h"
#include "XSynergy.h"

class CArchAppUtil : public IArchAppUtil {
public:
	CArchAppUtil();
	virtual ~CArchAppUtil();
	
	virtual bool parseArg(const int& argc, const char* const* argv, int& i);
	virtual void adoptApp(CApp* app);
	CApp& app() const;
	virtual void exitApp(int code) { throw XExitApp(code); }

	static CArchAppUtil& instance();
	static void exitAppStatic(int code) { instance().exitApp(code); }
	virtual void beforeAppExit() {}
	
private:
	CApp* m_app;
	static CArchAppUtil* s_instance;
};
