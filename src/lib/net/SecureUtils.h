/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2021 Barrier Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "FingerprintTypes.h"

#include <cstdint>
#include <openssl/ossl_typ.h>
#include <string>
#include <vector>

namespace deskflow {

/**
 * @brief formatSSLFingerprint Format an ssl Fingerprint
 * @param fingerprint input string
 * @param enableSeparators insert : seperator every byte when true
 * @return a Formated Fingerprint String
 */
std::string formatSSLFingerprint(const std::vector<uint8_t> &fingerprint, bool enableSeparators = true);

std::vector<std::uint8_t> SSLCertFingerprint(X509 *cert, FingerprintType type);

std::vector<std::uint8_t> pemFileCertFingerprint(const std::string &path, FingerprintType type);
} // namespace deskflow
