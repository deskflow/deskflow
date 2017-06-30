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
#include "base/Event.h"
#include "base/String.h"
#include "base/EventTypes.h"

namespace synergy {
class IStream;
}

//! Generic proxy for client
class ClientProxy : public BaseClientProxy {
public:
    /*!
    \c name is the name of the client.
    */
    ClientProxy (const String& name, synergy::IStream* adoptedStream);
    ~ClientProxy ();

    //! @name manipulators
    //@{

    //! Disconnect
    /*!
    Ask the client to disconnect, using \p msg as the reason.
    */
    void close (const char* msg);

    //@}
    //! @name accessors
    //@{

    //! Get stream
    /*!
    Returns the original stream passed to the c'tor.
    */
    synergy::IStream* getStream () const;

    //@}

    // IScreen
    virtual void* getEventTarget () const;
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

private:
    synergy::IStream* m_stream;
};
