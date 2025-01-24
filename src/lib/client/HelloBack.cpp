/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "HelloBack.h"

#include "base/Log.h"
#include "deskflow/ProtocolUtil.h"
#include "deskflow/protocol_types.h"

#include <map>
#include <set>

namespace deskflow::client {

//
// HelloBack::Deps
//

void HelloBack::Deps::invalidHello()
{
  m_invalidHello();
}

void HelloBack::Deps::incompatible(int major, int minor)
{
  m_incompatible(major, minor);
}

//
// HelloBack
//

void HelloBack::handleHello(deskflow::IStream *stream, const std::string &clientName) const
{
  int16_t serverMajor;
  int16_t serverMinor;

  // as luck would have it, both "Synergy" and "Barrier" are 7 chars,
  // so we eat 7 chars and then test for either protocol name.
  // we cannot re-use `readf` to check for various hello messages,
  // as `readf` eats bytes (advances the stream position reference).
  std::string protocolName;
  ProtocolUtil::readf(stream, kMsgHello, &protocolName, &serverMajor, &serverMinor);

  if (protocolName != kSynergyProtocolName && protocolName != kBarrierProtocolName) {
    m_deps->invalidHello();
    return;
  }

  // check versions
  LOG_DEBUG("got hello from %s, protocol v%d.%d", protocolName.c_str(), serverMajor, serverMinor);

  const auto helloBackMajor = m_majorVersion;
  auto helloBackMinor = m_minorVersion;

  if (shouldDowngrade(serverMajor, serverMinor)) {
    LOG_NOTE("downgrading to %d.%d protocol for server", serverMajor, serverMinor);
    helloBackMinor = serverMinor;
  } else if (serverMajor < m_majorVersion || (serverMajor == m_majorVersion && serverMinor < m_minorVersion)) {
    m_deps->incompatible(serverMajor, serverMinor);
    return;
  }

  // say hello back with same protocol name and version
  LOG_DEBUG(
      "saying hello back with version %s %d.%d", //
      protocolName.c_str(), helloBackMajor, helloBackMinor
  );

  // dynamically build write format for hello back since `ProtocolUtil::writef`
  // doesn't support formatting fixed length strings yet.
  std::string helloBackMessage = protocolName + kMsgHelloBackArgs;
  ProtocolUtil::writef(stream, helloBackMessage.c_str(), helloBackMajor, helloBackMinor, &clientName);
}

bool HelloBack::shouldDowngrade(int major, int minor) const
{
  const std::map<int, std::set<int>> map{
      // 1.6 is compatible with 1.7 and 1.8
      {6, {7, 8}},

      // 1.7 is compatible with 1.8
      {7, {8}},
  };

  if (major == m_majorVersion) {
    auto versions = map.find(minor);
    if (versions != map.end()) {
      auto compatibleVersions = versions->second;
      if (compatibleVersions.find(m_minorVersion) != compatibleVersions.end()) {
        return true;
      }
    }
  }

  return false;
}

} // namespace deskflow::client
