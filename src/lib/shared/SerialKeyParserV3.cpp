#include "SerialKeyParserV3.h"

bool
SerialKeyParserV3::parse(const std::string& plainSerial)
{
    setKey(plainSerial);
    return true;
}
