#include "SerialKeyParserV3.h"

SerialKeyParserV3::SerialKeyParserV3()
{

}

bool
SerialKeyParserV3::parse(const std::string& plainSerial)
{
    setKey(plainSerial);
}
