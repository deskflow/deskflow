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

#ifndef CARCHTASKBARXWINDOWS_H
#define CARCHTASKBARXWINDOWS_H

#include "IArchTaskBar.h"

#define ARCH_TASKBAR CArchTaskBarXWindows

//! X11 implementation of IArchTaskBar
class CArchTaskBarXWindows : public IArchTaskBar {
public:
	CArchTaskBarXWindows();
	virtual ~CArchTaskBarXWindows();

	// IArchTaskBar overrides
	virtual void		addReceiver(IArchTaskBarReceiver*);
	virtual void		removeReceiver(IArchTaskBarReceiver*);
	virtual void		updateReceiver(IArchTaskBarReceiver*);
};

#endif
