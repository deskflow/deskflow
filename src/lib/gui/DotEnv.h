/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QString>

namespace deskflow::gui {

const QString kDefaultEnvFilename = ".env";

/**
 * @brief Loads environment variables from a .env file.
 *
 * First checks current dir for .env, then looks in the app dir.
 */
void dotenv(const QString &filename = kDefaultEnvFilename);

} // namespace deskflow::gui
