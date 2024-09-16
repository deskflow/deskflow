/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2016 Symless Ltd.
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

#include <map>

#include "Product.h"

using SKE = Product::SerialKeyEditionID;

const char *const kLicensedProductName = "Deskflow 1";

const std::string SKE::Pro = "pro";
const std::string SKE::Basic = "basic";
const std::string SKE::Buisiness = "business";

using Edition = Product::Edition;

const std::map<std::string, Edition, std::less<>> kSerialKeyEditions{
    {SKE::Basic, Edition::kBasic},
    {SKE::Pro, Edition::kPro},
    {SKE::Buisiness, Edition::kBusiness},
};

Product::Product(Edition edition) : m_edition(edition) {}

Product::Product(const std::string &serialKeyEditionID) {
  setEdition(serialKeyEditionID);
}

Edition Product::edition() const { return m_edition; }

std::string Product::serialKeyId() const {
  switch (edition()) {
    using enum Edition;

  case kPro:
    return SKE::Pro;

  case kBasic:
    return SKE::Basic;

  case kBusiness:
    return SKE::Buisiness;

  default:
    throw InvalidType();
  }
}

std::string Product::name() const {

  const std::string nameBase = kLicensedProductName;
  switch (edition()) {
    using enum Edition;

  case kUnregistered:
    return nameBase + " (unregistered)";

  case kBasic:
    return nameBase + " Basic";

  case kPro:
    return nameBase + " Pro";

  case kBusiness:
    return nameBase + " Business";

  default:
    throw InvalidType();
  }
}

void Product::setEdition(Edition edition) { m_edition = edition; }

void Product::setEdition(const std::string &name) {
  const auto &pType = kSerialKeyEditions.find(name);

  if (pType != kSerialKeyEditions.end()) {
    m_edition = pType->second;
  } else {
    throw InvalidType();
  }
}

bool Product::isValid() const {
  if (m_edition == Edition::kUnregistered) {
    return false;
  }
  return kSerialKeyEditions.contains(serialKeyId());
}

bool Product::isTlsAvailable() const {
  switch (edition()) {
    using enum Edition;

  case kPro:
  case kBusiness:
    return true;

  case kBasic:
  case kUnregistered:
    return false;

  default:
    throw InvalidType();
  }
}
