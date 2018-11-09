/*
 * barrier -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
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
 */

#include "platform/XWindowsScreenSaver.h"

#include "platform/XWindowsUtil.h"
#include "barrier/IPlatformScreen.h"
#include "base/Log.h"
#include "base/Event.h"
#include "base/IEventQueue.h"
#include "base/TMethodEventJob.h"

#include <X11/Xatom.h>
#if HAVE_X11_EXTENSIONS_XTEST_H
#    include <X11/extensions/XTest.h>
#else
#    error The XTest extension is required to build barrier
#endif
#if HAVE_X11_EXTENSIONS_DPMS_H
extern "C" {
#    include <X11/Xmd.h>
#    include <X11/extensions/dpms.h>
#    if !HAVE_DPMS_PROTOTYPES
#        undef DPMSModeOn
#        undef DPMSModeStandby
#        undef DPMSModeSuspend
#        undef DPMSModeOff
#        define DPMSModeOn        0
#        define DPMSModeStandby    1
#        define DPMSModeSuspend    2
#        define DPMSModeOff        3
extern Bool DPMSQueryExtension(Display *, int *, int *);
extern Bool DPMSCapable(Display *);
extern Status DPMSEnable(Display *);
extern Status DPMSDisable(Display *);
extern Status DPMSForceLevel(Display *, CARD16);
extern Status DPMSInfo(Display *, CARD16 *, BOOL *);
#    endif
}
#endif

//
// XWindowsScreenSaver
//

XWindowsScreenSaver::XWindowsScreenSaver(IXWindowsImpl* impl, Display* display,
                                         Window window, void* eventTarget,
                                         IEventQueue* events) :
    m_display(display),
    m_xscreensaverSink(window),
    m_eventTarget(eventTarget),
    m_xscreensaver(None),
    m_xscreensaverActive(false),
    m_dpms(false),
    m_disabled(false),
    m_suppressDisable(false),
    m_disableTimer(NULL),
    m_disablePos(0),
    m_events(events)
{
    m_impl = impl;
    // get atoms
    m_atomScreenSaver = m_impl->XInternAtom(m_display, "SCREENSAVER", False);
    m_atomScreenSaverVersion = m_impl->XInternAtom(m_display,
                                                   "_SCREENSAVER_VERSION",
                                                   False);
    m_atomScreenSaverActivate = m_impl->XInternAtom(m_display, "ACTIVATE",
                                                    False);
    m_atomScreenSaverDeactivate = m_impl->XInternAtom(m_display, "DEACTIVATE",
                                                      False);

    // check for DPMS extension.  this is an alternative screen saver
    // that powers down the display.
#if HAVE_X11_EXTENSIONS_DPMS_H
    int eventBase, errorBase;
    if (m_impl->DPMSQueryExtension(m_display, &eventBase, &errorBase)) {
        if (m_impl->DPMSCapable(m_display)) {
            // we have DPMS
            m_dpms  = true;
        }
    }
#endif

    // watch top-level windows for changes
    bool error = false;
    {
        XWindowsUtil::ErrorLock lock(m_display, &error);
        Window root = m_impl->do_DefaultRootWindow(m_display);
        XWindowAttributes attr;
        m_impl->XGetWindowAttributes(m_display, root, &attr);
        m_rootEventMask = attr.your_event_mask;
        m_impl->XSelectInput(m_display, root,
                             m_rootEventMask | SubstructureNotifyMask);
    }
    if (error) {
        LOG((CLOG_DEBUG "didn't set root event mask"));
        m_rootEventMask = 0;
    }

    // get the built-in settings
    m_impl->XGetScreenSaver(m_display, &m_timeout, &m_interval,
                            &m_preferBlanking, &m_allowExposures);

    // get the DPMS settings
    m_dpmsEnabled = isDPMSEnabled();

    // get the xscreensaver window, if any
    if (!findXScreenSaver()) {
        setXScreenSaver(None);
    }

    // install disable timer event handler
    m_events->adoptHandler(Event::kTimer, this,
                            new TMethodEventJob<XWindowsScreenSaver>(this,
                                &XWindowsScreenSaver::handleDisableTimer));
}

