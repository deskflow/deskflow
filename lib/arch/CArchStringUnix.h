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

#ifndef CARCHSTRINGUNIX_H
#define CARCHSTRINGUNIX_H

#include "IArchString.h"

#define ARCH_STRING CArchStringUnix

//! Unix implementation of IArchString
class CArchStringUnix : public IArchString {
public:
	CArchStringUnix();
	virtual ~CArchStringUnix();

	// IArchString overrides
	virtual int			vsnprintf(char* str,
							int size, const char* fmt, va_list ap);
	virtual CArchMBState	newMBState();
	virtual void		closeMBState(CArchMBState);
	virtual void		initMBState(CArchMBState);
	virtual bool		isInitMBState(CArchMBState);
	virtual int			convMBToWC(wchar_t*, const char*, int, CArchMBState);
	virtual int			convWCToMB(char*, wchar_t, CArchMBState);
	virtual EWideCharEncoding
						getWideCharEncoding();
};

#endif
