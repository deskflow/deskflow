/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2016 - 2025 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

/**
 * @brief The elevate mode tristate determines two behaviors on Windows.
 *
 * The matrix for these two behaviors is as follows:
 *
 *               |    sods   |   elevate  |
 *               |-----------|------------|
 *  kAutomatic   |   true    |   false    |
 *  kAlways      |   false   |   true     |
 *  kNever       |   false   |   false    |
 *
 * The first, --stop-on-desk-switch (sods), is passed through the daemon as a
 * command line argument to the server/client, and determines if it restarts
 * when switching Windows desktops (e.g. when Windows UAC dialog pops up).
 *
 * The second, elevate, is passed as a boolean flag to the daemon over IPC,
 * and determines whether the server/client should be started with elevated privileges.
 */
enum class ElevateMode
{
  kAutomatic = 0,
  kAlways = 1,
  kNever = 2
};
