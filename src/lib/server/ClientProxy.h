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

namespace synergy { class IStream; }

//! Generic proxy for client
class ClientProxy : public BaseClientProxy {
public:
    /*!
    \c name is the name of the client.
    */
    ClientProxy(const String& name, synergy::IStream* adoptedStream);
    ClientProxy(ClientProxy const &) =delete;
    ClientProxy(ClientProxy &&) =delete;
    ~ClientProxy();

    ClientProxy& operator=(ClientProxy const &) =delete;
    ClientProxy& operator=(ClientProxy &&) =delete;

    //! @name manipulators
    //@{

    //! Disconnect
    /*!
    Ask the client to disconnect, using \p msg as the reason.
    */
    void                close(const char* msg);

    //@}
    //! @name accessors
    //@{

    //! Get stream
    /*!
    Returns the original stream passed to the c'tor.
    */
    synergy::IStream*    getStream() const override;

    //@}

    // IScreen
    void*       getEventTarget() const override;
    bool        getClipboard(ClipboardID id, IClipboard*) const override = 0;
    void        getShape(SInt32& x, SInt32& y,
                            SInt32& width, SInt32& height) const override = 0;
    void        getCursorPos(SInt32& x, SInt32& y) const override = 0;

    // IClient overrides
    void        enter(SInt32 xAbs, SInt32 yAbs,
                            UInt32 seqNum, KeyModifierMask mask,
                            bool forScreensaver) override = 0;
    bool        leave() override = 0;
    void        setClipboard(ClipboardID, const IClipboard*) override = 0;
    void        grabClipboard(ClipboardID) override = 0;
    void        setClipboardDirty(ClipboardID, bool) override = 0;
    void        keyDown(KeyID, KeyModifierMask, KeyButton, const String&) override = 0;
    void        keyRepeat(KeyID, KeyModifierMask,
                            SInt32 count, KeyButton, const String& lang) override = 0;
    void        keyUp(KeyID, KeyModifierMask, KeyButton) override = 0;
    void        mouseDown(ButtonID) override = 0;
    void        mouseUp(ButtonID) override = 0;
    void        mouseMove(SInt32 xAbs, SInt32 yAbs) override = 0;
    void        mouseRelativeMove(SInt32 xRel, SInt32 yRel) override = 0;
    void        mouseWheel(SInt32 xDelta, SInt32 yDelta) override = 0;
    void        screensaver(bool activate) override = 0;
    void        resetOptions() override = 0;
    void        setOptions(const OptionsList& options) override = 0;
    void        sendDragInfo(UInt32 fileCount, const char* info,
                            size_t size) override = 0;
    void        fileChunkSending(UInt8 mark, char* data, size_t dataSize) override = 0;
    void        secureInputNotification(const String& app) const override = 0;

private:
    synergy::IStream*    m_stream;
};
