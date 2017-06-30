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

#include "server/ClientProxy.h"
#include "synergy/Clipboard.h"
#include "synergy/protocol_types.h"

class Event;
class EventQueueTimer;
class IEventQueue;

//! Proxy for client implementing protocol version 1.0
class ClientProxy1_0 : public ClientProxy {
public:
    ClientProxy1_0 (const String& name, synergy::IStream* adoptedStream,
                    IEventQueue* events);
    ~ClientProxy1_0 ();

    // IScreen
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

protected:
    virtual bool parseHandshakeMessage (const UInt8* code);
    virtual bool parseMessage (const UInt8* code);

    virtual void resetHeartbeatRate ();
    virtual void setHeartbeatRate (double rate, double alarm);
    virtual void resetHeartbeatTimer ();
    virtual void addHeartbeatTimer ();
    virtual void removeHeartbeatTimer ();
    virtual bool recvClipboard ();

private:
    void disconnect ();
    void removeHandlers ();

    void handleData (const Event&, void*);
    void handleDisconnect (const Event&, void*);
    void handleWriteError (const Event&, void*);
    void handleFlatline (const Event&, void*);

    bool recvInfo ();
    bool recvGrabClipboard ();

protected:
    struct ClientClipboard {
    public:
        ClientClipboard ();

    public:
        Clipboard m_clipboard;
        UInt32 m_sequenceNumber;
        bool m_dirty;
    };

    ClientClipboard m_clipboard[kClipboardEnd];

private:
    typedef bool (ClientProxy1_0::*MessageParser) (const UInt8*);

    ClientInfo m_info;
    double m_heartbeatAlarm;
    EventQueueTimer* m_heartbeatTimer;
    MessageParser m_parser;
    IEventQueue* m_events;
};
