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

#include "SerialKeyEdition.h"

const std::string SerialKeyEdition::Pro = "pro";
const std::string SerialKeyEdition::ProChina = "pro_china";
const std::string SerialKeyEdition::Basic = "basic";
const std::string SerialKeyEdition::BasicChina = "basic_china";
const std::string SerialKeyEdition::Buisiness = "business";
const std::string SerialKeyEdition::Unregistered = "community";
const std::string SerialKeyEdition::Ultimate = "ultimate";
const std::string SerialKeyEdition::Lite = "lite";

const std::map<std::string, Edition, std::less<>> serialTypes{
    {SerialKeyEdition::Basic, kBasic},
    {SerialKeyEdition::Pro, kPro},
    {SerialKeyEdition::BasicChina, kBasicChina},
    {SerialKeyEdition::ProChina, kProChina},
    {SerialKeyEdition::Buisiness, kBusiness},
    {SerialKeyEdition::Lite, kLite},
    {SerialKeyEdition::Ultimate, kUltimate}};

SerialKeyEdition::SerialKeyEdition(Edition type) : m_type(type) {}

SerialKeyEdition::SerialKeyEdition(const std::string &type) { setType(type); }

Edition SerialKeyEdition::getType() const { return m_type; }

std::string SerialKeyEdition::getName() const {
  switch (getType()) {
  case kLite:
    return Lite;

  case kUltimate:
    return Ultimate;

  case kPro:
    return Pro;

  case kBasic:
    return Basic;

  case kBusiness:
    return Buisiness;

  case kBasicChina:
    return BasicChina;

  case kProChina:
    return ProChina;

  default:
    return Unregistered;
  }
}

std::string SerialKeyEdition::getProductName() const {
  const std::string nameBase = "Synergy 1";

  switch (getType()) {

  case kLite:
    return nameBase;

  case kUltimate:
    return nameBase + " Ultimate";

  case kPro:
    return nameBase + " Pro";

  case kBasic:
    return nameBase + " Basic";

  case kBusiness:
    return nameBase + " Business";

  case kBasicChina:
    return nameBase + " 中文版";

  case kProChina:
    return nameBase + " Pro 中文版";

  default:
    return nameBase + " (Unregistered)";
  }
}

void SerialKeyEdition::setType(Edition type) {
  m_type = type;
  setType(getName());
}

void SerialKeyEdition::setType(const std::string &name) {
  const auto &pType = serialTypes.find(name);

  if (pType != serialTypes.end()) {
    m_type = pType->second;
  } else {
    m_type = kUnregistered;
  }
}

bool SerialKeyEdition::isValid() const {
  return serialTypes.contains(getName());
}

bool SerialKeyEdition::isChina() const {
  return ((m_type == kBasicChina) || (m_type == kProChina));
}
