/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "arch/unix/ArchSystemUnix.h"
#include <array>

#include <common/constants.h>
#include <sys/utsname.h>

#ifndef __APPLE__
#include <QDBusConnection>
#include <QDBusError>
#include <QDBusInterface>
#include <QDBusReply>
#endif

//
// ArchSystemUnix
//

ArchSystemUnix::ArchSystemUnix()
{
  // do nothing
}

ArchSystemUnix::~ArchSystemUnix()
{
  // do nothing
}

std::string ArchSystemUnix::getOSName() const
{
#if defined(HAVE_SYS_UTSNAME_H)
  struct utsname info;
  if (uname(&info) == 0) {
    std::string msg;
    msg += info.sysname;
    msg += " ";
    msg += info.release;
    return msg;
  }
#endif
  return "Unix";
}

std::string ArchSystemUnix::getPlatformName() const
{
#if defined(HAVE_SYS_UTSNAME_H)
  struct utsname info;
  if (uname(&info) == 0) {
    return std::string(info.machine);
  }
#endif
  return "unknown";
}

std::string ArchSystemUnix::setting(const std::string &) const
{
  return "";
}

void ArchSystemUnix::setting(const std::string &, const std::string &) const
{
}

std::string ArchSystemUnix::getLibsUsed(void) const
{
  return "not implemented.\nuse lsof on shell";
}

#ifndef __APPLE__
bool ArchSystemUnix::DBusInhibitScreenCall(InhibitScreenServices serviceID, bool state, std::string &error)
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
      error = "coockies are not empty";
      return false;
    }

    QString msg = "Sleep is manually prevented by the %1 preferences";
    reply = screenSaverInterface.call("Inhibit", kAppName, msg.arg(kAppName));
    if (reply.isValid())
      cookies[serviceNum] = reply.value();
  } else {
    if (!cookies[serviceNum]) {
      error = "coockies are empty";
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
#endif
