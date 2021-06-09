#include <algorithm>
#include <map>

#include "SerialKeyEdition.h"

const std::string SerialKeyEdition::PRO = "pro";
const std::string SerialKeyEdition::PRO_CHINA = "pro_china";
const std::string SerialKeyEdition::BASIC = "basic";
const std::string SerialKeyEdition::BASIC_CHINA = "basic_china";
const std::string SerialKeyEdition::BUSINESS = "business";
const std::string SerialKeyEdition::UNREGISTERED = "unregistered";

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
}

void
SerialKeyEdition::setType(const std::string& type)
{
    static const std::map<std::string, Edition> types = {
        {BASIC, kBasic},
        {PRO, kPro},
        {BUSINESS, kBusiness},
        {BASIC_CHINA, kBasic_China},
        {PRO_CHINA, kPro_China}
    };

    const auto& pType = types.find(type);

    if (pType != types.end()) {
        m_Type = pType->second;
    }
    else {
        m_Type = kUnregistered;
    }
}
