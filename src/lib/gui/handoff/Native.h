/*
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#pragma once
#include "Transfer.h"
#include <functional>

namespace deskflow::handoff {
void installService(std::function<void(Request)> send);
Request currentApplication(const QString &bundleId = {});
void openReceived(const Request &request);
} // namespace deskflow::handoff
