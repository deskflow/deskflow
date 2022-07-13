#include <algorithm>
#include <map>

#include "SerialKeyEdition.h"

const std::string SerialKeyEdition::PRO = "pro";
const std::string SerialKeyEdition::PRO_CHINA = "pro_china";
const std::string SerialKeyEdition::BASIC = "basic";
const std::string SerialKeyEdition::BASIC_CHINA = "basic_china";
const std::string SerialKeyEdition::BUSINESS = "business";
const std::string SerialKeyEdition::UNREGISTERED = "unregistered";
const std::string SerialKeyEdition::ULTIMATE = "ultimate";
const std::string SerialKeyEdition::LITE = "lite";

namespace {

const std::map<std::string, Edition>& getSerialTypes()
{
#ifdef SYNERGY_BUSINESS
    static const std::map<std::string, Edition> serialTypes = {
        {SerialKeyEdition::BUSINESS, kBusiness}
    };
#else
    static const std::map<std::string, Edition> serialTypes {
        {SerialKeyEdition::BASIC, kBasic},
        {SerialKeyEdition::PRO, kPro},
        {SerialKeyEdition::BASIC_CHINA, kBasic_China},
        {SerialKeyEdition::PRO_CHINA, kPro_China},
        {SerialKeyEdition::BUSINESS, kBusiness},
        {SerialKeyEdition::LITE, kLite},
        {SerialKeyEdition::ULTIMATE, kUltimate}
    };
#endif
    return serialTypes;
}

} //namespace

SerialKeyEdition::SerialKeyEdition()
{

}

SerialKeyEdition::SerialKeyEdition(Edition type) :
	m_Type(type)
{

}

SerialKeyEdition::SerialKeyEdition(const std::string &type)
{
	setType(type);
}

Edition
SerialKeyEdition::getType() const
{
	return m_Type;
}

std::string
SerialKeyEdition::getName() const
{
	std::string Name;

	switch(getType()){
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
		case kBasic_China:
			Name = BASIC_CHINA;
			break;
		case kPro_China:
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

std::string
SerialKeyEdition::getDisplayName() const
{
    const std::string ApplicationName = "Synergy 1 ";
    std::string DisplayName(ApplicationName);

    switch (getType())
    {
        case kBasic_China:
            DisplayName = "Synergy 中文版";
            break;
        case kPro_China:
            DisplayName = "Synergy Pro 中文版";
            break;
        case kLite:
            DisplayName = "Synergy 1";
            break;
        default:
            std::string EditionName = getName();
            if (!EditionName.empty()){
                if (EditionName == UNREGISTERED){
                   std::transform(EditionName.begin(), EditionName.end(), EditionName.begin(), ::toupper);
                   EditionName = "(" + EditionName +")";
                }
                else{
                   EditionName[0] = static_cast<char>(::toupper(EditionName[0]));
                }
                DisplayName = ApplicationName + EditionName;
             }
    }

    return DisplayName;
}

void
SerialKeyEdition::setType(Edition type)
{
	m_Type = type;
	setType(getName());
}

void
SerialKeyEdition::setType(const std::string& type)
{
    auto types = getSerialTypes();
    const auto& pType = types.find(type);

    if (pType != types.end()) {
        m_Type = pType->second;
    }
    else {
        m_Type = kUnregistered;
    }
}

bool
SerialKeyEdition::isValid() const
{
    auto types = getSerialTypes();
    return (types.find(getName()) != types.end());
}

bool
SerialKeyEdition::isChina() const {
    return ((m_Type == kBasic_China) || (m_Type == kPro_China));
}
