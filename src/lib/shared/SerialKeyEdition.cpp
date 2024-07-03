#include <algorithm>
#include <map>

#include "SerialKeyEdition.h"

const std::string SerialKeyEdition::PRO = "pro";
const std::string SerialKeyEdition::PRO_CHINA = "pro_china";
const std::string SerialKeyEdition::BASIC = "basic";
const std::string SerialKeyEdition::BASIC_CHINA = "basic_china";
const std::string SerialKeyEdition::BUSINESS = "business";
const std::string SerialKeyEdition::UNREGISTERED = "community";
const std::string SerialKeyEdition::ULTIMATE = "ultimate";
const std::string SerialKeyEdition::LITE = "lite";

namespace {

const std::map<std::string, Edition> &getSerialTypes() {
  static const std::map<std::string, Edition> serialTypes{
      {SerialKeyEdition::BASIC, kBasic},
      {SerialKeyEdition::PRO, kPro},
      {SerialKeyEdition::BASIC_CHINA, kBasicChina},
      {SerialKeyEdition::PRO_CHINA, kProChina},
      {SerialKeyEdition::BUSINESS, kBusiness},
      {SerialKeyEdition::LITE, kLite},
      {SerialKeyEdition::ULTIMATE, kUltimate}};
  return serialTypes;
}

} // namespace

SerialKeyEdition::SerialKeyEdition() {}

SerialKeyEdition::SerialKeyEdition(Edition type) : m_Type(type) {}

SerialKeyEdition::SerialKeyEdition(const std::string &type) { setType(type); }

Edition SerialKeyEdition::getType() const { return m_Type; }

std::string SerialKeyEdition::getName() const {
  std::string Name;

  switch (getType()) {
  case kPro:
    Name = PRO;
    break;
  case kBasic:
    Name = BASIC;
    break;
  case kBusiness:
    Name = BUSINESS;
    break;
  case kUnregistered:
    Name = UNREGISTERED;
    break;
  case kBasicChina:
    Name = BASIC_CHINA;
    break;
  case kProChina:
    Name = PRO_CHINA;
    break;
  case kLite:
    Name = LITE;
    break;
  case kUltimate:
    Name = ULTIMATE;
    break;
  default:
    break;
  }

  return Name;
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
  m_Type = type;
  setType(getName());
}

void SerialKeyEdition::setType(const std::string &type) {
  auto types = getSerialTypes();
  const auto &pType = types.find(type);

  if (pType != types.end()) {
    m_Type = pType->second;
  } else {
    m_Type = kUnregistered;
  }
}

bool SerialKeyEdition::isValid() const {
  auto types = getSerialTypes();
  return (types.find(getName()) != types.end());
}

bool SerialKeyEdition::isChina() const {
  return ((m_Type == kBasicChina) || (m_Type == kProChina));
}