XWindowsScreenSaver::~XWindowsScreenSaver()
{
    // done with disable job
    if (m_disableTimer != NULL) {
        m_events->deleteTimer(m_disableTimer);
    }
    m_events->removeHandler(Event::kTimer, this);

    if (m_display != NULL) {
        enableDPMS(m_dpmsEnabled);
       m_impl->XSetScreenSaver(m_display, m_timeout, m_interval,
                               m_preferBlanking, m_allowExposures);
        clearWatchForXScreenSaver();
        XWindowsUtil::ErrorLock lock(m_display);
        m_impl->XSelectInput(m_display, DefaultRootWindow(m_display),
                             m_rootEventMask);
    }
}

void
XWindowsScreenSaver::destroy()
{
    m_display = NULL;
    delete this;
}

bool
XWindowsScreenSaver::handleXEvent(const XEvent* xevent)
{
    switch (xevent->type) {
    case CreateNotify:
        if (m_xscreensaver == None) {
            if (isXScreenSaver(xevent->xcreatewindow.window)) {
                // found the xscreensaver
                setXScreenSaver(xevent->xcreatewindow.window);
            }
            else {
                // another window to watch.  to detect the xscreensaver
                // window we look for a property but that property may
                // not yet exist by the time we get this event so we
                // have to watch the window for property changes.
                // this would be so much easier if xscreensaver did the
                // smart thing and stored its window in a property on
                // the root window.
                addWatchXScreenSaver(xevent->xcreatewindow.window);
            }
        }
        break;

    case DestroyNotify:
        if (xevent->xdestroywindow.window == m_xscreensaver) {
            // xscreensaver is gone
            LOG((CLOG_DEBUG "xscreensaver died"));
            setXScreenSaver(None);
            return true;
        }
        break;

    case PropertyNotify:
        if (xevent->xproperty.state == PropertyNewValue) {
            if (isXScreenSaver(xevent->xproperty.window)) {
                // found the xscreensaver
                setXScreenSaver(xevent->xcreatewindow.window);
            }
        }
        break;

    case MapNotify:
        if (xevent->xmap.window == m_xscreensaver) {
            // xscreensaver has activated
            setXScreenSaverActive(true);
            return true;
        }
        break;

    case UnmapNotify:
        if (xevent->xunmap.window == m_xscreensaver) {
            // xscreensaver has deactivated
            setXScreenSaverActive(false);
            return true;
        }
        break;
    }

    return false;
}

void
XWindowsScreenSaver::enable()
{
    // for xscreensaver
    m_disabled = false;
    updateDisableTimer();

    // for built-in X screen saver
    m_impl->XSetScreenSaver(m_display, m_timeout, m_interval, m_preferBlanking,
                            m_allowExposures);

    // for DPMS
    enableDPMS(m_dpmsEnabled);
}

void
XWindowsScreenSaver::disable()
{
    // for xscreensaver
    m_disabled = true;
    updateDisableTimer();

    // use built-in X screen saver
    m_impl->XGetScreenSaver(m_display, &m_timeout, &m_interval,
                            &m_preferBlanking, &m_allowExposures);
    m_impl->XSetScreenSaver(m_display, 0, m_interval, m_preferBlanking,
                            m_allowExposures);

    // for DPMS
    m_dpmsEnabled = isDPMSEnabled();
    enableDPMS(false);

    // FIXME -- now deactivate?
}

void
XWindowsScreenSaver::activate()
{
    // remove disable job timer
    m_suppressDisable = true;
    updateDisableTimer();

    // enable DPMS if it was enabled
    enableDPMS(m_dpmsEnabled);

    // try xscreensaver
    findXScreenSaver();
    if (m_xscreensaver != None) {
        sendXScreenSaverCommand(m_atomScreenSaverActivate);
        return;
    }

    // try built-in X screen saver
    if (m_timeout != 0) {
        m_impl->XForceScreenSaver(m_display, ScreenSaverActive);
    }

    // try DPMS
    activateDPMS(true);
}

void
XWindowsScreenSaver::deactivate()
{
    // reinstall disable job timer
    m_suppressDisable = false;
    updateDisableTimer();

    // try DPMS
    activateDPMS(false);

    // disable DPMS if screen saver is disabled
    if (m_disabled) {
        enableDPMS(false);
    }

    // try xscreensaver
    findXScreenSaver();
    if (m_xscreensaver != None) {
        sendXScreenSaverCommand(m_atomScreenSaverDeactivate);
        return;
    }

    // use built-in X screen saver
    m_impl->XForceScreenSaver(m_display, ScreenSaverReset);
}

bool
XWindowsScreenSaver::isActive() const
{
    // check xscreensaver
    if (m_xscreensaver != None) {
        return m_xscreensaverActive;
    }

    // check DPMS
    if (isDPMSActivated()) {
        return true;
    }

    // can't check built-in X screen saver activity
    return false;
}

