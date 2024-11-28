/*
 * Deskflow -- mouse and keyboard sharing utility
 *
 * SPDX-FileCopyrightText: Copyright (C) 2024 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#pragma once

/**
 * \enum SecurityLevel
 * \brief This enum is used to set how the client and server will communicate.
 */
enum class SecurityLevel
{
  PlainText, /** The Connection is not encrypted */
  Encrypted  /** The Connection is encrypted */
};
