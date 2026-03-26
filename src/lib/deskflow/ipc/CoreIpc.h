/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "common/Enums.h"

#include <QString>

void ipcSendToClient(const QString &command, const QString &args = "");
void ipcSendConnectionState(deskflow::core::ConnectionState state);
