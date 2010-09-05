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

#ifndef COSXCLIENTTASKBARRECEIVER_H
#define COSXCLIENTTASKBARRECEIVER_H

#include "CClientTaskBarReceiver.h"

class CBufferedLogOutputter;

//! Implementation of CClientTaskBarReceiver for OS X
class COSXClientTaskBarReceiver : public CClientTaskBarReceiver {
public:
	COSXClientTaskBarReceiver(const CBufferedLogOutputter*);
	virtual ~COSXClientTaskBarReceiver();

	// IArchTaskBarReceiver overrides
	virtual void		showStatus();
	virtual void		runMenu(int x, int y);
	virtual void		primaryAction();
	virtual const Icon	getIcon() const;
};

#endif
