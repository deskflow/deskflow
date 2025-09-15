/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "XDGPowerManager.h"
#include "arch/Arch.h"
#include "base/Log.h"
#include "common/Constants.h"

#include <QDBusConnection>
#include <QDBusError>
#include <QDBusInterface>
#include <QDBusReply>

namespace {

bool sleepInhibitCall(bool state, XDGPowerManager::InhibitScreenServices serviceID)
{
  if (std::string error; !XDGPowerManager::inhibitScreenCall(serviceID, state, error)) {
    LOG_DEBUG("dbus inhibit error %s", error.c_str());
    return false;
  }

  return true;
}

} // namespace

XDGPowerManager::~XDGPowerManager()
{
  enableSleep();
}

void XDGPowerManager::disableSleep() const
{
  if (!sleepInhibitCall(true, XDGPowerManager::InhibitScreenServices::kScreenSaver) &&
      !sleepInhibitCall(true, XDGPowerManager::InhibitScreenServices::kSessionManager)) {
    LOG_WARN("failed to prevent system from going to sleep");
  }
}

void XDGPowerManager::enableSleep() const
{
  if (!sleepInhibitCall(false, XDGPowerManager::InhibitScreenServices::kScreenSaver) &&
      !sleepInhibitCall(false, XDGPowerManager::InhibitScreenServices::kSessionManager)) {
    LOG_WARN("failed to enable system idle sleep");
  }
}

bool XDGPowerManager::inhibitScreenCall(InhibitScreenServices serviceID, bool state, std::string &error)
{
  error = "";
  static const std::array<QString, 2> services = {"org.freedesktop.ScreenSaver", "org.gnome.SessionManager"};
  static const std::array<QString, 2> paths = {"/org/freedesktop/ScreenSaver", "/org/gnome/SessionManager"};
  static std::array<uint, 2> cookies;

  auto serviceNum = static_cast<uint8_t>(serviceID);

  QDBusConnection bus = QDBusConnection::sessionBus();
  if (!bus.isConnected()) {
    error = "bus failed to connect";
    return false;
  }

  QDBusInterface screenSaverInterface(services[serviceNum], paths[serviceNum], services[serviceNum], bus);

  if (!screenSaverInterface.isValid()) {
    error = "screen saver interface failed to initialize";
    return false;
  }

  QDBusReply<uint> reply;
  if (state) {
    if (cookies[serviceNum]) {
      error = "cookies are not empty";
      return false;
    }

    QString msg = "Sleep is manually prevented by the %1 preferences";
    reply = screenSaverInterface.call("Inhibit", kAppName, msg.arg(kAppName));
    if (reply.isValid())
      cookies[serviceNum] = reply.value();
  } else {
    if (!cookies[serviceNum]) {
      error = "cookies are empty";
      return false;
    }
    reply = screenSaverInterface.call("UnInhibit", cookies[serviceNum]);
    cookies[serviceNum] = 0;
  }

  if (!reply.isValid()) {
    QDBusError qerror = reply.error();
    error = qerror.name().toStdString() + " : " + qerror.message().toStdString();
    return false;
  }

  return true;
}
