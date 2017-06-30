/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2004 Chris Schoeneman
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

#pragma once

#include "synergy/IPlatformScreen.h"
#include "synergy/DragInformation.h"
#include "common/stdexcept.h"

//! Base screen implementation
/*!
This screen implementation is the superclass of all other screen
implementations.  It implements a handful of methods and requires
subclasses to implement the rest.
*/
class PlatformScreen : public IPlatformScreen {
public:
    PlatformScreen (IEventQueue* events);
    virtual ~PlatformScreen ();

    // IScreen overrides
    virtual void* getEventTarget () const = 0;
    virtual bool getClipboard (ClipboardID id, IClipboard*) const = 0;
    virtual void
    getShape (SInt32& x, SInt32& y, SInt32& width, SInt32& height) const = 0;
    virtual void getCursorPos (SInt32& x, SInt32& y) const = 0;

    // IPrimaryScreen overrides
    virtual void reconfigure (UInt32 activeSides) = 0;
    virtual void warpCursor (SInt32 x, SInt32 y)                    = 0;
    virtual UInt32 registerHotKey (KeyID key, KeyModifierMask mask) = 0;
    virtual void unregisterHotKey (UInt32 id)                  = 0;
    virtual void fakeInputBegin ()                             = 0;
    virtual void fakeInputEnd ()                               = 0;
    virtual SInt32 getJumpZoneSize () const                    = 0;
    virtual bool isAnyMouseButtonDown (UInt32& buttonID) const = 0;
    virtual void getCursorCenter (SInt32& x, SInt32& y) const = 0;

    // ISecondaryScreen overrides
    virtual void fakeMouseButton (ButtonID id, bool press)           = 0;
    virtual void fakeMouseMove (SInt32 x, SInt32 y)                  = 0;
    virtual void fakeMouseRelativeMove (SInt32 dx, SInt32 dy) const  = 0;
    virtual void fakeMouseWheel (SInt32 xDelta, SInt32 yDelta) const = 0;

    // IKeyState overrides
    virtual void updateKeyMap ();
    virtual void updateKeyState ();
    virtual void setHalfDuplexMask (KeyModifierMask);
    virtual void fakeKeyDown (KeyID id, KeyModifierMask mask, KeyButton button);
    virtual bool fakeKeyRepeat (KeyID id, KeyModifierMask mask, SInt32 count,
                                KeyButton button);
    virtual bool fakeKeyUp (KeyButton button);
    virtual void fakeAllKeysUp ();
    virtual bool fakeCtrlAltDel ();
    virtual bool isKeyDown (KeyButton) const;
    virtual KeyModifierMask getActiveModifiers () const;
    virtual KeyModifierMask pollActiveModifiers () const;
    virtual SInt32 pollActiveGroup () const;
    virtual void pollPressedKeys (KeyButtonSet& pressedKeys) const;

    virtual void
    setDraggingStarted (bool started) {
        m_draggingStarted = started;
    }
    virtual bool isDraggingStarted ();
    virtual bool
    isFakeDraggingStarted () {
        return m_fakeDraggingStarted;
    }
    virtual String&
    getDraggingFilename () {
        return m_draggingFilename;
    }
    virtual void
    clearDraggingFilename () {
    }

    // IPlatformScreen overrides
    virtual void enable ()  = 0;
    virtual void disable () = 0;
    virtual void enter ()   = 0;
    virtual bool leave ()   = 0;
    virtual bool setClipboard (ClipboardID, const IClipboard*) = 0;
    virtual void checkClipboards ()                      = 0;
    virtual void openScreensaver (bool notify)           = 0;
    virtual void closeScreensaver ()                     = 0;
    virtual void screensaver (bool activate)             = 0;
    virtual void resetOptions ()                         = 0;
    virtual void setOptions (const OptionsList& options) = 0;
    virtual void setSequenceNumber (UInt32)              = 0;
    virtual bool isPrimary () const                      = 0;

    virtual void
    fakeDraggingFiles (DragFileList fileList) {
        throw std::runtime_error ("fakeDraggingFiles not implemented");
    }
    virtual const String&
    getDropTarget () const {
        throw std::runtime_error ("getDropTarget not implemented");
    }

protected:
    //! Update mouse buttons
    /*!
    Subclasses must implement this method to update their internal mouse
    button mapping and, if desired, state tracking.
    */
    virtual void updateButtons () = 0;

    //! Get the key state
    /*!
    Subclasses must implement this method to return the platform specific
    key state object that each subclass must have.
    */
    virtual IKeyState* getKeyState () const = 0;

    // IPlatformScreen overrides
    virtual void handleSystemEvent (const Event& event, void*) = 0;

protected:
    String m_draggingFilename;
    bool m_draggingStarted;
    bool m_fakeDraggingStarted;
};
