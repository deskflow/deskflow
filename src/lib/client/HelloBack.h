/*
 * Deskflow -- mouse and keyboard sharing utility
 *
 * SPDX-FileCopyrightText: Copyright (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0
 */

#pragma once

#include "common/basic_types.h"
#include "deskflow/protocol_types.h"
#include "io/IStream.h"

#include <functional>
#include <memory>

namespace deskflow::client {

class HelloBack
{
public:
  struct Deps
  {
    Deps() = default;
    explicit Deps(std::function<void()> invalidHello, std::function<void(int, int)> incompatible)
        : m_invalidHello(std::move(invalidHello)),
          m_incompatible(std::move(incompatible))
    {
    }
    virtual ~Deps() = default;

    /**
     * @brief Call when invalid hello message received from server.
     */
    virtual void invalidHello();

    /**
     * @brief Call when the client is incompatible with the server.
     */
    virtual void incompatible(int major, int minor);

  private:
    std::function<void()> m_invalidHello;
    std::function<void(int, int)> m_incompatible;
  };

  explicit HelloBack(
      std::shared_ptr<Deps> deps, const SInt16 majorVersion = kProtocolMajorVersion,
      const SInt16 minorVersion = kProtocolMinorVersion
  )
      : m_deps(deps),
        m_majorVersion(majorVersion),
        m_minorVersion(minorVersion)
  {
  }

  /**
   * @brief Handle hello message from server and reply with hello back.
   */
  void handleHello(deskflow::IStream *stream, const std::string &clientName) const;

private:
  bool shouldDowngrade(int major, int minor) const;

  std::shared_ptr<Deps> m_deps;
  SInt16 m_majorVersion;
  SInt16 m_minorVersion;
};

} // namespace deskflow::client
