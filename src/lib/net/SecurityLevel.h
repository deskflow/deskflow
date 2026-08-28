/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

/**
 * \enum SecurityLevel
 * \brief This enum is used to set how the client and server will communicate.
 */
enum class SecurityLevel
{
  PlainText, /** Connections will not be encrypted */
  Encrypted, /** Connections will be encrypted */
  PeerAuth   /** Connections will be encrypted and peers must be authenticated */
};
