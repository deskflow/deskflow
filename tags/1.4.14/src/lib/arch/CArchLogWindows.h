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

#ifndef CARCHLOGWINDOWS_H
#define CARCHLOGWINDOWS_H

#define WIN32_LEAN_AND_MEAN

#include "IArchLog.h"
#include <windows.h>

#define ARCH_LOG CArchLogWindows

//! Win32 implementation of IArchLog
class CArchLogWindows : public IArchLog {
public:
	CArchLogWindows();
	virtual ~CArchLogWindows();

	// IArchLog overrides
	virtual void		openLog(const char* name);
	virtual void		closeLog();
	virtual void		showLog(bool showIfEmpty);
	virtual void		writeLog(ELevel, const char*);

private:
	HANDLE				m_eventLog;
};

#endif
