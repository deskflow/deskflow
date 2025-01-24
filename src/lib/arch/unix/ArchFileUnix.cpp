/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "arch/unix/ArchFileUnix.h"

#include "common/constants.h"
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
  std::string rtn = "/Applications/";
  rtn.append(kAppName).append(".app/Contents/MacOS");
  return rtn;
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
      return std::filesystem::path(xdgDir) / kAppName;
    } else {
      return homeDir / ".config" / kAppName;
    }
#else
    return homeDir / "Library" / kAppName;
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

void ArchFileUnix::setProfileDirectory(const std::string &s)
{
  m_profileDirectory = s;
}

void ArchFileUnix::setPluginDirectory(const std::string &s)
{
  m_pluginDirectory = s;
}
