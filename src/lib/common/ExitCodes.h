/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2010 - 2018, 2024 - 2025 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 - 2007 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

static const int s_exitSuccess = 0;    //!< App successfully completed
static const int s_exitFailed = 1;     //!< App had a general failure
static const int s_exitTerminated = 2; //!< App was kill by a signal
static const int s_exitArgs = 3;       //!< App was unable to run due to bad arguments being passed
static const int s_exitConfig = 4;     //!< App was unable to read the configuration
static const int s_exitDuplicate = 5;  //!< An instance of the app (or core app) is already running
