/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Symless Ltd.
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
#include "base/String.h"
#include "common/ipc.h"

class IpcMessage : public EventData
{
public:
  ~IpcMessage() override = default;

  //! Gets the message type ID.
  IpcMessageType type() const
  {
    return m_type;
  }

protected:
  explicit IpcMessage(IpcMessageType type);

private:
  IpcMessageType m_type;
};

class IpcHelloMessage : public IpcMessage
{
public:
  explicit IpcHelloMessage(IpcClientType clientType);
  ~IpcHelloMessage() override = default;

  //! Gets the message type ID.
  IpcClientType clientType() const
  {
    return m_clientType;
  }

private:
  IpcClientType m_clientType;
};

class IpcHelloBackMessage : public IpcMessage
{
public:
  explicit IpcHelloBackMessage();
  ~IpcHelloBackMessage() override = default;
};

class IpcShutdownMessage : public IpcMessage
{
public:
  explicit IpcShutdownMessage();
  ~IpcShutdownMessage() override = default;
};

class IpcLogLineMessage : public IpcMessage
{
public:
  explicit IpcLogLineMessage(const String &logLine);
  ~IpcLogLineMessage() override = default;

  //! Gets the log line.
  String logLine() const
  {
    return m_logLine;
  }

private:
  String m_logLine;
};

class IpcCommandMessage : public IpcMessage
{
public:
  explicit IpcCommandMessage(const String &command, bool elevate);
  ~IpcCommandMessage() override = default;

  //! Gets the command.
  String command() const
  {
    return m_command;
  }

  //! Gets whether or not the process should be elevated on MS Windows.
  bool elevate() const
  {
    return m_elevate;
  }

private:
  String m_command;
  bool m_elevate;
};
