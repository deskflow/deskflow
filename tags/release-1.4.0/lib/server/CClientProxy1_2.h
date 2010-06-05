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

#ifndef CCLIENTPROXY1_2_H
#define CCLIENTPROXY1_2_H

#include "CClientProxy1_1.h"

//! Proxy for client implementing protocol version 1.2
class CClientProxy1_2 : public CClientProxy1_1 {
public:
	CClientProxy1_2(const CString& name, IStream* adoptedStream);
	~CClientProxy1_2();

	// IClient overrides
	virtual void		mouseRelativeMove(SInt32 xRel, SInt32 yRel);
};

#endif
