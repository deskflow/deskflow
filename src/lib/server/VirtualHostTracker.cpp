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

void VirtualHostTracker::clearConnectLine()
{
  m_connectLine.clear();
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

bool VirtualHostTracker::relaysTo(BaseClientProxy *active) const
{
  return m_host != nullptr && m_host == active;
}

void VirtualHostTracker::onFocusChange(
    BaseClientProxy *dst, BaseClientProxy *primary,
    const std::function<void(BaseClientProxy *, const std::string &)> &send, const std::string &connectPayload
)
{
  if (dst == nullptr || primary == nullptr) {
    return;
  }

  const bool dstIsPrimary = (dst == primary);

  if (m_host != nullptr && m_host != dst) {
    send(m_host, kDefaultDisconnect);
    m_host = nullptr;
  }

  const std::string &line = connectPayload.empty() ? m_connectLine : connectPayload;
  if (!dstIsPrimary && !line.empty() && m_host != dst) {
    send(dst, line);
    m_host = dst;
  }
}

void VirtualHostTracker::detach(
    const std::function<void(BaseClientProxy *, const std::string &)> &send, const std::string &disconnectLine
)
{
  if (m_host != nullptr) {
    send(m_host, disconnectLine);
    m_host = nullptr;
  }
}
