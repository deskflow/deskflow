/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>

#include "common/Constants.h"
#include "common/PlatformInfo.h"

namespace deskflow::platform {

/**
 * @brief setAppId Set the app id for the xdg portal registry
 * All Applications should do this before attempting any other portal calls
 * If the app is runing from inside a sandbox (i.e flatpak or a snap) this call does nothing
 */
inline void setAppId()
{
  // Sandboxed applications are unable to use this portal
  if (isSandboxed()) {
    return;
  }
  auto i = new QDBusInterface(
      "org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop", "org.freedesktop.host.portal.Registry",
      QDBusConnection::sessionBus(), nullptr
  );

  if (!i->property("version").toInt()) {
    qDebug() << "portal registry not found";
    return;
  }

  std::ignore = i->call("Register", kRevFqdnName, QVariantMap{});
}

} // namespace deskflow::platform
