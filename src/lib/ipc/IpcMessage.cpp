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

#include "ipc/IpcMessage.h"

#include <utility>
#include "ipc/Ipc.h"

IpcMessage::IpcMessage(UInt8 type) :
    m_type(type)
{
}

IpcMessage::~IpcMessage()
= default;

IpcHelloMessage::IpcHelloMessage(EIpcClientType clientType) :
    IpcMessage(kIpcHello),
    m_clientType(clientType)
{
}

IpcHelloMessage::~IpcHelloMessage()
= default;

IpcShutdownMessage::IpcShutdownMessage() :
IpcMessage(kIpcShutdown)
{
}

IpcShutdownMessage::~IpcShutdownMessage()
= default;

IpcLogLineMessage::IpcLogLineMessage(String  logLine) :
IpcMessage(kIpcLogLine),
m_logLine(std::move(logLine))
{
}

IpcLogLineMessage::~IpcLogLineMessage()
= default;

IpcCommandMessage::IpcCommandMessage(String  command, bool elevate) :
IpcMessage(kIpcCommand),
m_command(std::move(command)),
m_elevate(elevate)
{
}

IpcCommandMessage::~IpcCommandMessage()
= default;
