/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/IEventQueue.h"
#include "common/stdmap.h"
#include "deskflow/IScreenSaver.h"

#if X_DISPLAY_MISSING
#error X11 is required to build deskflow
#else
#include <X11/Xlib.h>
#endif

class Event;
class EventQueueTimer;

//! X11 screen saver implementation
class XWindowsScreenSaver : public IScreenSaver
{
public:
  XWindowsScreenSaver(Display *, Window, void *eventTarget, IEventQueue *events);
  XWindowsScreenSaver(XWindowsScreenSaver const &) = delete;
  XWindowsScreenSaver(XWindowsScreenSaver &&) = delete;
  virtual ~XWindowsScreenSaver();

  XWindowsScreenSaver &operator=(XWindowsScreenSaver const &) = delete;
  XWindowsScreenSaver &operator=(XWindowsScreenSaver &&) = delete;

  //! @name manipulators
  //@{

  //! Event filtering
  /*!
  Should be called for each system event before event translation and
  dispatch.  Returns true to skip translation and dispatch.
  */
  bool handleXEvent(const XEvent *);

  //! Destroy without the display
  /*!
  Tells this object to delete itself without using the X11 display.
  It may leak some resources as a result.
  */
  void destroy();

  //@}

  // IScreenSaver overrides
  virtual void enable();
  virtual void disable();
  virtual void activate();
  virtual void deactivate();
  virtual bool isActive() const;

private:
  // find and set the running xscreensaver's window.  returns true iff
  // found.
  bool findXScreenSaver();

  // set the xscreensaver's window, updating the activation state flag
  void setXScreenSaver(Window);

  // returns true if the window appears to be the xscreensaver window
  bool isXScreenSaver(Window) const;

  // set xscreensaver's activation state flag.  sends notification
  // if the state has changed.
  void setXScreenSaverActive(bool activated);

  // send a command to xscreensaver
  void sendXScreenSaverCommand(Atom, long = 0, long = 0);

  // watch all windows that could potentially be the xscreensaver for
  // the events that will confirm it.
  void watchForXScreenSaver();

  // stop watching all watched windows
  void clearWatchForXScreenSaver();

  // add window to the watch list
  void addWatchXScreenSaver(Window window);

  // install/uninstall the job used to suppress the screensaver
  void updateDisableTimer();

  // called periodically to prevent the screen saver from starting
  void handleDisableTimer(const Event &, void *);

  // force DPMS to activate or deactivate
  void activateDPMS(bool activate);

  // enable/disable DPMS screen saver
  void enableDPMS(bool);

  // check if DPMS is enabled
  bool isDPMSEnabled() const;

  // check if DPMS is activate
  bool isDPMSActivated() const;

private:
  using WatchList = std::map<Window, long>;

  // the X display
  Display *m_display;

  // window to receive xscreensaver responses
  Window m_xscreensaverSink;

  // the target for the events we generate
  void *m_eventTarget;

  // xscreensaver's window
  Window m_xscreensaver;

  // xscreensaver activation state
  bool m_xscreensaverActive;

  // old event mask on root window
  long m_rootEventMask;

  // potential xscreensaver windows being watched
  WatchList m_watchWindows;

  // atoms used to communicate with xscreensaver's window
  Atom m_atomScreenSaver;
  Atom m_atomScreenSaverVersion;
  Atom m_atomScreenSaverActivate;
  Atom m_atomScreenSaverDeactivate;

  // built-in screen saver settings
  int m_timeout;
  int m_interval;
  int m_preferBlanking;
  int m_allowExposures;

  // DPMS screen saver settings
  bool m_dpms;
  bool m_dpmsEnabled;

  // true iff the client wants the screen saver suppressed
  bool m_disabled;

  // true iff we're ignoring m_disabled.  this is true, for example,
  // when the client has called activate() and so presumably wants
  // to activate the screen saver even if disabled.
  bool m_suppressDisable;

  // the disable timer (NULL if not installed)
  EventQueueTimer *m_disableTimer;

  // fake mouse motion position for suppressing the screen saver.
  // xscreensaver since 2.21 requires the mouse to move more than 10
  // pixels to be considered significant.
  int32_t m_disablePos;

  IEventQueue *m_events;
};
