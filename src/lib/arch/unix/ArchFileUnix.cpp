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
