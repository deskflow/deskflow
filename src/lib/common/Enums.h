/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2010 - 2018, 2024 - 2025 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 - 2007 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QObject>

namespace deskflow::client {
Q_NAMESPACE
enum class ErrorType : uint8_t
{
  NoError,
  AlreadyConnected,
  HostnameError,
  GenericError
};
Q_ENUM_NS(ErrorType)
} // namespace deskflow::client

namespace deskflow::core {
Q_NAMESPACE
enum class ConnectionState
{
  Disconnected,
  Connecting,
  Connected,
  Listening
};
Q_ENUM_NS(ConnectionState)
} // namespace deskflow::core
