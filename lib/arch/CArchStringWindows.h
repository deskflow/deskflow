/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman
 * 
 * This package is free software you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef CARCHSTRINGWINDOWS_H
#define CARCHSTRINGWINDOWS_H

#include "IArchString.h"

#define ARCH_STRING CArchStringWindows

//! Win32 implementation of IArchString
class CArchStringWindows : public IArchString {
public:
	CArchStringWindows();
	virtual ~CArchStringWindows();

	// IArchString overrides
	virtual int			vsnprintf(char* str,
							int size, const char* fmt, va_list ap);
	virtual int			convStringMBToWC(wchar_t*,
							const char*, UInt32 n, bool* errors);
	virtual int			convStringWCToMB(char*,
							const wchar_t*, UInt32 n, bool* errors);
	virtual EWideCharEncoding
						getWideCharEncoding();
};

#endif
