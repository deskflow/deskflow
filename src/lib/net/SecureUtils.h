/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2021 Barrier Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "Fingerprint.h"

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
QString formatSSLFingerprint(const QByteArray &fingerprint, bool enableSeparators = true);

QString formatSSLFingerprintColumns(const QByteArray &fingerprint);

Fingerprint sslCertFingerprint(X509 *cert, Fingerprint::Type type);

Fingerprint pemFileCertFingerprint(const std::string &path, Fingerprint::Type type);

void generatePemSelfSignedCert(const std::string &path, int keyLength = 2048);

int getCertLength(const std::string &path);

QString generateFingerprintArt(const QByteArray &rawDigest);
} // namespace deskflow
