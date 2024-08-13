/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2024 Synergy Ltd.
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

#include "license_utils.h"

#include "license/parse_serial_key.h"
#include "string_utils.h"

#include <QString>
#include <QtGlobal>

namespace synergy::gui::license {

#ifdef SYNERGY_LICENSED_PRODUCT
const bool kLicensedProduct = true;
#else
const bool kLicensedProduct = false;
#endif

#ifdef SYNERGY_ENABLE_ACTIVATION
#ifndef SYNERGY_LICENSED_PRODUCT
#error "activation requires licensed product"
#endif
const bool kEnableActivation = true;
#else
const bool kEnableActivation = false;
#endif // SYNERGY_ENABLE_ACTIVATION

bool isLicensedProduct() {
  if (strToTrue(qEnvironmentVariable("SYNERGY_LICENSED_PRODUCT"))) {
    return true;
  } else {
    return kLicensedProduct;
  }
}

bool isActivationEnabled() {
  if (strToTrue(qEnvironmentVariable("SYNERGY_ENABLE_ACTIVATION"))) {
    return true;
  } else {
    return kEnableActivation;
  }
}

synergy::license::SerialKey parseSerialKey(const QString &hexString) {
  try {
    return synergy::license::parseSerialKey(hexString.toStdString());
  } catch (const std::exception &e) {
    qFatal("failed to parse serial key: %s", e.what());
    abort();
  } catch (...) {
    qFatal("failed to parse serial key, unknown error");
    abort();
  }
}

} // namespace synergy::gui::license
