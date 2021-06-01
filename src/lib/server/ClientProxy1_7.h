/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015-2016 Symless Ltd.
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

#include "server/ClientProxy1_6.h"

class Server;
class IEventQueue;

//! Proxy for client implementing protocol version 1.7
class ClientProxy1_7 : public ClientProxy1_6 {
public:
    ClientProxy1_7(const String& name, synergy::IStream* adoptedStream, Server* server, IEventQueue* events);
    ~ClientProxy1_7() override;

protected:
    // ClientProxy overrides
    virtual bool        parseMessage(const UInt8* code) override;

private:
    bool                recvWakeOnLan();
    void                handleDisconnect(const Event&, void*) override;

    std::vector<String> m_macAddresses;
    IEventQueue*        m_events;
};