bool
XWindowsScreenSaver::findXScreenSaver()
{
    // do nothing if we've already got the xscreensaver window
    if (m_xscreensaver == None) {
        // find top-level window xscreensaver window
        Window root = DefaultRootWindow(m_display);
        Window rw, pw, *cw;
        unsigned int nc;
        if (m_impl->XQueryTree(m_display, root, &rw, &pw, &cw, &nc)) {
            for (unsigned int i = 0; i < nc; ++i) {
                if (isXScreenSaver(cw[i])) {
                    setXScreenSaver(cw[i]);
                    break;
                }
            }
            m_impl->XFree(cw);
        }
    }

    return (m_xscreensaver != None);
}

void
XWindowsScreenSaver::setXScreenSaver(Window window)
{
    LOG((CLOG_DEBUG "xscreensaver window: 0x%08x", window));

    // save window
    m_xscreensaver = window;

    if (m_xscreensaver != None) {
        // clear old watch list
        clearWatchForXScreenSaver();

        // see if xscreensaver is active
        bool error = false;
        XWindowAttributes attr;
        {
            XWindowsUtil::ErrorLock lock(m_display, &error);
            m_impl->XGetWindowAttributes(m_display, m_xscreensaver, &attr);
        }
        setXScreenSaverActive(!error && attr.map_state != IsUnmapped);

        // save current DPMS state;  xscreensaver may have changed it.
        m_dpmsEnabled = isDPMSEnabled();
    }
    else {
        // screen saver can't be active if it doesn't exist
        setXScreenSaverActive(false);

        // start watching for xscreensaver
        watchForXScreenSaver();
    }
}

bool
XWindowsScreenSaver::isXScreenSaver(Window w) const
{
    // check for m_atomScreenSaverVersion string property
    Atom type;
    return (XWindowsUtil::getWindowProperty(m_display, w,
                                    m_atomScreenSaverVersion,
                                    NULL, &type, NULL, False) &&
                                type == XA_STRING);
}

void
XWindowsScreenSaver::setXScreenSaverActive(bool activated)
{
    if (m_xscreensaverActive != activated) {
        LOG((CLOG_DEBUG "xscreensaver %s on window 0x%08x", activated ? "activated" : "deactivated", m_xscreensaver));
        m_xscreensaverActive = activated;

        // if screen saver was activated forcefully (i.e. against
        // our will) then just accept it.  don't try to keep it
        // from activating since that'll just pop up the password
        // dialog if locking is enabled.
        m_suppressDisable = activated;
        updateDisableTimer();

        if (activated) {
            m_events->addEvent(Event(
                            m_events->forIPrimaryScreen().screensaverActivated(),
                            m_eventTarget));
        }
        else {
            m_events->addEvent(Event(
                            m_events->forIPrimaryScreen().screensaverDeactivated(),
                            m_eventTarget));
        }
    }
}

void
XWindowsScreenSaver::sendXScreenSaverCommand(Atom cmd, long arg1, long arg2)
{
    XEvent event;
    event.xclient.type         = ClientMessage;
    event.xclient.display      = m_display;
    event.xclient.window       = m_xscreensaverSink;
    event.xclient.message_type = m_atomScreenSaver;
    event.xclient.format       = 32;
    event.xclient.data.l[0]    = static_cast<long>(cmd);
    event.xclient.data.l[1]    = arg1;
    event.xclient.data.l[2]    = arg2;
    event.xclient.data.l[3]    = 0;
    event.xclient.data.l[4]    = 0;

    LOG((CLOG_DEBUG "send xscreensaver command: %d %d %d", (long)cmd, arg1, arg2));
    bool error = false;
    {
        XWindowsUtil::ErrorLock lock(m_display, &error);
        m_impl->XSendEvent(m_display, m_xscreensaver, False, 0, &event);
    }
    if (error) {
        findXScreenSaver();
    }
}

void
XWindowsScreenSaver::watchForXScreenSaver()
{
    // clear old watch list
    clearWatchForXScreenSaver();

    // add every child of the root to the list of windows to watch
    Window root = DefaultRootWindow(m_display);
    Window rw, pw, *cw;
    unsigned int nc;
    if (m_impl->XQueryTree(m_display, root, &rw, &pw, &cw, &nc)) {
        for (unsigned int i = 0; i < nc; ++i) {
            addWatchXScreenSaver(cw[i]);
        }
        m_impl->XFree(cw);
    }

    // now check for xscreensaver window in case it set the property
    // before we could request property change events.
    if (findXScreenSaver()) {
        // found it so clear out our watch list
        clearWatchForXScreenSaver();
    }
}

