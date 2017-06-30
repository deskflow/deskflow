/*
 * synergy -- mouse and keyboard sharing utility
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

#pragma once

#include "server/BaseClientProxy.h"
#include "synergy/protocol_types.h"

namespace synergy {
class Screen;
}

//! Primary screen as pseudo-client
/*!
The primary screen does not have a client associated with it.  This
class provides a pseudo-client to allow the primary screen to be
treated as if it was a client.
*/
class PrimaryClient : public BaseClientProxy {
public:
    /*!
    \c name is the name of the server and \p screen is primary screen.
    */
    PrimaryClient (const String& name, synergy::Screen* screen);
    ~PrimaryClient ();

#ifdef TEST_ENV
    PrimaryClient () : BaseClientProxy ("") {
    }
#endif

    //! @name manipulators
    //@{

    //! Update configuration
    /*!
    Handles reconfiguration of jump zones.
    */
    virtual void reconfigure (UInt32 activeSides);

    //! Register a system hotkey
    /*!
    Registers a system-wide hotkey for key \p key with modifiers \p mask.
    Returns an id used to unregister the hotkey.
    */
    virtual UInt32 registerHotKey (KeyID key, KeyModifierMask mask);

    //! Unregister a system hotkey
    /*!
    Unregisters a previously registered hot key.
    */
    virtual void unregisterHotKey (UInt32 id);

    //! Prepare to synthesize input on primary screen
    /*!
    Prepares the primary screen to receive synthesized input.  We do not
    want to receive this synthesized input as user input so this method
    ensures that we ignore it.  Calls to \c fakeInputBegin() and
    \c fakeInputEnd() may be nested;  only the outermost have an effect.
    */
    void fakeInputBegin ();

    //! Done synthesizing input on primary screen
    /*!
    Undoes whatever \c fakeInputBegin() did.
    */
    void fakeInputEnd ();

    //@}
    //! @name accessors
    //@{

    //! Get jump zone size
    /*!
    Return the jump zone size, the size of the regions on the edges of
    the screen that cause the cursor to jump to another screen.
    */
    SInt32 getJumpZoneSize () const;

    //! Get cursor center position
    /*!
    Return the cursor center position which is where we park the
    cursor to compute cursor motion deltas and should be far from
    the edges of the screen, typically the center.
    */
    void getCursorCenter (SInt32& x, SInt32& y) const;

    //! Get toggle key state
    /*!
    Returns the primary screen's current toggle modifier key state.
    */
    virtual KeyModifierMask getToggleMask () const;

    //! Get screen lock state
    /*!
    Returns true if the user is locked to the screen.
    */
    bool isLockedToScreen () const;

    //@}

    // FIXME -- these probably belong on IScreen
    virtual void enable ();
    virtual void disable ();

    // IScreen overrides
    virtual void* getEventTarget () const;
    virtual bool getClipboard (ClipboardID id, IClipboard*) const;
    virtual void
    getShape (SInt32& x, SInt32& y, SInt32& width, SInt32& height) const;
    virtual void getCursorPos (SInt32& x, SInt32& y) const;

    // IClient overrides
    virtual void enter (SInt32 xAbs, SInt32 yAbs, UInt32 seqNum,
                        KeyModifierMask mask, bool forScreensaver);
    virtual bool leave ();
    virtual void setClipboard (ClipboardID, const IClipboard*);
    virtual void grabClipboard (ClipboardID);
    virtual void setClipboardDirty (ClipboardID, bool);
    virtual void keyDown (KeyID, KeyModifierMask, KeyButton);
    virtual void keyRepeat (KeyID, KeyModifierMask, SInt32 count, KeyButton);
    virtual void keyUp (KeyID, KeyModifierMask, KeyButton);
    virtual void mouseDown (ButtonID);
    virtual void mouseUp (ButtonID);
    virtual void mouseMove (SInt32 xAbs, SInt32 yAbs);
    virtual void mouseRelativeMove (SInt32 xRel, SInt32 yRel);
    virtual void mouseWheel (SInt32 xDelta, SInt32 yDelta);
    virtual void screensaver (bool activate);
    virtual void resetOptions ();
    virtual void setOptions (const OptionsList& options);
    virtual void sendDragInfo (UInt32 fileCount, const char* info, size_t size);
    virtual void fileChunkSending (UInt8 mark, char* data, size_t dataSize);

    virtual synergy::IStream*
    getStream () const {
        return NULL;
    }
    bool
    isPrimary () const {
        return true;
    }

private:
    synergy::Screen* m_screen;
    bool m_clipboardDirty[kClipboardEnd];
    SInt32 m_fakeInputCount;
};
