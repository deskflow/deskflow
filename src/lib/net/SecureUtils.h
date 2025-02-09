/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2021 Barrier Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "FingerprintData.h"

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

std::string formatSSLFingerprintColumns(const std::vector<uint8_t> &fingerprint);

FingerprintData sslCertFingerprint(X509 *cert, FingerprintType type);

FingerprintData pemFileCertFingerprint(const std::string &path, FingerprintType type);

void generatePemSelfSignedCert(const std::string &path, int keyLength = 2048);

int getCertLength(const std::string &path);

std::string generateFingerprintArt(const std::vector<std::uint8_t> &rawDigest);
} // namespace deskflow
