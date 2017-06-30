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

#include "synergy/IClient.h"
#include "base/String.h"

namespace synergy {
class IStream;
}

//! Generic proxy for client or primary
class BaseClientProxy : public IClient {
public:
    /*!
    \c name is the name of the client.
    */
    BaseClientProxy (const String& name);
    ~BaseClientProxy ();

    //! @name manipulators
    //@{

    //! Save cursor position
    /*!
    Save the position of the cursor when jumping from client.
    */
    void setJumpCursorPos (SInt32 x, SInt32 y);

    //@}
    //! @name accessors
    //@{

    //! Get cursor position
    /*!
    Get the position of the cursor when last jumping from client.
    */
    void getJumpCursorPos (SInt32& x, SInt32& y) const;

    //! Get cursor position
    /*!
    Return if this proxy is for client or primary.
    */
    virtual bool
    isPrimary () const {
        return false;
    }

    //@}

    // IScreen
    virtual void* getEventTarget () const = 0;
    virtual bool getClipboard (ClipboardID id, IClipboard*) const = 0;
    virtual void
    getShape (SInt32& x, SInt32& y, SInt32& width, SInt32& height) const = 0;
    virtual void getCursorPos (SInt32& x, SInt32& y) const = 0;

    // IClient overrides
    virtual void enter (SInt32 xAbs, SInt32 yAbs, UInt32 seqNum,
                        KeyModifierMask mask, bool forScreensaver) = 0;
    virtual bool leave () = 0;
    virtual void setClipboard (ClipboardID, const IClipboard*) = 0;
    virtual void grabClipboard (ClipboardID) = 0;
    virtual void setClipboardDirty (ClipboardID, bool) = 0;
    virtual void keyDown (KeyID, KeyModifierMask, KeyButton) = 0;
    virtual void
    keyRepeat (KeyID, KeyModifierMask, SInt32 count, KeyButton) = 0;
    virtual void keyUp (KeyID, KeyModifierMask, KeyButton) = 0;
    virtual void mouseDown (ButtonID) = 0;
    virtual void mouseUp (ButtonID)   = 0;
    virtual void mouseMove (SInt32 xAbs, SInt32 yAbs)         = 0;
    virtual void mouseRelativeMove (SInt32 xRel, SInt32 yRel) = 0;
    virtual void mouseWheel (SInt32 xDelta, SInt32 yDelta)    = 0;
    virtual void screensaver (bool activate)             = 0;
    virtual void resetOptions ()                         = 0;
    virtual void setOptions (const OptionsList& options) = 0;
    virtual void
    sendDragInfo (UInt32 fileCount, const char* info, size_t size)          = 0;
    virtual void fileChunkSending (UInt8 mark, char* data, size_t dataSize) = 0;
    virtual String getName () const;
    virtual synergy::IStream* getStream () const = 0;

private:
    String m_name;
    SInt32 m_x, m_y;
};
