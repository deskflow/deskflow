/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Synergy Si Ltd.
 * Copyright (C) 2002 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the OpenSSL
 * library.
 * You must obey the GNU General Public License in all respects for all of
 * the code used other than OpenSSL. If you modify file(s) with this
 * exception, you may extend this exception to your version of the file(s),
 * but you are not obligated to do so. If you do not wish to do so, delete
 * this exception statement from your version. If you delete this exception
 * statement from all source files in the program, then also delete it here.
 */

#pragma once

#include "synergy/IScreenSaver.h"

#include <Carbon/Carbon.h>

class IEventQueue;

//! OSX screen saver implementation
class OSXScreenSaver : public IScreenSaver {
public:
	OSXScreenSaver(IEventQueue* events, void* eventTarget);
	virtual ~OSXScreenSaver();

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
    IEventQueue*        m_events;
};
