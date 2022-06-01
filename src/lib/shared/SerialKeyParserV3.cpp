#include "SerialKeyParserV3.h"

#if defined(BUILD_V3_LICENSE)
#include "license-parser/src/lib.rs.h"

bool SerialKeyParserV3::parse(const std::string &plainSerial)
{
    setKey(plainSerial);
    shared::LicenseData data = validate_license(rust::String(plainSerial));
    setType(data.trial ? "trial" : "");
    setEdition(std::string(data.license_type));
    setWarningTime(std::string(data.warn));
    setExpirationTime(std::string(data.expiration));
    return data.valid;
}

#else

bool SerialKeyParserV3::parse(const std::string &plainSerial)
{
    setKey(plainSerial);
    return false;
}

#endif
