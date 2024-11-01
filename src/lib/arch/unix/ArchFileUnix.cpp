/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2002 Chris Schoeneman
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

#include "arch/unix/ArchFileUnix.h"

#include <cstring>
#include <filesystem>
#include <pwd.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

//
// ArchFileUnix
//

ArchFileUnix::ArchFileUnix()
{
  // do nothing
}

ArchFileUnix::~ArchFileUnix()
{
  // do nothing
}

const char *ArchFileUnix::getBasename(const char *pathname)
{
  if (pathname == NULL) {
    return NULL;
  }

  const char *basename = strrchr(pathname, '/');
  if (basename != NULL) {
    return basename + 1;
  } else {
    return pathname;
  }
}

std::string ArchFileUnix::getUserDirectory()
{
  char *buffer = NULL;
  std::string dir;
#if HAVE_GETPWUID_R
  struct passwd pwent;
  struct passwd *pwentp{};
#if defined(_SC_GETPW_R_SIZE_MAX)
  long size = sysconf(_SC_GETPW_R_SIZE_MAX);
  if (size == -1) {
    size = BUFSIZ;
  }
#else
  long size = BUFSIZ;
#endif
  buffer = new char[size];
  getpwuid_r(getuid(), &pwent, buffer, size, &pwentp);
#else
  struct passwd *pwentp = getpwuid(getuid());
#endif
  if (pwentp != NULL && pwentp->pw_dir != NULL) {
    dir = pwentp->pw_dir;
  }
  delete[] buffer;
  return dir;
}

std::string ArchFileUnix::getSystemDirectory()
{
  return "/etc";
}

std::string ArchFileUnix::getInstalledDirectory()
{
#if WINAPI_XWINDOWS
  return "/usr/bin";
#else
  return "/Applications/Deskflow.app/Contents/MacOS";
#endif
}

std::string ArchFileUnix::getLogDirectory()
{
  return "/var/log";
}

std::string ArchFileUnix::getPluginDirectory()
{
  if (!m_pluginDirectory.empty()) {
    return m_pluginDirectory;
  }

#if WINAPI_XWINDOWS
  return getProfileDirectory().append("/plugins");
#else
  return getProfileDirectory().append("/Plugins");
#endif
}

std::string ArchFileUnix::getProfileDirectory()
{
  if (!m_profileDirectory.empty()) {
    return m_profileDirectory;
  } else {
    const std::filesystem::path homeDir = getUserDirectory();
#if WINAPI_XWINDOWS
    const auto xdgDir = std::getenv("XDG_CONFIG_HOME");
    if (xdgDir != nullptr) {
      return std::filesystem::path(xdgDir) / "Deskflow";
    } else {
      return homeDir / ".config" / "Deskflow";
    }
#else
    return homeDir / "Library" / "Deskflow";
#endif
  }
}

std::string ArchFileUnix::concatPath(const std::string &prefix, const std::string &suffix)
{
  std::string path;
  path.reserve(prefix.size() + 1 + suffix.size());
  path += prefix;
  if (path.size() == 0 || path[path.size() - 1] != '/') {
    path += '/';
  }
  path += suffix;
  return path;
}

void ArchFileUnix::setProfileDirectory(const String &s)
{
  m_profileDirectory = s;
}

void ArchFileUnix::setPluginDirectory(const String &s)
{
  m_pluginDirectory = s;
}
