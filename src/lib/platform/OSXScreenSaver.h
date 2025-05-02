/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/IScreenSaver.h"

#include <Carbon/Carbon.h>

class IEventQueue;

//! OSX screen saver implementation
class OSXScreenSaver : public IScreenSaver
{
public:
  OSXScreenSaver(IEventQueue *events, void *eventTarget);
  ~OSXScreenSaver();

  // IScreenSaver overrides
  void enable() override;
  void disable() override;
  void activate() override;
  void deactivate() override;
  bool isActive() const override;

private:
  void processLaunched(ProcessSerialNumber psn);
  void processTerminated(ProcessSerialNumber psn);

  static pascal OSStatus launchTerminationCallback(EventHandlerCallRef nextHandler, EventRef theEvent, void *userData);

private:
  // the target for the events we generate
  void *m_eventTarget;

  bool m_enabled;
  void *m_screenSaverController;
  void *m_autoReleasePool;
  EventHandlerRef m_launchTerminationEventHandlerRef;
  ProcessSerialNumber m_screenSaverPSN;
  IEventQueue *m_events;
};
