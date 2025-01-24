/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/Event.h"
#include "common/ipc.h"

#include <string>

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
  explicit IpcLogLineMessage(const std::string &logLine);
  ~IpcLogLineMessage() override = default;

  //! Gets the log line.
  std::string logLine() const
  {
    return m_logLine;
  }

private:
  std::string m_logLine;
};

class IpcCommandMessage : public IpcMessage
{
public:
  explicit IpcCommandMessage(const std::string &command, bool elevate);
  ~IpcCommandMessage() override = default;

  //! Gets the command.
  std::string command() const
  {
    return m_command;
  }

  //! Gets whether or not the process should be elevated on MS Windows.
  bool elevate() const
  {
    return m_elevate;
  }

private:
  std::string m_command;
  bool m_elevate;
};
