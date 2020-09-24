#include "SerialKeyType.h"

const std::string SerialKeyType::TRIAL = "trial";
const std::string SerialKeyType::TEMPORARY = "temp";

SerialKeyType::SerialKeyType() :
    m_isTrial(false),
    m_isTemporary(false)
{

}

void SerialKeyType::setKeyType(const std::string& Type)
{
    m_isTrial = false;
    m_isTemporary = false;

    if (Type == SerialKeyType::TRIAL){
        m_isTrial = true;
        m_isTemporary = true;
    }
    else if (Type == SerialKeyType::TEMPORARY){
        m_isTemporary = true;
    }
}

bool SerialKeyType::isTrial() const
{
    return m_isTrial;
}

bool SerialKeyType::isTemporary() const
{
    return m_isTemporary;
}

bool SerialKeyType::isPermanent() const
{
    return (!m_isTemporary);
}
