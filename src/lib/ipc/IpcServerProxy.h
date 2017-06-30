/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2012 Nick Bolton
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

#include "base/Event.h"
#include "base/EventTypes.h"

namespace synergy {
class IStream;
}
class IpcMessage;
class IpcLogLineMessage;
class IEventQueue;

class IpcServerProxy {
    friend class IpcClient;

public:
    IpcServerProxy (synergy::IStream& stream, IEventQueue* events);
    virtual ~IpcServerProxy ();

private:
    void send (const IpcMessage& message);

    void handleData (const Event&, void*);
    IpcLogLineMessage* parseLogLine ();
    void disconnect ();

private:
    synergy::IStream& m_stream;
    IEventQueue* m_events;
};
