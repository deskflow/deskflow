/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2015-2016 Symless Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "FingerprintData.h"
#include <cstdint>
#include <openssl/ossl_typ.h>
#include <string>
#include <vector>

namespace deskflow {

std::string format_ssl_fingerprint(const std::vector<std::uint8_t> &fingerprint, bool separator = true);
std::string format_ssl_fingerprint_columns(const std::vector<uint8_t> &fingerprint);

FingerprintData get_ssl_cert_fingerprint(X509 *cert, FingerprintType type);

FingerprintData get_pem_file_cert_fingerprint(const std::string &path, FingerprintType type);

void generate_pem_self_signed_cert(const std::string &path);

std::string create_fingerprint_randomart(const std::vector<std::uint8_t> &dgst_raw);

} // namespace deskflow
