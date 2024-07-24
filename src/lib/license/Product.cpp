/*
 * synergy -- mouse and keyboard sharing utility
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

const std::string SKE::Pro = "pro";
const std::string SKE::Basic = "basic";
const std::string SKE::Buisiness = "business";

using Edition = Product::Edition;

const std::map<std::string, Edition, std::less<>> serialKeyEditions{
    {SKE::Basic, Edition::kBasic},
    {SKE::Pro, Edition::kPro},
    {SKE::Buisiness, Edition::kBusiness},
};

Product::Product(Edition edition) : m_edition(edition) {}

Product::Product(const std::string &serialKeyEditionID) {
  setEdition(serialKeyEditionID);
}

Edition Product::getEdition() const { return m_edition; }

std::string Product::getSerialKeyId() const {
  switch (getEdition()) {
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

std::string Product::productName() const {

  const std::string nameBase = SYNERGY_PRODUCT_NAME;
  switch (getEdition()) {
    using enum Edition;

  case kPro:
    return nameBase + " Pro";

  case kBasic:
    return nameBase + " Basic";

  case kBusiness:
    return nameBase + " Business";

  default:
    throw InvalidType();
  }
}

void Product::setEdition(Edition edition) { m_edition = edition; }

void Product::setEdition(const std::string &name) {
  const auto &pType = serialKeyEditions.find(name);

  if (pType != serialKeyEditions.end()) {
    m_edition = pType->second;
  } else {
    throw InvalidType();
  }
}

bool Product::isValid() const {
  return serialKeyEditions.contains(getSerialKeyId());
}
