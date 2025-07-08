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

static const int s_exitSuccess = 0;    //!< App successfully completed
static const int s_exitFailed = 1;     //!< App had a general failure
static const int s_exitTerminated = 2; //!< App was kill by a signal
static const int s_exitArgs = 3;       //!< App was unable to run due to bad arguments being passed
static const int s_exitConfig = 4;     //!< App was unable to read the configuration
