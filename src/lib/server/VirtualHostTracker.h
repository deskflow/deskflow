/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <string>
#include <utility>

class BaseClientProxy;

//! Tracks which remote client hosts a virtual device and replays connect on focus changes.
//! Used only on the Server event thread (not thread-safe).
class VirtualHostTracker
{
public:
  static constexpr const char *kDefaultDisconnect = R"({"type": "disconnect"})";

  //! Cache connect JSON; pass an empty string to clear.
  void setConnectLine(std::string line = {});
  [[nodiscard]] bool hasConnectLine() const;
  [[nodiscard]] const std::string &connectLine() const;

  [[nodiscard]] BaseClientProxy *host() const;
  void clearHostIf(BaseClientProxy *client);
  [[nodiscard]] bool hostsActiveClient(BaseClientProxy *active) const;

  template<typename SendFn>
  void onFocusChange(
      BaseClientProxy *dst, BaseClientProxy *primary, SendFn &&send, const std::string &connectPayload = {}
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

  template<typename SendFn>
  void detach(SendFn &&send, const std::string &disconnectLine = kDefaultDisconnect)
  {
    if (m_host != nullptr) {
      send(m_host, disconnectLine);
      m_host = nullptr;
    }
  }

private:
  BaseClientProxy *m_host = nullptr;
  std::string m_connectLine;
};
