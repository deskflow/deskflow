/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2003 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
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

#ifndef CXWINDOWSSERVERTASKBARRECEIVER_H
#define CXWINDOWSSERVERTASKBARRECEIVER_H

#include "CServerTaskBarReceiver.h"

class CBufferedLogOutputter;

//! Implementation of CServerTaskBarReceiver for X Windows
class CXWindowsServerTaskBarReceiver : public CServerTaskBarReceiver {
public:
	CXWindowsServerTaskBarReceiver(const CBufferedLogOutputter*);
	virtual ~CXWindowsServerTaskBarReceiver();

	// IArchTaskBarReceiver overrides
	virtual void		showStatus();
	virtual void		runMenu(int x, int y);
	virtual void		primaryAction();
	virtual const Icon	getIcon() const;
};

#endif
