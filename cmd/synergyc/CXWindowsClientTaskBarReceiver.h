/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2003 Chris Schoeneman
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

#ifndef CXWINDOWSCLIENTTASKBARRECEIVER_H
#define CXWINDOWSCLIENTTASKBARRECEIVER_H

#include "CClientTaskBarReceiver.h"

class CBufferedLogOutputter;

//! Implementation of CClientTaskBarReceiver for X Windows
class CXWindowsClientTaskBarReceiver : public CClientTaskBarReceiver {
public:
	CXWindowsClientTaskBarReceiver(const CBufferedLogOutputter*);
	virtual ~CXWindowsClientTaskBarReceiver();

	// IArchTaskBarReceiver overrides
	virtual void		showStatus();
	virtual void		runMenu(int x, int y);
	virtual void		primaryAction();
	virtual const Icon	getIcon() const;
};

#endif
