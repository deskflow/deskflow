/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "server/VirtualHostTracker.h"

#include "server/BaseClientProxy.h"

void VirtualHostTracker::setConnectLine(std::string line)
{
  m_connectLine = std::move(line);
}

bool VirtualHostTracker::hasConnectLine() const
{
  return !m_connectLine.empty();
}

const std::string &VirtualHostTracker::connectLine() const
{
  return m_connectLine;
}

BaseClientProxy *VirtualHostTracker::host() const
{
  return m_host;
}

void VirtualHostTracker::clearHostIf(BaseClientProxy *client)
{
  if (m_host == client) {
    m_host = nullptr;
  }
}

bool VirtualHostTracker::hostsActiveClient(BaseClientProxy *active) const
{
  return m_host != nullptr && m_host == active;
}
