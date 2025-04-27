/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2010 - 2018, 2024 - 2025 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 - 2007 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#if defined(_WIN32)
#define SYSAPI_WIN32 1
#define WINAPI_MSWINDOWS 1
#elif HAVE_CONFIG_H
#include "Config.h"
#else
#error "config.h missing"
#endif

// define nullptr
#include <stddef.h>

// make assert available since we use it a lot
#include <assert.h>
#include <stdint.h>
#include <stdlib.h> // IWYU pragma: keep
#include <string.h> // IWYU pragma: keep

// defined in Carbon
#if !defined(__MACTYPES__)
#if defined(__APPLE__)
#include <CoreServices/CoreServices.h>
#else
#endif
#endif

enum
{
  kExitSuccess = 0,    // successful completion
  kExitFailed = 1,     // general failure
  kExitTerminated = 2, // killed by signal
  kExitArgs = 3,       // bad arguments
  kExitConfig = 4,     // cannot read configuration
};
