/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <functional>
#include <string>

class BaseClientProxy;

//! Tracks which remote client hosts a virtual device and replays connect on focus changes.
class VirtualHostTracker
{
public:
  static constexpr const char *kDefaultDisconnect = R"({"type": "disconnect"})";

  void setConnectLine(std::string line);
  void clearConnectLine();
  [[nodiscard]] bool hasConnectLine() const;
  [[nodiscard]] const std::string &connectLine() const;

  [[nodiscard]] BaseClientProxy *host() const;
  void clearHostIf(BaseClientProxy *client);
  [[nodiscard]] bool relaysTo(BaseClientProxy *active) const;

  void onFocusChange(
      BaseClientProxy *dst, BaseClientProxy *primary,
      const std::function<void(BaseClientProxy *, const std::string &)> &send,
      const std::string &connectPayload = {}
  );

  void detach(
      const std::function<void(BaseClientProxy *, const std::string &)> &send,
      const std::string &disconnectLine = kDefaultDisconnect
  );

private:
  BaseClientProxy *m_host = nullptr;
  std::string m_connectLine;
};
