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

#ifndef CARCHDAEMONUNIX_H
#define CARCHDAEMONUNIX_H

#include "CArchDaemonNone.h"

#undef ARCH_DAEMON
#define ARCH_DAEMON CArchDaemonUnix

//! Unix implementation of IArchDaemon
class CArchDaemonUnix : public CArchDaemonNone {
public:
	CArchDaemonUnix();
	virtual ~CArchDaemonUnix();

	// IArchDaemon overrides
	virtual int			daemonize(const char* name, DaemonFunc func);
};

#endif
