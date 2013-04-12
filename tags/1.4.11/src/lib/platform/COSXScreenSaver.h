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

#ifndef COSXSCREENSAVER_H
#define COSXSCREENSAVER_H

#include "IScreenSaver.h"
#include <Carbon/Carbon.h>

//! OSX screen saver implementation
class COSXScreenSaver : public IScreenSaver {
public:
	COSXScreenSaver(void* eventTarget);
	virtual ~COSXScreenSaver();

	// IScreenSaver overrides
	virtual void		enable();
	virtual void		disable();
	virtual void		activate();
	virtual void		deactivate();
	virtual bool		isActive() const;
	
private:
	void				processLaunched(ProcessSerialNumber psn);
	void				processTerminated(ProcessSerialNumber psn);
	
	static pascal OSStatus
						launchTerminationCallback(
							EventHandlerCallRef nextHandler,
							EventRef theEvent, void* userData);

private:
	// the target for the events we generate
	void*				m_eventTarget;

	bool				m_enabled;
	void*				m_screenSaverController;
	void*				m_autoReleasePool;
	EventHandlerRef		m_launchTerminationEventHandlerRef;
	ProcessSerialNumber	m_screenSaverPSN;
};

#endif
