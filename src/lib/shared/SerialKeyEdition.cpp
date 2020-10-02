#include <algorithm>
#include "SerialKeyEdition.h"

const std::string SerialKeyEdition::PRO = "pro";
const std::string SerialKeyEdition::BASIC = "basic";
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
		default:
			break;
	}

	return Name;
}

std::string
SerialKeyEdition::getDisplayName() const
{
	const std::string ApplicationName = "Synergy 1 ";
	std::string EditionName = getName();

	if (!EditionName.empty()){
		if (EditionName == UNREGISTERED){
			std::transform(EditionName.begin(), EditionName.end(), EditionName.begin(), ::toupper);
			EditionName = "(" + EditionName +")";
		}
		else{
			EditionName[0] = static_cast<char>(::toupper(EditionName[0]));
		}
	}

	return (ApplicationName + EditionName);
}

void
SerialKeyEdition::setType(Edition type)
{
	m_Type = type;
}

void
SerialKeyEdition::setType(const std::string& type)
{
	if (type == BASIC){
		m_Type = kBasic;
	}
	else if (type == PRO){
		m_Type = kPro;
	}
	else if (type == BUSINESS){
		m_Type = kBusiness;
	}
	else{
		m_Type = kUnregistered;
	}
}
