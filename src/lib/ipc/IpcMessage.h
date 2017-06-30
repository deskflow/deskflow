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

#include "ipc/Ipc.h"
#include "base/EventTypes.h"
#include "base/String.h"
#include "base/Event.h"

class IpcMessage : public EventData {
public:
    virtual ~IpcMessage ();

    //! Gets the message type ID.
    UInt8
    type () const {
        return m_type;
    }

protected:
    IpcMessage (UInt8 type);

private:
    UInt8 m_type;
};

class IpcHelloMessage : public IpcMessage {
public:
    IpcHelloMessage (EIpcClientType clientType);
    virtual ~IpcHelloMessage ();

    //! Gets the message type ID.
    EIpcClientType
    clientType () const {
        return m_clientType;
    }

private:
    EIpcClientType m_clientType;
};

class IpcShutdownMessage : public IpcMessage {
public:
    IpcShutdownMessage ();
    virtual ~IpcShutdownMessage ();
};


class IpcLogLineMessage : public IpcMessage {
public:
    IpcLogLineMessage (const String& logLine);
    virtual ~IpcLogLineMessage ();

    //! Gets the log line.
    String
    logLine () const {
        return m_logLine;
    }

private:
    String m_logLine;
};

class IpcCommandMessage : public IpcMessage {
public:
    IpcCommandMessage (const String& command, bool elevate);
    virtual ~IpcCommandMessage ();

    //! Gets the command.
    String
    command () const {
        return m_command;
    }

    //! Gets whether or not the process should be elevated on MS Windows.
    bool
    elevate () const {
        return m_elevate;
    }

private:
    String m_command;
    bool m_elevate;
};
