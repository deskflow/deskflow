/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 - 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once
#include <QString>

inline static const auto kSynergyProtocolOption = "synergy";
inline static const auto kBarrierProtocolOption = "barrier";

enum class NetworkProtocol
{
  Unknown = -1,
  Synergy,
  Barrier
};

static int networkProtocolToInt(const NetworkProtocol proto)
{
  return static_cast<int>(proto);
}

static NetworkProtocol networkProtocolFromInt(const int value)
{
  return static_cast<NetworkProtocol>(value);
}

static QString networkProtocolToOption(const NetworkProtocol proto)
{
  switch (proto) {
  case NetworkProtocol::Synergy:
    return kSynergyProtocolOption;
  case NetworkProtocol::Barrier:
    return kBarrierProtocolOption;
  default:
    return {};
  }
}

static QString networkProtocolToName(const NetworkProtocol proto)
{
  switch (proto) {
  case NetworkProtocol::Synergy:
    return QStringLiteral("Synergy");
  case NetworkProtocol::Barrier:
    return QStringLiteral("Barrier");
  default:
    return {};
  }
}

static NetworkProtocol networkProtocolFromString(const QString &proto)
{
  using enum NetworkProtocol;
  if (proto.compare(kBarrierProtocolOption, Qt::CaseInsensitive) == 0)
    return Barrier;
  if (proto.compare(kSynergyProtocolOption, Qt::CaseInsensitive) == 0)
    return Synergy;
  return Unknown;
}
