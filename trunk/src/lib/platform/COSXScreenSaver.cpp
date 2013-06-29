/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#import "COSXScreenSaver.h"
#import "COSXScreenSaverUtil.h"
#import "CLog.h"
#import "IEventQueue.h"
#import "IPrimaryScreen.h"
#import <string.h>

//
// COSXScreenSaver
//

COSXScreenSaver::COSXScreenSaver(IEventQueue* events, void* eventTarget) :
	m_events(events),
	m_eventTarget(eventTarget),
	m_enabled(true)
{
	m_autoReleasePool       = screenSaverUtilCreatePool();
	m_screenSaverController = screenSaverUtilCreateController();

	// install launch/termination event handlers
	EventTypeSpec launchEventTypes[2];
	launchEventTypes[0].eventClass = kEventClassApplication;
	launchEventTypes[0].eventKind  = kEventAppLaunched;
	launchEventTypes[1].eventClass = kEventClassApplication;
	launchEventTypes[1].eventKind  = kEventAppTerminated;
	
	EventHandlerUPP launchTerminationEventHandler =
		NewEventHandlerUPP(launchTerminationCallback);
	InstallApplicationEventHandler(launchTerminationEventHandler, 2,
								launchEventTypes, this,
								&m_launchTerminationEventHandlerRef);
	DisposeEventHandlerUPP(launchTerminationEventHandler);
	
	m_screenSaverPSN.highLongOfPSN = 0;
	m_screenSaverPSN.lowLongOfPSN  = 0;

	// test if screensaver is running and find process number
	if (isActive()) {
		ProcessInfoRec	procInfo;
		Str31			procName;	// pascal string. first byte holds length.
		memset(&procInfo, 0, sizeof(procInfo));
		procInfo.processName       = procName;
		procInfo.processInfoLength = sizeof(ProcessInfoRec);

		ProcessSerialNumber	psn;
		OSErr err = GetNextProcess(&psn);
		while (err == 0) {
			memset(procName, 0, sizeof(procName));
			err = GetProcessInformation(&psn, &procInfo);
			if (err != 0) {
				break;
			}
			if (strcmp("ScreenSaverEngine", (const char*)&procName[1]) == 0) {
				m_screenSaverPSN = psn;
				break;
			}
			err = GetNextProcess(&psn);
		}
	}
}

COSXScreenSaver::~COSXScreenSaver()
{
	RemoveEventHandler(m_launchTerminationEventHandlerRef);
//	screenSaverUtilReleaseController(m_screenSaverController);
	screenSaverUtilReleasePool(m_autoReleasePool);
}

void
COSXScreenSaver::enable()
{
	m_enabled = true;
	screenSaverUtilEnable(m_screenSaverController);
}

void
COSXScreenSaver::disable()
{
	m_enabled = false;
	screenSaverUtilDisable(m_screenSaverController);
}

void
COSXScreenSaver::activate()
{
	screenSaverUtilActivate(m_screenSaverController);
}

void
COSXScreenSaver::deactivate()
{
	screenSaverUtilDeactivate(m_screenSaverController, m_enabled);
}

bool
COSXScreenSaver::isActive() const
{
	return (screenSaverUtilIsActive(m_screenSaverController) != 0);
}

void
COSXScreenSaver::processLaunched(ProcessSerialNumber psn)
{
	CFStringRef	processName;
	OSStatus	err = CopyProcessName(&psn, &processName);
	
	if (err == 0 && CFEqual(CFSTR("ScreenSaverEngine"), processName)) {
		m_screenSaverPSN = psn;
		LOG((CLOG_DEBUG1 "ScreenSaverEngine launched. Enabled=%d", m_enabled));
		if (m_enabled) {
			m_events->addEvent(
				CEvent(m_events->forIPrimaryScreen().screensaverActivated(),
					m_eventTarget));
		}
	}
}

void
COSXScreenSaver::processTerminated(ProcessSerialNumber psn)
{
	if (m_screenSaverPSN.highLongOfPSN == psn.highLongOfPSN &&
		m_screenSaverPSN.lowLongOfPSN  == psn.lowLongOfPSN) {
		LOG((CLOG_DEBUG1 "ScreenSaverEngine terminated. Enabled=%d", m_enabled));
		if (m_enabled) {
			m_events->addEvent(
				CEvent(m_events->forIPrimaryScreen().screensaverDeactivated(),
					m_eventTarget));
		}
		
		m_screenSaverPSN.highLongOfPSN = 0;
		m_screenSaverPSN.lowLongOfPSN  = 0;
	}
}

pascal OSStatus
COSXScreenSaver::launchTerminationCallback(
				EventHandlerCallRef nextHandler,
				EventRef theEvent, void* userData)
{
	OSStatus		result;
    ProcessSerialNumber psn; 
    EventParamType	actualType;
    UInt32			actualSize;

    result = GetEventParameter(theEvent, kEventParamProcessID,
							   typeProcessSerialNumber, &actualType,
							   sizeof(psn), &actualSize, &psn);

	if ((result == noErr) &&
		(actualSize > 0) &&
		(actualType == typeProcessSerialNumber)) {
		COSXScreenSaver* screenSaver = (COSXScreenSaver*)userData;
		UInt32 eventKind = GetEventKind(theEvent);
		if (eventKind == kEventAppLaunched) {
			screenSaver->processLaunched(psn);
		}
		else if (eventKind == kEventAppTerminated) {
			screenSaver->processTerminated(psn);
		}
	}
    return (CallNextEventHandler(nextHandler, theEvent));
}