void
XWindowsScreenSaver::clearWatchForXScreenSaver()
{
    // stop watching all windows
    XWindowsUtil::ErrorLock lock(m_display);
    for (WatchList::iterator index = m_watchWindows.begin();
                                index != m_watchWindows.end(); ++index) {
        m_impl->XSelectInput(m_display, index->first, index->second);
    }
    m_watchWindows.clear();
}

void
XWindowsScreenSaver::addWatchXScreenSaver(Window window)
{
    // get window attributes
    bool error = false;
    XWindowAttributes attr;
    {
        XWindowsUtil::ErrorLock lock(m_display, &error);
        m_impl->XGetWindowAttributes(m_display, window, &attr);
    }

    // if successful and window uses override_redirect (like xscreensaver
    // does) then watch it for property changes.  
    if (!error && attr.override_redirect == True) {
        error = false;
        {
            XWindowsUtil::ErrorLock lock(m_display, &error);
            m_impl->XSelectInput(m_display, window,
                                 attr.your_event_mask | PropertyChangeMask);
        }
        if (!error) {
            // if successful then add the window to our list
            m_watchWindows.insert(std::make_pair(window, attr.your_event_mask));
        }
    }
}

void
XWindowsScreenSaver::updateDisableTimer()
{
    if (m_disabled && !m_suppressDisable && m_disableTimer == NULL) {
        // 5 seconds should be plenty often to suppress the screen saver
        m_disableTimer = m_events->newTimer(5.0, this);
    }
    else if ((!m_disabled || m_suppressDisable) && m_disableTimer != NULL) {
        m_events->deleteTimer(m_disableTimer);
        m_disableTimer = NULL;
    }
}

void
XWindowsScreenSaver::handleDisableTimer(const Event&, void*)
{
    // send fake mouse motion directly to xscreensaver
    if (m_xscreensaver != None) {
        XEvent event;
        event.xmotion.type         = MotionNotify;
        event.xmotion.display      = m_display;
        event.xmotion.window       = m_xscreensaver;
        event.xmotion.root         = DefaultRootWindow(m_display);
        event.xmotion.subwindow    = None;
        event.xmotion.time         = CurrentTime;
        event.xmotion.x            = m_disablePos;
        event.xmotion.y            = 0;
        event.xmotion.x_root       = m_disablePos;
        event.xmotion.y_root       = 0;
        event.xmotion.state        = 0;
        event.xmotion.is_hint      = NotifyNormal;
        event.xmotion.same_screen  = True;

        XWindowsUtil::ErrorLock lock(m_display);
        m_impl->XSendEvent(m_display, m_xscreensaver, False, 0, &event);

        m_disablePos = 20 - m_disablePos;
    }
}

void
XWindowsScreenSaver::activateDPMS(bool activate)
{
#if HAVE_X11_EXTENSIONS_DPMS_H
    if (m_dpms) {
        // DPMSForceLevel will generate a BadMatch if DPMS is disabled
        XWindowsUtil::ErrorLock lock(m_display);
        m_impl->DPMSForceLevel(m_display,
                               activate ? DPMSModeStandby : DPMSModeOn);
    }
#endif
}

void
XWindowsScreenSaver::enableDPMS(bool enable)
{
#if HAVE_X11_EXTENSIONS_DPMS_H
    if (m_dpms) {
        if (enable) {
            m_impl->DPMSEnable(m_display);
        }
        else {
            m_impl->DPMSDisable(m_display);
        }
    }
#endif
}

bool
XWindowsScreenSaver::isDPMSEnabled() const
{
#if HAVE_X11_EXTENSIONS_DPMS_H
    if (m_dpms) {
        CARD16 level;
        BOOL state;
        m_impl->DPMSInfo(m_display, &level, &state);
        return (state != False);
    }
    else {
        return false;
    }
#else
    return false;
#endif
}

bool
XWindowsScreenSaver::isDPMSActivated() const
{
#if HAVE_X11_EXTENSIONS_DPMS_H
    if (m_dpms) {
        CARD16 level;
        BOOL state;
        m_impl->DPMSInfo(m_display, &level, &state);
        return (level != DPMSModeOn);
    }
    else {
        return false;
    }
#else
    return false;
#endif
}
