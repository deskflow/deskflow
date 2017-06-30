/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2013-2016 Symless Ltd.
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

#include "server/ClientProxy1_4.h"
#include "base/Stopwatch.h"
#include "common/stdvector.h"

class Server;
class IEventQueue;

//! Proxy for client implementing protocol version 1.5
class ClientProxy1_5 : public ClientProxy1_4 {
public:
    ClientProxy1_5 (const String& name, synergy::IStream* adoptedStream,
                    Server* server, IEventQueue* events);
    ~ClientProxy1_5 ();

    virtual void sendDragInfo (UInt32 fileCount, const char* info, size_t size);
    virtual void fileChunkSending (UInt8 mark, char* data, size_t dataSize);
    virtual bool parseMessage (const UInt8* code);
    void fileChunkReceived ();
    void dragInfoReceived ();

private:
    IEventQueue* m_events;
};
