/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2004 Chris Schoeneman
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "arch/unix/ArchSystemUnix.h"
#include <array>

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

    reply = screenSaverInterface.call("Inhibit", "Deskflow", "Sleep is manually prevented by the Deskflow preferences");
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
