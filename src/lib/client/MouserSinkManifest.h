/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QString>

namespace deskflow::client {

//! Write ``mouser-sink.json`` so Mouser can auto-detect the loopback sink.
void writeMouserSinkManifest(int port, const QString &token, bool hidPassthroughActive);

//! Remove the manifest when the client disconnects or HID delivery stops.
void clearMouserSinkManifest();

} // namespace deskflow::client
