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

#include "server/ClientProxy1_7.h"

#include "server/Server.h"
#include "synergy/ProtocolUtil.h"
#include "synergy/StreamChunker.h"
#include "synergy/ClipboardChunk.h"
#include "io/IStream.h"
#include "base/TMethodEventJob.h"
#include "base/Log.h"

//
// ClientProxy1_7
//

ClientProxy1_7::ClientProxy1_7(const String& name, synergy::IStream* stream, Server* server, IEventQueue* events) :
    ClientProxy1_6(name, stream, server, events),
    m_events(events)
{
    /*m_events->adoptHandler(m_events->forClipboard().clipboardSending(),
                                this,
                                new TMethodEventJob<ClientProxy1_6>(this,
                                    &ClientProxy1_6::handleClipboardSendingEvent));*/

    LOG((CLOG_DEBUG1 "querying client \"%s\" wake-on-lan info", getName().c_str()));
    ProtocolUtil::writef(getStream(), kMsgQWol);
}

ClientProxy1_7::~ClientProxy1_7()
{
}

bool
ClientProxy1_7::parseMessage(const UInt8* code)
{
    // process message
    if (memcmp(code, kMsgDWol, 4) == 0) {
        recvWakeOnLan();
        return true;
    }
    else {
        return ClientProxy1_6::parseMessage(code);
    }
}

bool
ClientProxy1_7::recvWakeOnLan()
{
    // parse the message
    ClientWakeOnLanInfo wol;
    if (!ProtocolUtil::readf(getStream(), kMsgDWol + 4,
        &wol.m_mac[0],
        &wol.m_mac[1],
        &wol.m_mac[2],
        &wol.m_mac[3],
        &wol.m_mac[4],
        &wol.m_mac[5])) {
        return false;
    }
    //LOG((CLOG_DEBUG "received client \"%s\" info shape=%d,%d %dx%d at %d,%d", getName().c_str(), x, y, w, h, mx, my));
    LOG((CLOG_INFO "received client \"%s\" info %d,%d,%d,%d,%d,%d", getName().c_str(), wol.m_mac[0], wol.m_mac[1], wol.m_mac[2], wol.m_mac[3], wol.m_mac[4], wol.m_mac[5]));

    // validate

    // save
    m_macAddresses.push_back("");

    return true;
}