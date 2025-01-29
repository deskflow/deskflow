/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2021 Barrier Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <string>

namespace deskflow {
/**

 * @brief formatSSLFingerprint Format an ssl Fingerprint
 * @param fingerprint input string
 * @param convertToHex when true converts the string to a hex string
 * @param enableSeparators insert : seperator every byte when true
 * @return a Formated Fingerprint String
 */
std::string
formatSSLFingerprint(const std::string &fingerprint, bool convertToHex = true, bool enableSeparators = true);

} // namespace deskflow
