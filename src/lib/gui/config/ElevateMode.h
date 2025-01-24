/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

/**
 * @brief The elevate mode tristate determines two behaviors on Windows.
 *
 * The first, switch-on-desk-switch (SodS), passed through deskflowd as a
 * command line argument to deskflow core, determines if the server restarts
 * when switching Windows desktops (e.g. when Windows UAC dialog pops up).
 * The second, passed as a boolean flag to Deskflowd over the IPC inside
 * IpcMessageType::CommandMessage, determines whether Deskflow should be started
 * with elevated privileges.
 *
 * The matrix for these two behaviors is as follows:
 *
 *               |    SodS   |   Elevate  |
 *               |-----------|------------|
 *  kAutomatic   |   true    |   false    |
 *  kAlways      |   false   |   true     |
 *  kNever       |   false   |   false    |
 */
enum class ElevateMode
{
  kAutomatic = 0,
  kAlways = 1,
  kNever = 2
};
